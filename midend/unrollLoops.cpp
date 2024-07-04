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
            breakFlag =
                new IR::Declaration_Variable(nameGen.newName("breakFlag"), IR::Type_Boolean::get());
        }
        needFlagCheck = true;
        return setFlag(breakFlag->name, true);
    }
    IR::Node *postorder(IR::ContinueStatement *) override {
        if (!continueFlag) {
            continueFlag = new IR::Declaration_Variable(nameGen.newName("continueFlag"),
                                                        IR::Type_Boolean::get());
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
        auto s2 = s->apply_visitor_postorder(*this)->to<IR::Statement>();
        BUG_CHECK(s2, "RemoveBreakContinue::postorder failed to return a statement");
        return new IR::IfStatement(cond, s2, nullptr);
    }
};

/* replace references to the 'index' var with a constant */
class ReplaceIndexRefs : public Transform, P4WriteContext {
    cstring indexVar;
    long value;
    bool stop_after_modification = false;

    IR::Node *preorder(IR::PathExpression *pe) {
        if (stop_after_modification || pe->path->name != indexVar) return pe;
        if (isWrite()) {
            stop_after_modification = true;
            return pe;
        }
        return new IR::Constant(pe->type, value);
    }
    IR::AssignmentStatement *preorder(IR::AssignmentStatement *assign) {
        visit(assign->right, "right", 1);
        visit(assign->left, "left", 0);
        prune();
        return assign;
    }

 public:
    ReplaceIndexRefs(cstring iv, long v) : indexVar(iv), value(v) { forceClone = true; }
};

static long evalLoop(const IR::Expression *exp, cstring index, long val, bool &fail) {
    if (auto *pe = exp->to<IR::PathExpression>()) {
        if (pe->path->name != index) fail = true;
        return val;
    } else if (auto *k = exp->to<IR::Constant>()) {
        return k->asLong();
    } else if (auto *e = exp->to<IR::Leq>()) {
        return evalLoop(e->left, index, val, fail) <= evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Lss>()) {
        return evalLoop(e->left, index, val, fail) < evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Geq>()) {
        return evalLoop(e->left, index, val, fail) >= evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Grt>()) {
        return evalLoop(e->left, index, val, fail) > evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Equ>()) {
        return evalLoop(e->left, index, val, fail) == evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Neq>()) {
        return evalLoop(e->left, index, val, fail) != evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Add>()) {
        return evalLoop(e->left, index, val, fail) + evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Sub>()) {
        return evalLoop(e->left, index, val, fail) - evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Mul>()) {
        return evalLoop(e->left, index, val, fail) * evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Div>()) {
        return evalLoop(e->left, index, val, fail) / evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Mod>()) {
        return evalLoop(e->left, index, val, fail) % evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Shl>()) {
        return evalLoop(e->left, index, val, fail) << evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Shr>()) {
        return evalLoop(e->left, index, val, fail) >> evalLoop(e->right, index, val, fail);
    } else if (auto *e = exp->to<IR::Neg>()) {
        return -evalLoop(e->expr, index, val, fail);
    } else if (auto *e = exp->to<IR::Cmpl>()) {
        return ~evalLoop(e->expr, index, val, fail);
    } else if (auto *e = exp->to<IR::LNot>()) {
        return !evalLoop(e->expr, index, val, fail);
    }
    fail = true;
    return 0;
}

static const IR::PathExpression *incrRecursiveUse(const IR::Expression *e, cstring index) {
    if (auto *pe = e->to<IR::PathExpression>()) {
        if (pe->path->name == index) return pe;
    } else if (auto *bin = e->to<IR::Operation::Binary>()) {
        if (auto *pe = incrRecursiveUse(bin->left, index)) return pe;
        if (auto *pe = incrRecursiveUse(bin->right, index)) return pe;
    } else if (auto *un = e->to<IR::Operation::Unary>()) {
        if (auto *pe = incrRecursiveUse(un->expr, index)) return pe;
    }
    return nullptr;
}

static bool find_def(const IR::IndexedVector<IR::StatOrDecl> &stmts, const IR::Node *def,
                     const IR::AssignmentStatement **as = nullptr) {
    for (auto *a : stmts) {
        if (a == def) {
            if (as) *as = a->to<IR::AssignmentStatement>();
            return true;
        }
    }
    return false;
}

bool UnrollLoops::findLoopBounds(IR::ForStatement *fstmt, loop_bounds_t &bounds) {
    Pattern::Match<IR::PathExpression> v;
    Pattern::Match<IR::Constant> k;
    if (v.Relation(k).match(fstmt->condition)) {
        auto d = resolveUnique(v->path->name, P4::ResolutionType::Any);
        bounds.index = d ? d->to<IR::Declaration_Variable>() : nullptr;
        if (!bounds.index) return false;
        const IR::AssignmentStatement *init = nullptr, *incr = nullptr;
        for (auto *d : defUse->getDefs(v)) {
            if (!find_def(fstmt->init, d->parent->node, &init) &&
                !find_def(fstmt->updates, d->parent->node, &incr))
                return false;
        }
        if (!init || !incr) return false;
        auto initval = init->right->to<IR::Constant>();
        if (!initval) return false;
        auto *incr_ref = incrRecursiveUse(incr->right, bounds.index->name);
        if (!incr_ref) return false;
        for (auto *d : defUse->getDefs(incr_ref)) {
            if (!find_def(fstmt->init, d->parent->node) &&
                !find_def(fstmt->updates, d->parent->node))
                return false;
        }
        bool fail = false;
        for (long val = initval->asLong();
             evalLoop(fstmt->condition, bounds.index->name, val, fail) && !fail;
             val = evalLoop(incr->right, bounds.index->name, val, fail)) {
            if (bounds.indexes.size() > 1000) {
                fail = true;
                break;
            }
            bounds.indexes.push_back(val);
        }
        return !fail;
    }
    return false;
}

bool UnrollLoops::findLoopBounds(IR::ForInStatement *fstmt, loop_bounds_t &bounds) {
    if (auto range = fstmt->collection->to<IR::Range>()) {
        auto lo = range->left->to<IR::Constant>();
        auto hi = range->right->to<IR::Constant>();
        if (lo && hi) {
            for (long i = lo->asLong(); i <= hi->asLong(); ++i) bounds.indexes.push_back(i);
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

const IR::Statement *UnrollLoops::doUnroll(const loop_bounds_t &bounds, const IR::Statement *body,
                                           const IR::IndexedVector<IR::StatOrDecl> *updates) {
    RemoveBreakContinue rbc(nameGen);
    body = body->apply(rbc, getChildContext());
    if (rbc.continueFlag)
        body = new IR::BlockStatement({setFlag(rbc.continueFlag->name, false), body});
    auto *rv = new IR::BlockStatement(body->srcInfo);
    if (auto *bs = body->to<IR::BlockStatement>()) rv->annotations = bs->annotations;
    if (rbc.breakFlag) rv->append(rbc.breakFlag);
    if (rbc.continueFlag) rv->append(rbc.continueFlag);
    if (rbc.breakFlag) rv->append(setFlag(rbc.breakFlag->name, false));
    IR::BlockStatement *blk = rv;
    for (long indexval : bounds.indexes) {
        ReplaceIndexRefs rir(bounds.index->name, indexval);
        blk->append(body->apply(rir));
        if (rbc.breakFlag) {
            auto newblk = new IR::BlockStatement;
            auto cond = new IR::LNot(new IR::PathExpression(rbc.breakFlag->name));
            blk->append(new IR::IfStatement(cond, newblk, nullptr));
            blk = newblk;
        }
        if (updates) {
            for (auto *u : *updates) {
                auto *n = u->apply(rir)->to<IR::StatOrDecl>();
                BUG_CHECK(n, "unexpected nullptr");
                blk->append(n);
            }
        }
    }
    return rv;
}

const IR::Statement *UnrollLoops::preorder(IR::ForStatement *fstmt) {
    loop_bounds_t bounds;
    if (findLoopBounds(fstmt, bounds)) {
        LOG3("Unrolling loop" << Log::indent << Log::endl << fstmt << Log::unindent);
        auto *rv = new IR::BlockStatement;
        for (auto *i : fstmt->init) rv->append(i);
        rv->append(doUnroll(bounds, fstmt->body, &fstmt->updates));
        LOG4("Unrolled loop" << Log::indent << Log::endl << rv << Log::unindent);
        return rv;
    }
    return fstmt;
}

const IR::Statement *UnrollLoops::preorder(IR::ForInStatement *fstmt) {
    loop_bounds_t bounds;
    if (findLoopBounds(fstmt, bounds)) {
        LOG3("Unrolling loop" << Log::indent << Log::endl << fstmt << Log::unindent);
        auto rv = doUnroll(bounds, fstmt->body);
        LOG4("Unrolled loop" << Log::indent << Log::endl << rv << Log::unindent);
        return rv;
    }
    return fstmt;
}
}  // namespace P4
