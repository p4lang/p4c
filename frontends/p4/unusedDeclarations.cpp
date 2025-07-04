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

UnusedDeclarations *RemoveUnusedPolicy::getUnusedDeclarationsPass(const UsedDeclSet &used,
                                                                  bool warn) const {
    return new UnusedDeclarations(used, warn);
}

Visitor::profile_t UnusedDeclarations::init_apply(const IR::Node *node) {
    LOG4("Used set " << used);
    return Transform::init_apply(node);
}

bool CollectUsedDeclarations::preorder(const IR::KeyElement *ke) {
    visit(ke->annotations, "annotations");
    visit(ke->expression, "expression");

    auto decls = lookupMatchKind(ke->matchType->path->name);
    BUG_CHECK(decls.size() == 1, "");
    used.setUsed(decls.front());

    return false;
}

bool CollectUsedDeclarations::preorder(const IR::PathExpression *path) {
    auto decl = resolvePath(path->path, false);
    if (decl) used.setUsed(decl);

    return true;
}

bool CollectUsedDeclarations::preorder(const IR::Type_Name *type) {
    auto decl = resolvePath(type->path, true);
    if (decl) used.setUsed(decl);

    return true;
}

void UsedDeclSet::dbprint(std::ostream &out) const {
    if (usedDecls.empty()) out << "Empty" << '\n';
    for (const auto *decl : usedDecls) out << dbp(decl) << '\n';
}

bool UnusedDeclarations::giveWarning(const IR::Node *node) {
    if (!warnOnly) return false;
    if (isSystemFile(node->srcInfo.getSourceFile())) return false;
    auto p = warned.emplace(node);
    LOG3("Warn about " << dbp(node) << " " << p.second);
    return p.second;
}

const IR::Node *UnusedDeclarations::preorder(IR::Type_Enum *type) {
    prune();  // never remove individual enum members
    if (!warnOnly && !used.isUsed(getOriginal<IR::Type_Enum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node *UnusedDeclarations::preorder(IR::Type_SerEnum *type) {
    prune();  // never remove individual enum members
    if (!warnOnly && !used.isUsed(getOriginal<IR::Type_SerEnum>())) {
        LOG3("Removing " << type);
        return nullptr;
    }
    return type;
}

const IR::Node *UnusedDeclarations::preorder(IR::P4Control *cont) {
    auto orig = getOriginal<IR::P4Control>();
    if (!used.isUsed(orig)) {
        if (giveWarning(orig))
            warn(ErrorType::WARN_UNUSED, "control '%2%' is unused", cont, cont->externalName());
        if (!warnOnly) {
            info(ErrorType::INFO_REMOVED, "removing control '%2%'", cont, cont->externalName());
            prune();
            return nullptr;
        }
    }
    visit(cont->controlLocals, "controlLocals");
    visit(cont->body);
    prune();
    return cont;
}

const IR::Node *UnusedDeclarations::preorder(IR::P4Parser *parser) {
    auto orig = getOriginal<IR::P4Parser>();
    if (!used.isUsed(orig)) {
        if (giveWarning(orig))
            warn(ErrorType::WARN_UNUSED, "parser '%2%' is unused", parser, parser->externalName());
        if (!warnOnly) {
            info(ErrorType::INFO_REMOVED, "removing parser '%2%'", parser, parser->externalName());
            prune();
            return nullptr;
        }
    }
    visit(parser->parserLocals, "parserLocals");
    visit(parser->states, "states");
    prune();
    return parser;
}

const IR::Node *UnusedDeclarations::preorder(IR::P4Table *table) {
    if (!used.isUsed(getOriginal<IR::IDeclaration>())) {
        if (giveWarning(getOriginal()))
            warn(ErrorType::WARN_UNUSED, "table '%1%' is unused", table);
        if (!warnOnly) {
            info(ErrorType::INFO_REMOVED, "removing table '%2%'", table, table->externalName());
            prune();
            return nullptr;
        }
    }
    prune();
    return table;
}

const IR::Node *UnusedDeclarations::preorder(IR::Declaration_Variable *decl) {
    prune();
    if (warnOnly) return process(decl);
    if (decl->initializer == nullptr) return process(decl);
    if (!SideEffects::check(decl->initializer, this, nullptr, nullptr)) return process(decl);
    return decl;
}

const IR::Node *UnusedDeclarations::process(const IR::IDeclaration *decl) {
    LOG3("Visiting " << decl);
    if (decl->getName().name == IR::ParserState::verify && getParent<IR::P4Program>())
        return decl->getNode();
    if (decl->externalName(decl->getName().name).startsWith("__"))
        // Internal identifiers, e.g., __v1model_version
        return decl->getNode();
    if (used.isUsed(getOriginal<IR::IDeclaration>())) return decl->getNode();
    if (giveWarning(getOriginal())) warn(ErrorType::WARN_UNUSED, "'%1%' is unused", decl);
    if (warnOnly) return decl->getNode();
    prune();  // no need to go deeper
    return nullptr;
}

const IR::Node *UnusedDeclarations::preorder(IR::Parameter *param) {
    // Skip all things that just declare "prototypes"
    if (isInContext<IR::Type_Parser>() && !isInContext<IR::P4Parser>()) return param;
    if (isInContext<IR::Type_Control>() && !isInContext<IR::P4Control>()) return param;
    if (isInContext<IR::Type_Package>()) return param;
    if (isInContext<IR::Type_Method>() && !isInContext<IR::Function>()) return param;
    LOG3("Visiting " << param);
    return warnIfUnused(param);
}

const IR::Node *UnusedDeclarations::warnIfUnused(const IR::Node *node) {
    if (!used.isUsed(getOriginal<IR::IDeclaration>())) {
        if (giveWarning(getOriginal())) {
            warn(ErrorType::WARN_UNUSED, "'%1%' is unused", node);
        }
    }
    return node;
}

const IR::Node *UnusedDeclarations::preorder(IR::Declaration_Instance *decl) {
    // Don't delete instances; they may have consequences on the control-plane API
    if (decl->getName().name == IR::P4Program::main && getParent<IR::P4Program>()) return decl;
    if (!used.isUsed(getOriginal<IR::Declaration_Instance>())) {
        if (giveWarning(getOriginal())) warn(ErrorType::WARN_UNUSED, "'%1%' is unused", decl);
        if (!warnOnly) {
            // We won't delete extern instances; these may be useful even if not references.
            auto type = decl->type;
            if (type->is<IR::Type_Specialized>()) type = type->to<IR::Type_Specialized>()->baseType;
            if (type->is<IR::Type_Name>())
                type = getDeclaration(type->to<IR::Type_Name>()->path, true)->to<IR::Type>();
            if (!type->is<IR::Type_Extern>()) return process(decl);
        }
    }
    // don't scan the initializer: we don't want to delete virtual methods
    prune();
    return decl;
}

const IR::Node *UnusedDeclarations::preorder(IR::ParserState *state) {
    if (state->name == IR::ParserState::accept || state->name == IR::ParserState::reject ||
        state->name == IR::ParserState::start)
        return state;

    if (warnOnly || used.isUsed(getOriginal<IR::ParserState>())) return state;
    LOG3("Removing " << state);
    prune();
    return nullptr;
}

// extern functions declared in files should be kept,
// even if it is not referenced in the user program. Compiler
// backend may synthesize code to use the extern functions.
const IR::Node *UnusedDeclarations::preorder(IR::Method *method) { return method; }

}  // namespace P4
