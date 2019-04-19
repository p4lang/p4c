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
ResolutionContext::resolve(IR::ID name, P4::ResolutionType type, bool forwardOK) const {
    static std::vector<const IR::IDeclaration*> empty;

    std::vector<const IR::INamespace*> toTry(stack);
    toTry.insert(toTry.end(), globals.begin(), globals.end());

    for (auto it = toTry.rbegin(); it != toTry.rend(); ++it) {
        const IR::INamespace* current = *it;
        LOG3("Trying to resolve in " << current->toString());

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

            if (!forwardOK && name.srcInfo.isValid()) {
                std::function<bool(const IR::IDeclaration*)> locationFilter =
                        [name](const IR::IDeclaration* d) {
                    Util::SourceInfo nsi = name.srcInfo;
                    Util::SourceInfo dsi = d->getNode()->srcInfo;
                    bool before = dsi <= nsi;
                    LOG3("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
                    return before;
                };
                decls = decls->where(locationFilter);
            }

            auto vector = decls->toVector();
            if (!vector->empty()) {
                LOG3("Resolved in " << dbp(current->getNode()));
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

            if (!forwardOK && name.srcInfo.isValid()) {
                Util::SourceInfo nsi = name.srcInfo;
                Util::SourceInfo dsi = decl->getNode()->srcInfo;
                bool before = dsi <= nsi;
                LOG3("\tPosition test:" << dsi << "<=" << nsi << "=" << before);
                if (!before)
                    continue;
            }

            LOG3("Resolved in " << dbp(current->getNode()));
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
                                 bool forwardOK) const {
    const std::vector<const IR::IDeclaration*> *decls = resolve(name, type, forwardOK);
    // Check overloaded symbols.
    if (!argumentStack.empty() && decls->size() > 1) {
        auto arguments = argumentStack.back();
        decls = Util::Enumerator<const IR::IDeclaration*>::createEnumerator(*decls)->
                where([arguments](const IR::IDeclaration* d) {
                        auto func = d->to<IR::IFunctional>();
                        if (func == nullptr)
                            return true;
                        return func->callMatches(arguments); })->
                toVector();
    }

    if (decls->empty()) {
        ::error(ErrorType::ERR_NOT_FOUND, "declaration", name);
        return nullptr;
    }
    if (decls->size() == 1)
        return decls->at(0);

    ::error(ErrorType::ERR_INVALID, "multiple matching declarations", name);
    for (auto a : *decls)
        ::error("Candidate: %1%", a);
    return nullptr;
}

const IR::Type *
ResolutionContext::resolveType(const IR::Type *type) const {
    // We allow lookups forward for type variables, which are declared after they are used
    // in function returns.  forwardOK = true below.
    if (auto tname = type->to<IR::Type_Name>())
        return resolveUnique(tname->path->name, ResolutionType::Type, true)->to<IR::Type>();
    return type;
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
    LOG2("Adding to context " << dbp(ns));
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    checkShadowing(ns);
    context->push(ns);
}

void ResolveReferences::addToGlobals(const IR::INamespace* ns) {
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->addGlobal(ns);
}

void ResolveReferences::removeFromContext(const IR::INamespace* ns) {
    LOG2("Removing from context " << dbp(ns));
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->pop(ns);
}

void ResolveReferences::resolvePath(const IR::Path* path, bool isType) const {
    LOG2("Resolving " << path << " " << (isType ? "as type" : "as identifier"));
    ResolutionContext* ctx = context;
    if (path->absolute)
        ctx = new ResolutionContext(rootNamespace);
    ResolutionType k = isType ? ResolutionType::Type : ResolutionType::Any;

    BUG_CHECK(!resolveForward.empty(), "Empty resolveForward");
    bool forwardOK = resolveForward.back();

    const IR::IDeclaration* decl = ctx->resolveUnique(path->name, k, forwardOK);
    if (decl == nullptr) {
        refMap->usedName(path->name.name);
        return;
    }

    refMap->setDeclaration(path, decl);
}

void ResolveReferences::checkShadowing(const IR::INamespace* ns) const {
    if (!checkShadow) return;
    for (auto decl : *ns->getDeclarations()) {
        const IR::Node* node = decl->getNode();
        if (node->is<IR::StructField>())
            continue;

        auto prev = context->resolve(decl->getName(), ResolutionType::Any, anyOrder);
        if (prev->empty()) continue;

        for (auto p : *prev) {
            const IR::Node* pnode = p->getNode();
            if (pnode == node) continue;
            if ((pnode->is<IR::Method>() || pnode->is<IR::Type_Extern>() ||
                 pnode->is<IR::P4Program>()) &&
                (node->is<IR::Method>() || node->is<IR::Function>() ||
                 node->is<IR::P4Control>() || node->is<IR::P4Parser>() ||
                 node->is<IR::Type_Package>()))
                // These can overload each other.
                // Also, the constructor is supposed to have the same name as the class.
                continue;
            if (pnode->is<IR::Attribute>() && node->is<IR::AttribLocal>())
                // attribute locals often match attributes
                continue;

            ::warning(ErrorType::WARN_SHADOWING, "%1% shadows %2%", node, pnode);
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

// Visitor methods

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
    LOG2("Reference map " << refMap);
}

bool ResolveReferences::preorder(const IR::This* pointer) {
    auto decl = findContext<IR::Declaration_Instance>();
    if (findContext<IR::Function>() == nullptr || decl == nullptr)
        ::error(ErrorType::ERR_INVALID,
                "%1% can only be used in the definition of an abstract method", pointer);
    refMap->setDeclaration(pointer, decl);
    return true;
}

bool ResolveReferences::preorder(const IR::MethodCallExpression* call) {
    LOG2("Adding to context " << dbp(call));
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->enterMethodCall(call->arguments);
    visit(call->method);
    LOG2("Removing from context " << dbp(call));
    context->exitMethodCall();
    visit(call->typeArguments);
    visit(call->arguments);
    return false;
}

bool ResolveReferences::preorder(const IR::PathExpression* path) {
    resolvePath(path->path, false); return true; }

bool ResolveReferences::preorder(const IR::Type_Name* type) {
    resolvePath(type->path, true); return true; }

bool ResolveReferences::preorder(const IR::P4Control *c) {
    refMap->usedName(c->name.name);
    addToContext(c->getTypeParameters());
    addToContext(c->getApplyParameters());
    addToContext(c->getConstructorParameters());
    addToContext(c);  // add the locals
    return true;
}

void ResolveReferences::postorder(const IR::P4Control *c) {
    removeFromContext(c);
    removeFromContext(c->getConstructorParameters());
    removeFromContext(c->getApplyParameters());
    removeFromContext(c->getTypeParameters());
}

bool ResolveReferences::preorder(const IR::P4Parser *p) {
    refMap->usedName(p->name.name);
    addToContext(p->getTypeParameters());
    addToContext(p->getApplyParameters());
    addToContext(p->getConstructorParameters());
    addToContext(p);
    return true;
}

void ResolveReferences::postorder(const IR::P4Parser *p) {
    removeFromContext(p);
    removeFromContext(p->getConstructorParameters());
    removeFromContext(p->getApplyParameters());
    removeFromContext(p->getTypeParameters());
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
    return true;
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
    addToContext(c->parameters);
    addToContext(c);
    return true;
}

void ResolveReferences::postorder(const IR::P4Action *c) {
    removeFromContext(c);
    removeFromContext(c->parameters);
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
    // FIXME -- should the typeParamters be part of the extern's scope?
    addToContext(t->typeParameters);
    addToContext(t);
    return true; }

void ResolveReferences::postorder(const IR::Type_Extern *t) {
    removeFromContext(t);
    removeFromContext(t->typeParameters);
    }

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
{ removeFromContext(b); }

bool ResolveReferences::preorder(const IR::Declaration_Instance *decl) {
    visit(decl->annotations);
    refMap->usedName(decl->name.name);

    // This looks a lot like a method call
    BUG_CHECK(context != nullptr, "No resolution context; did not start at P4Program?");
    context->enterMethodCall(decl->arguments);
    visit(decl->type);
    context->exitMethodCall();
    visit(decl->arguments);
    visit(decl->properties);
    if (decl->initializer)
        visit(decl->initializer);
    return false;
}

#undef PROCESS_NAMESPACE

}  // namespace P4
