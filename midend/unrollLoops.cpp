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

namespace P4 {

/** helper function to create a simple assignment to a boolean flag variable */
static IR::AssignmentStatement *setFlag(cstring name, bool val) {
    auto var = new IR::PathExpression(IR::Type::Boolean::get(), new IR::Path(name));
    return new IR::AssignmentStatement(var, new IR::BoolLiteral(val));
}

}  // namespace P4

namespace P4 {

using namespace literals;

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

long UnrollLoops::evalLoop(const IR::Expression *exp, long val,
                           const ComputeDefUse::locset_t &idefs, bool &fail) {
    if (fail) return 1;
    if (auto *pe = exp->to<IR::PathExpression>()) {
        if (defUse->getDefs(pe) != idefs) fail = true;
    } else if (auto *k = exp->to<IR::Constant>()) {
        val = k->asLong();
    } else if (auto *e = exp->to<IR::Leq>()) {
        val = evalLoop(e->left, val, idefs, fail) <= evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Lss>()) {
        val = evalLoop(e->left, val, idefs, fail) < evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Geq>()) {
        val = evalLoop(e->left, val, idefs, fail) >= evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Grt>()) {
        val = evalLoop(e->left, val, idefs, fail) > evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Equ>()) {
        val = evalLoop(e->left, val, idefs, fail) == evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Neq>()) {
        val = evalLoop(e->left, val, idefs, fail) != evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Add>()) {
        val = evalLoop(e->left, val, idefs, fail) + evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Sub>()) {
        val = evalLoop(e->left, val, idefs, fail) - evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Mul>()) {
        val = evalLoop(e->left, val, idefs, fail) * evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Div>()) {
        val = evalLoop(e->left, val, idefs, fail) / evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Mod>()) {
        val = evalLoop(e->left, val, idefs, fail) % evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Shl>()) {
        val = evalLoop(e->left, val, idefs, fail) << evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Shr>()) {
        val = evalLoop(e->left, val, idefs, fail) >> evalLoop(e->right, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Neg>()) {
        val = -evalLoop(e->expr, val, idefs, fail);
    } else if (auto *e = exp->to<IR::Cmpl>()) {
        val = ~evalLoop(e->expr, val, idefs, fail);
    } else if (auto *e = exp->to<IR::LNot>()) {
        val = !evalLoop(e->expr, val, idefs, fail);
    } else {
        fail = true;
        return 1;
    }
    if (auto *bt = exp->type->to<IR::Type::Bits>()) {
        if (bt->size > 0 && bt->size < int(CHAR_BIT * sizeof(long))) {
            val = val & ((1ULL << bt->size) - 1);
            if (bt->isSigned && (val >> (bt->size - 1)) != 0) val -= (1LL << bt->size);
        }
    }
    return val;
}

bool UnrollLoops::findLoopBounds(IR::ForStatement *fstmt, loop_bounds_t &bounds) {
    Pattern::Match<IR::PathExpression> v;
    Pattern::Match<IR::Constant> k;
    if (v.Relation(k).match(fstmt->condition)) {
        auto d = resolveUnique(v->path->name, P4::ResolutionType::Any);
        bounds.index = d ? d->to<IR::Declaration_Variable>() : nullptr;
        if (!bounds.index) return false;
        const IR::AssignmentStatement *init = nullptr, *incr = nullptr;
        const IR::Constant *initval = nullptr;
        auto &index_defs = defUse->getDefs(v);
        if (index_defs.size() != 2) {
            // must be exactly 2 defs -- one to a constant before the loop and one
            // to a non-constant in the loop
            return false;
        } else if ((init = index_defs.front()->parent->node->to<IR::AssignmentStatement>()) &&
                   (initval = init->right->to<IR::Constant>())) {
            incr = index_defs.back()->parent->node->to<IR::AssignmentStatement>();
        } else if ((init = index_defs.back()->parent->node->to<IR::AssignmentStatement>()) &&
                   (initval = init->right->to<IR::Constant>())) {
            incr = index_defs.front()->parent->node->to<IR::AssignmentStatement>();
        } else {
            return false;
        }
        if (!incr) return false;
        bool fail = false;
        for (long val = initval->asLong();
             evalLoop(fstmt->condition, val, index_defs, fail) && !fail;
             val = evalLoop(incr->right, val, index_defs, fail)) {
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
    bool canUnroll = findLoopBounds(fstmt, bounds);
    bool shouldUnroll = policy(fstmt, canUnroll, bounds);
    if (canUnroll && shouldUnroll) {
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
    bool canUnroll = findLoopBounds(fstmt, bounds);
    bool shouldUnroll = policy(fstmt, canUnroll, bounds);
    if (canUnroll && shouldUnroll) {
        LOG3("Unrolling loop" << Log::indent << Log::endl << fstmt << Log::unindent);
        auto rv = doUnroll(bounds, fstmt->body);
        LOG4("Unrolled loop" << Log::indent << Log::endl << rv << Log::unindent);
        return rv;
    }
    return fstmt;
}

UnrollLoops::Policy UnrollLoops::default_unroll(true);
UnrollLoops::Policy UnrollLoops::default_nounroll(false);

bool UnrollLoops::Policy::operator()(const IR::LoopStatement *loop, bool canUnroll,
                                     const loop_bounds_t &) {
    if (loop->getAnnotation("unroll"_cs)) {
        if (!canUnroll) warning(ErrorType::WARN_UNSUPPORTED, "Can't unroll loop: %1%", loop);
        return true;
    }
    if (loop->getAnnotation("nounroll"_cs)) return false;
    return unroll_default;
}

}  // namespace P4
