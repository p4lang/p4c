// SPDX-FileCopyrightText: 2026 Abhishek Agarwal
//
// SPDX-License-Identifier: Apache-2.0

#include "elimDeadSetInvalid.h"

#include <set>
#include <vector>

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/parserCallGraph.h"
#include "ir/ir.h"
#include "midend/def_use.h"

namespace P4 {

namespace {

using DeclSet = std::set<const IR::IDeclaration *>;

bool isFieldOf(const IR::Member *m) {
    auto *type = m->expr->type;
    if (!type->is<IR::Type_Struct>() && !type->is<IR::Type_HeaderUnion>()) return false;
    return type->to<IR::Type_StructLike>()->getField(m->member.name) != nullptr;
}

/// Copies parsers so that no select case is shared between states. ParsersUnroll can
/// leave one shared by several states, which ComputeDefUse then reports as an IR loop.
class UnshareParserSelects : public Transform {
 public:
    UnshareParserSelects() {
        visitDagOnce = false;
        forceClone = true;
        setName("UnshareParserSelects");
    }
    const IR::Node *preorder(IR::P4Parser *parser) override { return parser; }
    const IR::Node *preorder(IR::P4Control *control) override {
        prune();
        return control;
    }
    const IR::Node *preorder(IR::Type *type) override {
        prune();
        return type;
    }
};

/// Locals used in ways ComputeDefUse does not link to the valid bit: whole reads,
/// non-constant indexes, stack operations, and methods on structs or unions.
/// Also all locals of parsers with loops, as ComputeDefUse does not follow parser back edges.
class FindUnsafeLocals : public Inspector, public ResolutionContext {
    DeclSet &unsafe;

 public:
    explicit FindUnsafeLocals(DeclSet &unsafe) : unsafe(unsafe) {
        visitDagOnce = false;
        setName("FindUnsafeLocals");
    }

    Visitor::profile_t init_apply(const IR::Node *node) override {
        unsafe.clear();
        return Inspector::init_apply(node);
    }

    void postorder(const IR::PathExpression *pe) override {
        if (auto *mce = getParent<IR::MethodCallExpression>(); mce && mce->method == pe) return;
        auto *decl = getDeclaration(pe->path);
        if (!decl || !decl->is<IR::Declaration_Variable>()) return;
        const IR::Expression *e = pe;
        const Context *ctxt = getContext();
        for (; ctxt; ctxt = ctxt->parent) {
            if (auto *m = ctxt->node->to<IR::Member>()) {
                if (m->expr->type->is<IR::Type_Header>()) return;  // a field or a header method
                if (!isFieldOf(m)) break;
                e = m;
            } else if (auto *ai = ctxt->node->to<IR::ArrayIndex>()) {
                if (ai->left != e || !ai->right->is<IR::Constant>()) break;
                e = ai;
            } else {
                break;
            }
        }
        if (ctxt) {
            if (auto *as = ctxt->node->to<IR::AssignmentStatement>()) {
                if (as->left == e) return;
            } else if (auto *arg = ctxt->node->to<IR::Argument>()) {
                auto *mce = findContext<IR::MethodCallExpression>();
                if (auto *type = mce ? mce->method->type->to<IR::Type_MethodBase>() : nullptr) {
                    ParameterSubstitution subst;
                    subst.populate(type->parameters, mce->arguments);
                    auto *param = subst.findParameter(arg);
                    if (param && param->direction == IR::Direction::Out) return;
                }
            }
        }
        if (unsafe.insert(decl).second) LOG3("Unsafe local " << decl->getName());
    }

    void postorder(const IR::P4Parser *parser) override {
        ParserCallGraph transitions("transitions");
        ComputeParserCG pcg(&transitions);
        pcg.setCalledBy(this);
        parser->apply(pcg, getChildContext());
        auto *start = parser->getDeclByName(IR::ParserState::start)->to<IR::ParserState>();
        std::vector<const IR::ParserState *> order;
        if (!transitions.sccSort(start, order)) return;
        LOG3("Parser " << parser->name << " has a loop, all its locals are unsafe");
        forAllMatching<IR::Declaration_Variable>(
            parser, [&](const IR::Declaration_Variable *v) { unsafe.insert(v); });
    }
};

class DoElimDeadSetInvalid : public Transform, public ResolutionContext {
    const ComputeDefUse *defuse;
    const DeclSet &unsafe;

    // The local at the root of a path of struct fields and constant indexes. A member of a
    // header union is not followed: setInvalid() on it clears the other members too.
    const IR::Declaration_Variable *rootLocal(const IR::Expression *e) const {
        while (true) {
            if (auto *pe = e->to<IR::PathExpression>())
                return getDeclaration(pe->path, true)->to<IR::Declaration_Variable>();
            if (auto *m = e->to<IR::Member>()) {
                if (!isFieldOf(m) || m->expr->type->is<IR::Type_HeaderUnion>()) return nullptr;
                e = m->expr;
            } else if (auto *ai = e->to<IR::ArrayIndex>()) {
                if (!ai->right->is<IR::Constant>()) return nullptr;
                e = ai->left;
            } else {
                return nullptr;
            }
        }
    }

 public:
    DoElimDeadSetInvalid(const ComputeDefUse *defuse, const DeclSet &unsafe)
        : defuse(defuse), unsafe(unsafe) {
        visitDagOnce = false;
        setName("DoElimDeadSetInvalid");
    }

    const IR::Node *postorder(IR::MethodCallStatement *mcs) override {
        auto *call = getOriginal<IR::MethodCallStatement>()->methodCall;
        auto *method = call->method->to<IR::Member>();
        if (!method || method->member != IR::Type_Header::setInvalid ||
            !method->expr->type->is<IR::Type_Header>())
            return mcs;
        auto *decl = rootLocal(method->expr);
        if (!decl || unsafe.contains(decl) || !defuse->getUses(call).empty()) return mcs;
        LOG3("Removing dead " << call);
        if (getParent<IR::BlockStatement>() || getParent<IR::ParserState>()) return nullptr;
        auto *orig = getOriginal();
        if (auto *ifs = getParent<IR::IfStatement>())
            if (ifs->ifFalse == orig && ifs->ifTrue != orig) return nullptr;
        return new IR::EmptyStatement(mcs->srcInfo);
    }
};

}  // namespace

ElimDeadSetInvalid::ElimDeadSetInvalid() {
    auto *unsafe = new DeclSet;
    auto *defuse = new ComputeDefUse;
    passes.push_back(new UnshareParserSelects);
    passes.push_back(defuse);
    passes.push_back(new FindUnsafeLocals(*unsafe));
    passes.push_back(new DoElimDeadSetInvalid(defuse, *unsafe));
    setName("ElimDeadSetInvalid");
}

}  // namespace P4
