/*
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

#include "typeChecker.h"

namespace p4c::P4 {

using namespace literals;

const IR::Node *TypeInference::postorder(IR::P4Table *table) {
    currentActionList = nullptr;
    if (done()) return table;
    auto type = new IR::Type_Table(table);
    setType(getOriginal(), type);
    setType(table, type);
    return table;
}

bool TypeInference::checkParameters(const IR::ParameterList *paramList, bool forbidModules,
                                    bool forbidPackage) const {
    for (auto p : paramList->parameters) {
        auto type = getType(p);
        if (type == nullptr) return false;
        if (auto ts = type->to<IR::Type_SpecializedCanonical>()) type = ts->baseType;
        if (forbidPackage && type->is<IR::Type_Package>()) {
            typeError("%1%: parameter cannot be a package", p);
            return false;
        }
        if (p->direction != IR::Direction::None &&
            (type->is<IR::Type_Extern>() || type->is<IR::Type_String>())) {
            typeError("%1%: a parameter with type %2% cannot have a direction", p, type);
            return false;
        }
        if ((forbidModules || p->direction != IR::Direction::None) &&
            (type->is<IR::Type_Parser>() || type->is<IR::Type_Control>() ||
             type->is<IR::P4Parser>() || type->is<IR::P4Control>())) {
            typeError("%1%: parameter cannot have type %2%", p, type);
            return false;
        }
    }
    return true;
}

const IR::ParameterList *TypeInference::canonicalizeParameters(const IR::ParameterList *params) {
    if (params == nullptr) return params;

    bool changes = false;
    auto vec = new IR::IndexedVector<IR::Parameter>();
    for (auto p : *params->getEnumerator()) {
        auto paramType = getTypeType(p->type);
        if (paramType == nullptr) return nullptr;
        BUG_CHECK(!paramType->is<IR::Type_Type>(), "%1%: Unexpected parameter type", paramType);
        if (paramType != p->type) {
            p = new IR::Parameter(p->srcInfo, p->name, p->annotations, p->direction, paramType,
                                  p->defaultValue);
            changes = true;
        }
        setType(p, paramType);
        vec->push_back(p);
    }
    if (changes)
        return new IR::ParameterList(params->srcInfo, *vec);
    else
        return params;
}

const IR::Node *TypeInference::postorder(IR::P4Action *action) {
    if (done()) return action;
    auto pl = canonicalizeParameters(action->parameters);
    if (pl == nullptr) return action;
    if (!checkParameters(action->parameters, forbidModules, forbidPackages)) return action;
    auto type = new IR::Type_Action(new IR::TypeParameters(), nullptr, pl);

    bool foundDirectionless = false;
    for (auto p : action->parameters->parameters) {
        auto ptype = getType(p);
        BUG_CHECK(ptype, "%1%: parameter type missing when it was found previously", p);
        if (ptype->is<IR::Type_Extern>())
            typeError("%1%: Action parameters cannot have extern types", p->type);
        if (p->direction == IR::Direction::None)
            foundDirectionless = true;
        else if (foundDirectionless)
            typeError("%1%: direction-less action parameters have to be at the end", p);
    }
    setType(getOriginal(), type);
    setType(action, type);
    return action;
}

const IR::Node *TypeInference::postorder(IR::Declaration_Variable *decl) {
    if (done()) return decl;
    auto type = getTypeType(decl->type);
    if (type == nullptr) return decl;

    if (const IR::IMayBeGenericType *gt = type->to<IR::IMayBeGenericType>()) {
        // Check that there are no unbound type parameters
        if (!gt->getTypeParameters()->empty()) {
            typeError("Unspecified type parameters for %1% in %2%", gt, decl);
            return decl;
        }
    }

    const IR::Type *baseType = type;
    if (auto sc = type->to<IR::Type_SpecializedCanonical>()) baseType = sc->baseType;
    if (baseType->is<IR::IContainer>() || baseType->is<IR::Type_Extern>() ||
        baseType->is<IR::Type_Parser>() || baseType->is<IR::Type_Control>()) {
        typeError("%1%: cannot declare variables of type '%2%' (consider using an instantiation)",
                  decl, type);
        return decl;
    }

    if (type->is<IR::Type_String>() || type->is<IR::Type_InfInt>()) {
        typeError("%1%: Cannot declare variables with type %2%", decl, type);
        return decl;
    }

    auto orig = getOriginal<IR::Declaration_Variable>();
    if (decl->initializer != nullptr) {
        auto init = assignment(decl, type, decl->initializer);
        if (decl->initializer != init) {
            auto declType = type->getP4Type();
            decl->type = declType;
            decl->initializer = init;
            LOG2("Created new declaration " << decl);
        }
    }
    setType(decl, type);
    setType(orig, type);
    return decl;
}

const IR::Node *TypeInference::postorder(IR::Declaration_Constant *decl) {
    if (done()) return decl;
    auto type = getTypeType(decl->type);
    if (type == nullptr) return decl;

    if (type->is<IR::Type_Extern>()) {
        typeError("%1%: Cannot declare constants of extern types", decl->name);
        return decl;
    }

    if (!isCompileTimeConstant(decl->initializer))
        typeError("%1%: Cannot evaluate initializer to a compile-time constant", decl->initializer);
    auto orig = getOriginal<IR::Declaration_Constant>();
    auto newInit = assignment(decl, type, decl->initializer);
    if (newInit != decl->initializer)
        decl = new IR::Declaration_Constant(decl->srcInfo, decl->name, decl->annotations,
                                            decl->type, newInit);
    setType(decl, type);
    setType(orig, type);
    return decl;
}

// Return true on success
bool TypeInference::checkAbstractMethods(const IR::Declaration_Instance *inst,
                                         const IR::Type_Extern *type) {
    // Make a list of the abstract methods
    IR::NameMap<IR::Method, ordered_map> virt;
    for (auto m : type->methods)
        if (m->isAbstract) virt.addUnique(m->name, m);
    if (virt.size() == 0 && inst->initializer == nullptr) return true;
    if (virt.size() == 0 && inst->initializer != nullptr) {
        typeError("%1%: instance initializers for extern without abstract methods",
                  inst->initializer);
        return false;
    } else if (virt.size() != 0 && inst->initializer == nullptr) {
        typeError("%1%: must declare abstract methods for %2%", inst, type);
        return false;
    }

    for (auto d : inst->initializer->components) {
        if (auto *func = d->to<IR::Function>()) {
            LOG2("Type checking " << func);
            if (func->type->typeParameters->size() != 0) {
                typeError("%1%: abstract method implementations cannot be generic", func);
                return false;
            }
            auto ftype = getType(func);
            if (virt.find(func->name.name) == virt.end()) {
                typeError("%1%: no matching abstract method in %2%", func, type);
                return false;
            }
            auto meth = virt[func->name.name];
            auto methtype = getType(meth);
            virt.erase(func->name.name);
            auto tvs =
                unify(inst, methtype, ftype, "Method '%1%' does not have the expected type '%2%'",
                      {func, methtype});
            if (tvs == nullptr) return false;
            BUG_CHECK(errorCount() > 0 || tvs->isIdentity(), "%1%: expected no type variables",
                      tvs);
        }
    }
    bool rv = true;
    for (auto &vm : virt) {
        if (!vm.second->annotations->getSingle("optional"_cs)) {
            typeError("%1%: %2% abstract method not implemented", inst, vm.second);
            rv = false;
        }
    }
    return rv;
}

const IR::Node *TypeInference::preorder(IR::Declaration_Instance *decl) {
    // We need to control the order of the type-checking: we want to do first
    // the declaration, and then typecheck the initializer if present.
    if (done()) return decl;
    visit(decl->type, "type");
    visit(decl->arguments, "arguments");
    visit(decl->annotations, "annotations");
    visit(decl->properties, "properties");

    auto type = getTypeType(decl->type);
    if (type == nullptr) {
        prune();
        return decl;
    }
    auto orig = getOriginal<IR::Declaration_Instance>();

    auto simpleType = type;
    if (auto *sc = type->to<IR::Type_SpecializedCanonical>()) simpleType = sc->substituted;

    if (auto et = simpleType->to<IR::Type_Extern>()) {
        auto [newType, newArgs] = checkExternConstructor(decl, et, decl->arguments);
        if (newArgs == nullptr) {
            prune();
            return decl;
        }
        // type can be Type_Extern or Type_SpecializedCanonical.  If it is already
        // specialized, the type arguments were specified explicitly.
        // Otherwise, we use the type received from checkExternConstructor, which
        // has substituted the type variables with fresh ones.
        if (type->is<IR::Type_Extern>()) type = newType;
        decl->arguments = newArgs;
        setType(orig, type);
        setType(decl, type);

        if (decl->initializer != nullptr) visit(decl->initializer);
        // This will need the decl type to be already known
        bool s = checkAbstractMethods(decl, et);
        if (!s) {
            prune();
            return decl;
        }
    } else if (simpleType->is<IR::IContainer>()) {
        if (decl->initializer != nullptr) {
            typeError("%1%: initializers only allowed for extern instances", decl->initializer);
            prune();
            return decl;
        }
        if (!simpleType->is<IR::Type_Package>() && (findContext<IR::IContainer>() == nullptr)) {
            ::p4c::error(ErrorType::ERR_INVALID, "%1%: cannot instantiate at top-level", decl);
            return decl;
        }
        auto typeAndArgs =
            containerInstantiation(decl, decl->arguments, simpleType->to<IR::IContainer>());
        auto type = typeAndArgs.first;
        auto args = typeAndArgs.second;
        if (type == nullptr || args == nullptr) {
            prune();
            return decl;
        }
        learn(type, this, getChildContext());
        if (args != decl->arguments) decl->arguments = args;
        setType(decl, type);
        setType(orig, type);
    } else {
        typeError("%1%: cannot allocate objects of type %2%", decl, type);
    }
    prune();
    return decl;
}

const IR::Node *TypeInference::preorder(IR::Function *function) {
    if (done()) return function;
    visit(function->type);
    auto type = getTypeType(function->type);
    if (type == nullptr) return function;
    setType(getOriginal(), type);
    setType(function, type);
    visit(function->body);
    prune();
    return function;
}

const IR::Node *TypeInference::postorder(IR::Method *method) {
    if (done()) return method;
    auto type = getTypeType(method->type);
    if (type == nullptr) return method;
    if (auto mtype = type->to<IR::Type_Method>()) {
        if (mtype->returnType) {
            if (auto gen = mtype->returnType->to<IR::IMayBeGenericType>()) {
                if (gen->getTypeParameters()->size() != 0) {
                    typeError("%1%: no type parameters supplied for return generic type",
                              method->type->returnType);
                    return method;
                }
            }
        }
    }
    setType(getOriginal(), type);
    setType(method, type);
    return method;
}

}  // namespace p4c::P4
