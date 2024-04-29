/*
Copyright 2024 Nvidia Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "unrollLoops.h"

#include "ir/pattern.h"

/** helper function to create a simple assignment to a boolean flag variable */
static IR::AssignmentStatement *setFlag(cstring name, bool val) {
    auto var = new IR::PathExpression(IR::Type::Boolean::get(), new IR::Path(name));
    return new IR::AssignmentStatement(var, new IR::BoolLiteral(val));
}

namespace P4 {

/** transform to remove break and continue statements from the body of a loop
 * This pass is designed to run on just the body of a loop, generally returning a
 * new BlockStatement with the break and continue statements removed and replaced
 * with new temp bool variables.  The declarations and initializations of the temps
 * need to be inserted elsewhere -- the temp for continue can be added to the top
 * of the loop body (and should be set to false there), but the temp for break needs
 * to be defined and set to false outside the loop and should be checked in the loop
 * condition
 */
class RemoveBreakContinue : public Transform {
    NameGenerator &nameGen;

 public:
    explicit RemoveBreakContinue(NameGenerator &ng) : nameGen(ng) {}

    IR::Declaration_Variable *breakFlag = nullptr;
    IR::Declaration_Variable *continueFlag = nullptr;
    bool needFlagCheck = false;

 private:
    profile_t init_apply(const IR::Node *root) override {
        breakFlag = continueFlag = nullptr;
        needFlagCheck = false;
        return Transform::init_apply(root);
    }

    // skip nested loops -- they'll need to be expanded themselves
    IR::Node *preorder(IR::ForStatement *s) override {
        prune();
        return preorder(static_cast<IR::Statement *>(s));
    }
    IR::Node *preorder(IR::ForInStatement *s) override {
        prune();
        return preorder(static_cast<IR::Statement *>(s));
    }

    IR::Node *postorder(IR::BreakStatement *) override {
        if (!breakFlag) {
            breakFlag = new IR::Declaration_Variable(
                nameGen.newName("breakFlag"), IR::Type_Boolean::get(), new IR::BoolLiteral(false));
        }
        needFlagCheck = true;
        return setFlag(breakFlag->name, true);
    }
    IR::Node *postorder(IR::ContinueStatement *) override {
        if (!continueFlag) {
            continueFlag =
                new IR::Declaration_Variable(nameGen.newName("continueFlag"),
                                             IR::Type_Boolean::get(), new IR::BoolLiteral(false));
        }
        needFlagCheck = true;
        return setFlag(continueFlag->name, true);
    }
    IR::Node *preorder(IR::Statement *s) override {
        if (!needFlagCheck) return s;
        const IR::Expression *cond = nullptr;
        if (breakFlag) cond = new IR::LNot(new IR::PathExpression(breakFlag->name));
        if (continueFlag) {
            const IR::Expression *c2 = new IR::LNot(new IR::PathExpression(continueFlag->name));
            cond = cond ? new IR::LAnd(cond, c2) : c2;
        }
        BUG_CHECK(cond, "need flag check but no flags?");
        needFlagCheck = false;
        s->visit_children(*this);
        needFlagCheck = true;
        prune();
        return new IR::IfStatement(cond, s, nullptr);
    }
};

/* replace references to the 'index' var with a constant */
class ReplaceIndexRefs : public Transform {
    cstring indexVar;
    long value;

    IR::Node *preorder(IR::PathExpression *pe) {
        if (pe->path->name == indexVar) return new IR::Constant(pe->type, value);
        return pe;
    }

 public:
    ReplaceIndexRefs(cstring iv, long v) : indexVar(iv), value(v) { forceClone = true; }
};

bool UnrollLoops::findLoopBounds(IR::ForStatement *fstmt, loop_bounds_t &bounds) {
    Pattern::Match<IR::PathExpression> v;
    Pattern::Match<IR::Constant> k;
    if (v.Relation(k).match(fstmt->condition)) {
        auto d = resolveUnique(v->path->name, P4::ResolutionType::Any);
        bounds.index = d ? d->to<IR::Declaration_Variable>() : nullptr;
        if (!bounds.index) return false;
        auto &defs = defUse->getDefs(v);
        const IR::AssignmentStatement *init = nullptr, *incr = nullptr;
        for (auto *d : defs) {
            bool ok = false;
            for (auto *a : fstmt->init) {
                if (a == d->parent->node) {
                    init = a->to<IR::AssignmentStatement>();
                    ok = true;
                    break;
                }
            }
            for (auto *a : fstmt->updates) {
                if (a == d->parent->node) {
                    incr = a->to<IR::AssignmentStatement>();
                    ok = true;
                    break;
                }
            }
            if (!ok) return false;
        }
        if (!init || !incr) return false;
        auto initval = init->right->to<IR::Constant>();
        if (!initval) return false;
        Pattern::Match<IR::PathExpression> v2;
        Pattern::Match<IR::Constant> k2;
        long neg_incr = 1;
        if ((v2 + k2).match(incr->right)) {
            if (!v2->equiv(*v)) return false;
            neg_incr = 1;
        } else if ((v2 - k2).match(incr->right)) {
            if (!v2->equiv(*v)) return false;
            neg_incr = -1;
        } else {
            return false;
        }
        bounds.step = k2->asLong() * neg_incr;
        if (bounds.step > 0) {
            bounds.min = initval->asLong();
            bounds.max = k->asLong();
            if (fstmt->condition->is<IR::Lss>())
                bounds.max--;
            else if (!fstmt->condition->is<IR::Leq>())
                return false;
            ;
        } else {
            bounds.min = k->asLong();
            bounds.max = initval->asLong();
            if (fstmt->condition->is<IR::Grt>())
                bounds.min++;
            else if (!fstmt->condition->is<IR::Geq>())
                return false;
            ;
        }
        return true;
    }
    return false;
}

bool UnrollLoops::findLoopBounds(IR::ForInStatement *fstmt, loop_bounds_t &bounds) {
    if (auto range = fstmt->collection->to<IR::Range>()) {
        auto lo = range->left->to<IR::Constant>();
        auto hi = range->right->to<IR::Constant>();
        if (lo && hi) {
            bounds.min = lo->asLong();
            bounds.max = hi->asLong();
            bounds.step = 1;
            if (!(bounds.index = fstmt->decl)) {
                auto d = resolveUnique(fstmt->ref->path->name, P4::ResolutionType::Any);
                BUG_CHECK(d && d->is<IR::Declaration_Variable>(), "index is not a var?");
                bounds.index = d->to<IR::Declaration_Variable>();
            }
            return true;
        }
    }
    return false;
}

const IR::Statement *UnrollLoops::doUnroll(const loop_bounds_t &bounds, const IR::Statement *body) {
    RemoveBreakContinue rbc(nameGen);
    body = body->apply(rbc, getChildContext());
    if (rbc.continueFlag)
        body = new IR::BlockStatement({setFlag(rbc.continueFlag->name, false), body});
    auto *rv = new IR::BlockStatement(body->srcInfo);
    if (auto *bs = body->to<IR::BlockStatement>()) rv->annotations = bs->annotations;
    if (rbc.breakFlag) rv->append(rbc.breakFlag);
    if (rbc.continueFlag) rv->append(rbc.continueFlag);
    if (rbc.breakFlag) rv->append(setFlag(rbc.breakFlag->name, false));
    if (bounds.step > 0) {
        for (long indexval = bounds.min; indexval <= bounds.max; indexval += bounds.step)
            rv->append(body->apply(ReplaceIndexRefs(bounds.index->name, indexval)));
    } else {
        for (long indexval = bounds.max; indexval >= bounds.min; indexval += bounds.step)
            rv->append(body->apply(ReplaceIndexRefs(bounds.index->name, indexval)));
    }
    return rv;
}

const IR::Statement *UnrollLoops::preorder(IR::ForStatement *fstmt) {
    loop_bounds_t bounds;
    if (findLoopBounds(fstmt, bounds)) {
        return doUnroll(bounds, fstmt->body);
    }
    return fstmt;
}

const IR::Statement *UnrollLoops::preorder(IR::ForInStatement *fstmt) {
    loop_bounds_t bounds;
    if (findLoopBounds(fstmt, bounds)) {
        return doUnroll(bounds, fstmt->body);
    }
    return fstmt;
}
}  // namespace P4
