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

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/error_catalog.h"
#include "lib/log.h"
#include "sideEffects.h"

namespace P4 {

Visitor::profile_t RemoveUnusedDeclarations::init_apply(const IR::Node *node) {
    LOG4("Reference map " << refMap);
    return Transform::init_apply(node);
}

bool RemoveUnusedDeclarations::giveWarning(const IR::Node *node) {
    if (warned == nullptr) return false;
    auto p = warned->emplace(node);
    LOG3("Warn about " << dbp(node) << " " << p.second);
    return p.second;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::Type_Enum *type) {
    prune();  // never remove individual enum members
    if (!refMap->isUsed(getOriginal<IR::Type_Enum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::Type_SerEnum *type) {
    prune();  // never remove individual enum members
    if (!refMap->isUsed(getOriginal<IR::Type_SerEnum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::P4Control *cont) {
    auto orig = getOriginal<IR::P4Control>();
    if (!refMap->isUsed(orig)) {
        LOG3("Removing " << cont << dbp(orig));
        prune();
        return nullptr;
    }

    visit(cont->controlLocals, "controlLocals");
    visit(cont->body);
    prune();
    return cont;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::P4Parser *parser) {
    auto orig = getOriginal<IR::P4Parser>();
    if (!refMap->isUsed(orig)) {
        LOG3("Removing " << parser << dbp(orig));
        prune();
        return nullptr;
    }

    visit(parser->parserLocals, "parserLocals");
    visit(parser->states, "states");
    prune();
    return parser;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::P4Table *table) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>())) {
        if (giveWarning(getOriginal()))
            warn(ErrorType::WARN_UNUSED, "Table %1% is not used; removing", table);
        LOG3("Removing " << table);
        table = nullptr;
    }
    prune();
    return table;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::Declaration_Variable *decl) {
    prune();
    if (decl->initializer == nullptr) return process(decl);
    if (!SideEffects::check(decl->initializer, this, nullptr, nullptr)) return process(decl);
    return decl;
}

const IR::Node *RemoveUnusedDeclarations::process(const IR::IDeclaration *decl) {
    LOG3("Visiting " << decl);
    if (decl->getName().name == IR::ParserState::verify && getParent<IR::P4Program>())
        return decl->getNode();
    if (decl->getName().name.startsWith("__"))
        // Internal identifiers, e.g., __v1model_version
        return decl->getNode();
    if (refMap->isUsed(getOriginal<IR::IDeclaration>())) return decl->getNode();
    LOG3("Removing " << getOriginal());
    prune();  // no need to go deeper
    return nullptr;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::Parameter *param) {
    // Skip all things that just declare "prototypes"
    if (findContext<IR::Type_Parser>() && !findContext<IR::P4Parser>()) return param;
    if (findContext<IR::Type_Control>() && !findContext<IR::P4Control>()) return param;
    if (findContext<IR::Type_Package>()) return param;
    if (findContext<IR::Type_Method>() && !findContext<IR::Function>()) return param;
    return warnIfUnused(param);
}

const IR::Node *RemoveUnusedDeclarations::warnIfUnused(const IR::Node *node) {
    if (!refMap->isUsed(getOriginal<IR::IDeclaration>()))
        if (giveWarning(getOriginal())) warn(ErrorType::WARN_UNUSED, "'%1%' is unused", node);
    return node;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::Declaration_Instance *decl) {
    // Don't delete instances; they may have consequences on the control-plane API
    if (decl->getName().name == IR::P4Program::main && getParent<IR::P4Program>()) return decl;
    if (!refMap->isUsed(getOriginal<IR::Declaration_Instance>())) {
        if (giveWarning(getOriginal())) warn(ErrorType::WARN_UNUSED, "%1%: unused instance", decl);
        // We won't delete extern instances; these may be useful even if not references.
        auto type = decl->type;
        if (type->is<IR::Type_Specialized>()) type = type->to<IR::Type_Specialized>()->baseType;
        if (type->is<IR::Type_Name>())
            type = refMap->getDeclaration(type->to<IR::Type_Name>()->path, true)->to<IR::Type>();
        if (!type->is<IR::Type_Extern>()) return process(decl);
        prune();
        return decl;
    }
    // don't scan the initializer: we don't want to delete virtual methods
    prune();
    return decl;
}

const IR::Node *RemoveUnusedDeclarations::preorder(IR::ParserState *state) {
    if (state->name == IR::ParserState::accept || state->name == IR::ParserState::reject ||
        state->name == IR::ParserState::start)
        return state;

    if (refMap->isUsed(getOriginal<IR::ParserState>())) return state;
    LOG3("Removing " << state);
    prune();
    return nullptr;
}

// extern functions declared in files should be kept,
// even if it is not referenced in the user program. Compiler
// backend may synthesize code to use the extern functions.
const IR::Node *RemoveUnusedDeclarations::preorder(IR::Method *method) { return method; }

}  // namespace P4
