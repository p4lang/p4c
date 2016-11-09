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

#include "resolveReferences.h"
#include <sstream>

namespace P4 {

std::vector<const IR::IDeclaration*>*
ResolutionContext::resolve(IR::ID name, P4::ResolutionType type, bool previousOnly) const {
    static std::vector<const IR::IDeclaration*> empty;

    std::vector<const IR::INamespace*> toTry(stack);
    toTry.insert(toTry.end(), globals.begin(), globals.end());

    for (auto it = toTry.rbegin(); it != toTry.rend(); ++it) {
        const IR::INamespace* current = *it;
        LOG2("Trying to resolve in " << current->toString());

        if (current->is<IR::IGeneralNamespace>()) {
            auto gen = current->to<IR::IGeneralNamespace>();
            Util::Enumerator<const IR::IDeclaration*>* decls = gen->getDeclsByName(name);
            switch (type) {
                case P4::ResolutionType::Any:
                    break;
                case P4::ResolutionType::Type: {
                    std::function<bool(const IR::IDeclaration*)> kindFilter =
                            [](const IR::IDeclaration* d) {
                        return d->is<IR::Type>();
                    };
                    decls = decls->where(kindFilter);
                    break;
                }
                case P4::ResolutionType::TypeVariable: {
                    std::function<bool(const IR::IDeclaration*)> kindFilter =
                            [](const IR::IDeclaration* d) {
                    return d->is<IR::Type_Var>(); };
                    decls = decls->where(kindFilter);
                    break;
                }
            default:
                BUG("Unexpected enumeration value %1%", static_cast<int>(type));
            }

            if (previousOnly) {
                std::function<bool(const IR::IDeclaration*)> locationFilter =
                        [name](const IR::IDeclaration* d) {
                    Util::SourceInfo nsi = name.srcInfo;
                    Util::SourceInfo dsi = d->getNode()->srcInfo;
                    bool before = dsi <= nsi;
                    LOG2("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
                    return before;
                };
                decls = decls->where(locationFilter);
            }

            auto vector = decls->toVector();
            if (!vector->empty()) {
                LOG2("Resolved in " << dbp(current->getNode()));
                return vector;
            } else {
                continue;
            }
        } else {
            auto simple = current->to<IR::ISimpleNamespace>();
            auto decl = simple->getDeclByName(name);
            if (decl == nullptr)
                continue;
            switch (type) {
                case P4::ResolutionType::Any:
                    break;
                case P4::ResolutionType::Type: {
                    if (!decl->is<IR::Type>())
                        continue;
                    break;
                }
                case P4::ResolutionType::TypeVariable: {
                    if (!decl->is<IR::Type_Var>())
                        continue;
                    break;
                }
            default:
                BUG("Unexpected enumeration value %1%", static_cast<int>(type));
            }

            if (previousOnly) {
                Util::SourceInfo nsi = name.srcInfo;
                Util::SourceInfo dsi = decl->getNode()->srcInfo;
                bool before = dsi <= nsi;
                LOG2("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
                if (!before)
                    continue;
            }

            LOG2("Resolved in " << dbp(current->getNode()));
            auto result = new std::vector<const IR::IDeclaration*>();
            result->push_back(decl);
            return result;
        }
    }

    return &empty;
}

void ResolutionContext::done() {
    pop(rootNamespace);
    BUG_CHECK(stack.empty(), "ResolutionContext::stack not empty");
}

const IR::IDeclaration*
ResolutionContext::resolveUnique(IR::ID name,
                                 P4::ResolutionType type,
                                 bool previousOnly) const {
    auto decls = resolve(name, type, previousOnly);
    if (decls->empty()) {
        ::error("Could not find declaration for %1%", name);
        return nullptr;
    }
    if (decls->size() > 1) {
        ::error("Multiple matching declarations for %1%", name);
        for (auto a : *decls)
            ::error("Candidate: %1%", a);
        return nullptr;
    }
    return decls->at(0);
}

void ResolutionContext::dbprint(std::ostream& out) const {
    out << "Context stack[" << stack.size() << "]" << std::endl;
    for (auto it = stack.begin(); it != stack.end(); it++) {
        const IR::INamespace* ns = *it;
        const IR::Node* node = ns->getNode();
        node->dbprint(out);
        out << std::endl;
    }
    out << "Globals[" << stack.size() << "]" << std::endl;
    for (auto it = globals.begin(); it != globals.end(); it++) {
        const IR::INamespace* ns = *it;
        const IR::Node* node = ns->getNode();
        node->dbprint(out);
        out << std::endl;
    }
    out << "----------" << std::endl;
}

/////////////////////////////////////////////////////

ResolveReferences::ResolveReferences(ReferenceMap* refMap,
                                     bool checkShadow) :
        refMap(refMap),
        context(nullptr),
        rootNamespace(nullptr),
        anyOrder(false),
        checkShadow(checkShadow) {
    CHECK_NULL(refMap);
    setName("ResolveReferences");
    visitDagOnce = false;
}

void ResolveReferences::addToContext(const IR::INamespace* ns) {
    LOG1("Adding to context " << dbp(ns));
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->push(ns);
}

void ResolveReferences::addToGlobals(const IR::INamespace* ns) {
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->addGlobal(ns);
}

void ResolveReferences::removeFromContext(const IR::INamespace* ns) {
    LOG1("Removing from context " << dbp(ns));
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->pop(ns);
}

void ResolveReferences::resolvePath(const IR::Path* path, bool isType) const {
    LOG1("Resolving " << path << " " << (isType ? "as type" : "as identifier"));
    ResolutionContext* ctx = context;
    if (path->absolute)
        ctx = new ResolutionContext(rootNamespace);
    ResolutionType k = isType ? ResolutionType::Type : ResolutionType::Any;

    BUG_CHECK(!resolveForward.empty(), "Empty resolveForward");
    bool allowForward = resolveForward.back();

    const IR::IDeclaration* decl = ctx->resolveUnique(path->name, k, !allowForward);
    if (decl == nullptr) {
        refMap->usedName(path->name.name);
        return;
    }

    refMap->setDeclaration(path, decl);
}

void ResolveReferences::checkShadowing(const IR::INamespace*ns) const {
    if (!checkShadow) return;

    auto e = ns->getDeclarations();
    while (e->moveNext()) {
        const IR::IDeclaration* decl = e->getCurrent();
        const IR::Node* node = decl->getNode();
        auto prev = context->resolve(decl->getName(), ResolutionType::Any, !anyOrder);
        if (prev->empty()) continue;
        for (auto p : *prev) {
            const IR::Node* pnode = p->getNode();
            if (pnode == node) continue;
            if (pnode->is<IR::Type_Method>() && node->is<IR::Type_Method>()) {
                auto md = node->to<IR::Type_Method>();
                auto mp = pnode->to<IR::Type_Method>();
                if (md->parameters->size() != mp->parameters->size())
                    continue;
            }
            ::warning("%1% shadows %2%", decl->getName(), p->getName());
        }
    }
}

Visitor::profile_t ResolveReferences::init_apply(const IR::Node* node) {
    anyOrder = refMap->isV1();
    if (!refMap->checkMap(node))
        refMap->clear();
    return Inspector::init_apply(node);
}

void ResolveReferences::end_apply(const IR::Node* node) {
    refMap->updateMap(node);
}

/////////////////// visitor methods ////////////////////////

// visitor should be invoked here
bool ResolveReferences::preorder(const IR::P4Program* program) {
    if (refMap->checkMap(program))
        return false;

    BUG_CHECK(resolveForward.empty(), "Expected empty resolvePath");
    resolveForward.push_back(anyOrder);
    BUG_CHECK(rootNamespace == nullptr, "Root namespace already set");
    rootNamespace = program;
    context = new ResolutionContext(rootNamespace);
    return true;
}

void ResolveReferences::postorder(const IR::P4Program*) {
    rootNamespace = nullptr;
    context->done();
    resolveForward.pop_back();
    BUG_CHECK(resolveForward.empty(), "Expected empty resolvePath");
    context = nullptr;
    LOG1("Reference map " << refMap);
}

bool ResolveReferences::preorder(const IR::PathExpression* path) {
    resolvePath(path->path, false); return true; }

bool ResolveReferences::preorder(const IR::Type_Name* type) {
    resolvePath(type->path, true); return true; }

bool ResolveReferences::preorder(const IR::P4Control *c) {
    refMap->usedName(c->name.name);
    addToContext(c);
    addToContext(c->type->typeParameters);
    addToContext(c->type->applyParams);
    addToContext(c->constructorParams);
    return true;
}

void ResolveReferences::postorder(const IR::P4Control *c) {
    removeFromContext(c->constructorParams);
    removeFromContext(c->type->applyParams);
    removeFromContext(c->type->typeParameters);
    removeFromContext(c);
    checkShadowing(c);
}

bool ResolveReferences::preorder(const IR::P4Parser *c) {
    refMap->usedName(c->name.name);
    addToContext(c);
    addToContext(c->type->typeParameters);
    addToContext(c->type->applyParams);
    addToContext(c->constructorParams);
    return true;
}

void ResolveReferences::postorder(const IR::P4Parser *c) {
    removeFromContext(c->constructorParams);
    removeFromContext(c->type->applyParams);
    removeFromContext(c->type->typeParameters);
    removeFromContext(c);
    checkShadowing(c);
}

bool ResolveReferences::preorder(const IR::Function* function) {
    refMap->usedName(function->name.name);
    addToContext(function->type->parameters);
    return true;
}

void ResolveReferences::postorder(const IR::Function* function) {
    removeFromContext(function->type->parameters);
}

bool ResolveReferences::preorder(const IR::P4Table* t) {
    refMap->usedName(t->name.name);
    addToContext(t->parameters);
    return true;
}

void ResolveReferences::postorder(const IR::P4Table* t) {
    removeFromContext(t->parameters);
}

bool ResolveReferences::preorder(const IR::TableProperties *p) {
    addToContext(p);
    return true;
}

void ResolveReferences::postorder(const IR::TableProperties *p) {
    removeFromContext(p);
}

bool ResolveReferences::preorder(const IR::P4Action *c) {
    refMap->usedName(c->name.name);
    addToContext(c);
    addToContext(c->parameters);
    return true;
}

void ResolveReferences::postorder(const IR::P4Action *c) {
    removeFromContext(c->parameters);
    removeFromContext(c);
    checkShadowing(c);
}

bool ResolveReferences::preorder(const IR::Type_Method *t) {
    // Function return values in generic functions may depend on the type arguments:
    // T f<T>()
    // where T is declared *after* its first use
    resolveForward.push_back(true);
    if (t->typeParameters != nullptr)
        addToContext(t->typeParameters);
    addToContext(t->parameters);
    return true;
}

void ResolveReferences::postorder(const IR::Type_Method *t) {
    removeFromContext(t->parameters);
    if (t->typeParameters != nullptr)
        removeFromContext(t->typeParameters);
    resolveForward.pop_back();
}

bool ResolveReferences::preorder(const IR::Type_Extern *t) {
    refMap->usedName(t->name.name);
    addToContext(t->typeParameters); return true; }

void ResolveReferences::postorder(const IR::Type_Extern *t) {
    removeFromContext(t->typeParameters); }

bool ResolveReferences::preorder(const IR::ParserState *s) {
    refMap->usedName(s->name.name);
    // State references may be resolved forward
    resolveForward.push_back(true);
    addToContext(s);
    return true;
}

void ResolveReferences::postorder(const IR::ParserState *s) {
    removeFromContext(s);
    resolveForward.pop_back();
    checkShadowing(s);
}

bool ResolveReferences::preorder(const IR::Declaration_MatchKind *d)
{ addToGlobals(d); return true; }

bool ResolveReferences::preorder(const IR::Type_ArchBlock *t) {
    resolveForward.push_back(anyOrder);
    addToContext(t->typeParameters);
    return true;
}

void ResolveReferences::postorder(const IR::Type_ArchBlock *t) {
    refMap->usedName(t->name.name);
    removeFromContext(t->typeParameters);
    resolveForward.pop_back();
}

bool ResolveReferences::preorder(const IR::Type_StructLike *t)
{ refMap->usedName(t->name.name); addToContext(t); return true; }

void ResolveReferences::postorder(const IR::Type_StructLike *t)
{ removeFromContext(t); }

bool ResolveReferences::preorder(const IR::BlockStatement *b)
{ addToContext(b); return true; }

void ResolveReferences::postorder(const IR::BlockStatement *b)
{ removeFromContext(b); checkShadowing(b); }

bool ResolveReferences::preorder(const IR::Declaration_Instance *decl) {
    refMap->usedName(decl->name.name);
    if (decl->initializer != nullptr)
        addToContext(decl->initializer);
    return true;
}

void ResolveReferences::postorder(const IR::Declaration_Instance *decl) {
    if (decl->initializer != nullptr)
        removeFromContext(decl->initializer);
}

#undef PROCESS_NAMESPACE

}  // namespace P4
