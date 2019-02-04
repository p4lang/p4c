/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "unusedDeclarations.h"
#include "sideEffects.h"

namespace P4 {

Visitor::profile_t RemoveUnusedDeclarations::init_apply(const IR::Node* node) {
    LOG4("Reference map " << refMap);
    return Transform::init_apply(node);
}

bool RemoveUnusedDeclarations::giveWarning(const IR::Node* node) {
    if (warned == nullptr)
        return false;
    auto p = warned->emplace(node);
    LOG3("Warn about " << dbp(node) << " " << p.second);
    return p.second;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Type_Enum* type) {
    prune();  // never remove individual enum members
    if (!refMap->isUsed(getOriginal<IR::Type_Enum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Type_SerEnum* type) {
    prune();  // never remove individual enum members
    if (!refMap->isUsed(getOriginal<IR::Type_SerEnum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Control* cont) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        LOG3("Removing " << cont);
        prune();
        return nullptr;
    }

    visit(cont->controlLocals, "controlLocals");
    visit(cont->body);
    prune();
    return cont;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Parser* cont) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        LOG3("Removing " << cont);
        prune();
        return nullptr;
    }

    visit(cont->parserLocals, "parserLocals");
    visit(cont->states, "states");
    prune();
    return cont;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::P4Table* table) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        if (giveWarning(getOriginal()))
            ::warning(ErrorType::WARN_UNUSED, "Table %1% is not used; removing", table);
        LOG3("Removing " << table);
        table = nullptr;
    }
    prune();
    return table;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Declaration_Variable* decl) {
    prune();
    if (decl->initializer == nullptr)
        return process(decl);
    if (!SideEffects::check(decl->initializer, nullptr, nullptr))
        return process(decl);
    return decl;
}

const IR::Node* RemoveUnusedDeclarations::process(const IR::IDeclaration* decl) {
    LOG3("Visiting " << decl);
    if (decl->getName().name == IR::ParserState::verify && getParent<IR::P4Program>())
        return decl->getNode();
    if (refMap->isUsed(getOriginal<IR::IDeclaration>()))
        return decl->getNode();
    LOG3("Removing " << getOriginal());
    prune();  // no need to go deeper
    return nullptr;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::Declaration_Instance* decl) {
    // Don't delete instances; they may have consequences on the control-plane API
    if (decl->getName().name == IR::P4Program::main && getParent<IR::P4Program>())
        return decl;
    if (!refMap->isUsed(getOriginal<IR::Declaration_Instance>())) {
        if (giveWarning(getOriginal()))
            ::warning(ErrorType::WARN_UNUSED, "%1%: unused instance", decl);
        // We won't delete extern instances; these may be useful even if not references.
        auto type = decl->type;
        if (type->is<IR::Type_Specialized>())
            type = type->to<IR::Type_Specialized>()->baseType;
        if (type->is<IR::Type_Name>())
            type = refMap->getDeclaration(type->to<IR::Type_Name>()->path, true)->to<IR::Type>();
        if (!type->is<IR::Type_Extern>())
            return process(decl);
        prune();
        return decl;
    }
    // don't scan the initializer: we don't want to delete virtual methods
    prune();
    return decl;
}

const IR::Node* RemoveUnusedDeclarations::preorder(IR::ParserState* state) {
    if (state->name == IR::ParserState::accept ||
        state->name == IR::ParserState::reject ||
        state->name == IR::ParserState::start)
        return state;

    if (refMap->isUsed(getOriginal<IR::ParserState>()))
        return state;
    LOG3("Removing " << state);
    prune();
    return nullptr;
}

}  // namespace P4
