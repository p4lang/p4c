// SPDX-FileCopyrightText: 2026 Abhishek Agarwal
//
// SPDX-License-Identifier: Apache-2.0

#include "elimDeadSetInvalid.h"

#include <functional>
#include <map>
#include <vector>

#include "frontends/p4/methodInstance.h"

namespace P4 {

namespace {

bool isFieldOf(const IR::Member *m) {
    auto *type = m->expr->type;
    if (!type->is<IR::Type_Struct>() && !type->is<IR::Type_HeaderUnion>()) return false;
    return type->to<IR::Type_StructLike>()->getField(m->member.name) != nullptr;
}

// The local at the root of a path of struct or union fields and constant indexes
const IR::Declaration_Variable *rootLocal(const IR::Expression *e, ReferenceMap *refMap) {
    while (true) {
        if (auto *pe = e->to<IR::PathExpression>())
            return refMap->getDeclaration(pe->path, true)->to<IR::Declaration_Variable>();
        if (auto *m = e->to<IR::Member>()) {
            if (!isFieldOf(m)) return nullptr;
            e = m->expr;
        } else if (auto *ai = e->to<IR::ArrayIndex>()) {
            if (!ai->right->is<IR::Constant>()) return nullptr;
            e = ai->left;
        } else {
            return nullptr;
        }
    }
}

// True if some state of the parser can be reached again from itself
bool hasLoop(const IR::P4Parser *parser, ReferenceMap *refMap) {
    std::map<const IR::ParserState *, std::vector<const IR::ParserState *>> next;
    for (auto *state : parser->states) {
        if (!state->selectExpression) continue;
        forAllMatching<IR::PathExpression>(
            state->selectExpression, [&](const IR::PathExpression *pe) {
                auto *d = refMap->getDeclaration(pe->path);
                if (auto *to = d ? d->to<IR::ParserState>() : nullptr) next[state].push_back(to);
            });
    }
    std::set<const IR::ParserState *> done, open;
    std::function<bool(const IR::ParserState *)> reachesOpen = [&](const IR::ParserState *s) {
        if (open.count(s)) return true;
        if (!done.insert(s).second) return false;
        open.insert(s);
        for (auto *n : next[s])
            if (reachesOpen(n)) return true;
        open.erase(s);
        return false;
    };
    for (auto *state : parser->states)
        if (reachesOpen(state)) return true;
    return false;
}

}  // namespace

Visitor::profile_t FindUnsafeLocals::init_apply(const IR::Node *node) {
    unsafe.clear();
    return Inspector::init_apply(node);
}

void FindUnsafeLocals::postorder(const IR::PathExpression *pe) {
    auto *decl = refMap->getDeclaration(pe->path);
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
            if (auto *mce = findContext<IR::MethodCallExpression>()) {
                auto *mi = MethodInstance::resolve(mce, refMap, typeMap);
                auto *param = mi->substitution.findParameter(arg);
                if (param && param->direction == IR::Direction::Out) return;
            }
        }
    }
    LOG3("Unsafe local " << decl->getName());
    unsafe.insert(decl);
}

void FindUnsafeLocals::postorder(const IR::P4Parser *parser) {
    if (!hasLoop(parser, refMap)) return;
    LOG3("Parser " << parser->name << " has a loop, all its locals are unsafe");
    forAllMatching<IR::Declaration_Variable>(
        parser, [&](const IR::Declaration_Variable *v) { unsafe.insert(v); });
}

const IR::Node *DoElimDeadSetInvalid::postorder(IR::MethodCallStatement *mcs) {
    auto *call = getOriginal<IR::MethodCallStatement>()->methodCall;
    auto *bim = MethodInstance::resolve(call, refMap, typeMap)->to<BuiltInMethod>();
    if (!bim || bim->name != IR::Type_Header::setInvalid) return mcs;
    auto *decl = rootLocal(bim->appliedTo, refMap);
    if (!decl || unsafe.count(decl) || !defuse->getUses(call).empty()) return mcs;
    LOG3("Removing dead " << call);
    if (getParent<IR::IndexedVector<IR::StatOrDecl>>()) return nullptr;
    auto *orig = getOriginal();
    if (auto *ifs = getParent<IR::IfStatement>())
        if (ifs->ifFalse == orig && ifs->ifTrue != orig) return nullptr;
    return new IR::EmptyStatement(mcs->srcInfo);
}

}  // namespace P4
