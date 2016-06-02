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

#include "typeChecker.h"
#include "typeUnification.h"
#include "ir/substitution.h"
#include "typeConstraints.h"
#include "ir/dump.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

TypeChecking::TypeChecking(ReferenceMap* refMap, TypeMap* typeMap,
                           bool clearMap, bool isv1, bool updateExpressions) {
    addPasses({
       new P4::ResolveReferences(refMap, isv1),
       new P4::TypeInference(refMap, typeMap, clearMap, true),
       updateExpressions ? new ApplyTypesToExpressions(typeMap) : nullptr,
       updateExpressions ? new P4::ResolveReferences(refMap, isv1) : nullptr });
    setName("TypeChecking");
    setStopOnError(true);
}

// Used to set the type of Constants after type inference
class ConstantTypeSubstitution : public Transform {
    IR::TypeVariableSubstitution* subst;
    TypeMap                     * typeMap;

 public:
    ConstantTypeSubstitution(IR::TypeVariableSubstitution* subst,
                             TypeMap* typeMap) : subst(subst), typeMap(typeMap)
    { CHECK_NULL(subst); CHECK_NULL(typeMap); setName("ConstantTypeSubstitution"); }
    // Expressions may be created anew
    const IR::Node* postorder(IR::Expression* expression) override {
        auto type = typeMap->getType(getOriginal(), true);
        if (typeMap->isCompileTimeConstant(getOriginal<IR::Expression>()))
            typeMap->setCompileTimeConstant(expression);
        typeMap->setType(expression, type);
        return expression;
    }
    const IR::Node* postorder(IR::Constant* cst) override {
        auto cstType = typeMap->getType(getOriginal(), true);
        if (!cstType->is<IR::ITypeVar>())
            return cst;
        auto repl = subst->get(cstType->to<IR::ITypeVar>());
        if (repl != nullptr && !repl->is<IR::ITypeVar>()) {
            // maybe the substitution could not infer a width...
            LOG1("Inferred type " << repl << " for " << cst);
            cst = new IR::Constant(cst->srcInfo, repl, cst->value, cst->base);
            typeMap->setType(cst, repl);
            typeMap->setCompileTimeConstant(cst);
        }
        return cst;
    }

    const IR::Expression* convert(const IR::Expression* expr)
    { return expr->apply(*this)->to<IR::Expression>(); }
};

TypeInference::TypeInference(ReferenceMap* refMap, TypeMap* typeMap, bool clearMap, bool readOnly) :
        refMap(refMap), typeMap(typeMap), clearMap(clearMap), readOnly(readOnly),
        initialNode(nullptr) {
    CHECK_NULL(typeMap);
    CHECK_NULL(refMap);
    visitDagOnce = false;  // the done() method will take care of this
    setName("TypeInference");
}

Visitor::profile_t TypeInference::init_apply(const IR::Node* node) {
    if (node->is<IR::P4Program>()) {
        LOG2("Reference map for type checker:" << std::endl << refMap);
        LOG1("TypeInference for " << dbp(node));
    }
    initialNode = node;
    refMap->validateMap(node);
    if (clearMap) {
        // Clear map only if program has not changed from last time
        // oterwise we can reuse it
        if (!typeMap->checkMap(node))
            typeMap->clear();
    }
    return Transform::init_apply(node);
}

void TypeInference::end_apply(const IR::Node* node) {
    if (readOnly && !(*node == *initialNode))
        BUG("%1%: typechecker mutated node", node);
    typeMap->updateMap(node);
    if (node->is<IR::P4Program>())
        LOG2("Typemap: " << std::endl << typeMap);
}

bool TypeInference::done() const {
    auto orig = getOriginal();
    bool done = typeMap->contains(orig);
    LOG3("Visiting " << dbp(orig) << (done ? " done" : ""));
    return done;
}

const IR::Type* TypeInference::getType(const IR::Node* element) const {
    const IR::Type* result = typeMap->getType(element);
    if (result == nullptr) {
        typeError("Could not find type of %1%", element);
        return nullptr;
    }
    return result;
}

void TypeInference::setType(const IR::Node* element, const IR::Type* type) {
    typeMap->setType(element, type);
    // mark each canonical type as pointing to itself.
    if (!typeMap->contains(type))
        typeMap->setType(type, type);
}

IR::TypeVariableSubstitution* TypeInference::unify(const IR::Node* errorPosition,
                                                   const IR::Type* destType,
                                                   const IR::Type* srcType,
                                                   bool reportErrors) const {
    CHECK_NULL(destType); CHECK_NULL(srcType);
    if (srcType == destType)
        return new IR::TypeVariableSubstitution();

    TypeConstraints constraints;
    constraints.addEqualityConstraint(destType, srcType);
    auto result = constraints.solve(errorPosition, reportErrors);
    return result;
}

const IR::IndexedVector<IR::StructField>*
TypeInference::canonicalizeFields(const IR::Type_StructLike* type) {
    bool changes = false;
    auto fields = new IR::IndexedVector<IR::StructField>();
    for (auto field : *type->fields) {
        auto ftype = canonicalize(field->type);
        if (ftype != field->type)
            changes = true;
        auto newField = new IR::StructField(field->srcInfo, field->name, field->annotations, ftype);
        fields->push_back(newField);
    }
    if (changes)
        return fields;
    else
        return type->fields;
}

const IR::ParameterList* TypeInference::canonicalize(const IR::ParameterList* params) {
    if (params == nullptr)
        return params;

    bool changes = false;
    auto vec = new IR::IndexedVector<IR::Parameter>();
    for (auto p : *params->getEnumerator()) {
        auto paramType = canonicalize(p->type);
        if (paramType != p->type) {
            p = new IR::Parameter(p->srcInfo, p->name, p->annotations, p->direction, paramType);
            changes = true;
        }
        setType(p, paramType);
        vec->push_back(p);
    }
    if (changes)
        return new IR::ParameterList(params->srcInfo, vec);
    else
        return params;
}

const IR::TypeParameters* TypeInference::canonicalize(const IR::TypeParameters* params) {
    auto newparams = new IR::IndexedVector<IR::Type_Var>();
    bool changes = false;
    for (auto p : *params->parameters) {
        auto np = canonicalize(p)->to<IR::Type_Var>();
        newparams->push_back(np);
        if (np != p)
            changes = true;
    }
    if (changes)
        return new IR::TypeParameters(params->srcInfo, newparams);
    else
        return params;
}

// This function does not return nullptr,
// but it may return the wrong type if typechecking fails.
// This function could probably be rewritten as a visitor...
const IR::Type* TypeInference::canonicalize(const IR::Type* type) {
    CHECK_NULL(type);
    if (typeMap->contains(type))
        return typeMap->getType(type, true);

    if (type->is<IR::Type_InfInt>()) {
        return type;
    } else if (type->is<IR::Type_Base>()) {
        if (!type->is<IR::Type_Bits>())
            // all other base types are singletons
            return type;
        auto tb = type->to<IR::Type_Bits>();
        auto canon = IR::Type_Bits::get(tb->size, tb->isSigned);
        return canon;
    } else if (type->is<IR::Type_Enum>() ||
               type->is<IR::Type_ActionEnum>() ||
               type->is<IR::Type_MatchKind>()) {
        return type;
    } else if (type->is<IR::Type_Tuple>()) {
        auto tuple = type->to<IR::Type_Tuple>();
        auto fields = new IR::Vector<IR::Type>();
        // tuple<set<a>, b> = set<tuple<a, b>>
        bool anySet = false;
        for (auto t : *tuple->components) {
            if (t->is<IR::Type_Set>()) {
                anySet = true;
                t = t->to<IR::Type_Set>()->elementType;
            }
            t = canonicalize(t);
            fields->push_back(t);
        }
        const IR::Type* canon = new IR::Type_Tuple(type->srcInfo, fields);
        if (anySet)
            canon = new IR::Type_Set(type->srcInfo, canon);
        return canon;
    } else if (type->is<IR::Type_Parser>()) {
        auto tp = type->to<IR::Type_Parser>();
        auto pl = canonicalize(tp->applyParams);
        auto tps = canonicalize(tp->typeParams);
        if (pl != tp->applyParams || tps != tp->typeParams)
            return new IR::Type_Parser(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::Type_Control>()) {
        auto tp = type->to<IR::Type_Control>();
        auto pl = canonicalize(tp->applyParams);
        auto tps = canonicalize(tp->typeParams);
        if (pl != tp->applyParams || tps != tp->typeParams)
            return new IR::Type_Control(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::Type_Package>()) {
        auto tp = type->to<IR::Type_Package>();
        auto pl = canonicalize(tp->constructorParams);
        auto tps = canonicalize(tp->typeParams);
        if (pl != tp->constructorParams || tps != tp->typeParams)
            return new IR::Type_Package(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::P4Control>()) {
        auto cont = type->to<IR::P4Control>();
        auto ctype = canonicalize(cont->type)->to<IR::Type_Control>();
        auto pl = canonicalize(cont->constructorParams);
        if (ctype != cont->type || pl != cont->constructorParams)
            return new IR::P4Control(cont->srcInfo, cont->name,
                                     ctype, pl, cont->stateful, cont->body);
        return type;
    } else if (type->is<IR::P4Parser>()) {
        auto cont = type->to<IR::P4Parser>();
        auto ctype = canonicalize(cont->type)->to<IR::Type_Parser>();
        auto pl = canonicalize(cont->constructorParams);
        if (ctype != cont->type || pl != cont->constructorParams)
            return new IR::P4Parser(cont->srcInfo, cont->name, ctype,
                                    pl, cont->stateful, cont->states);
        return type;
    } else if (type->is<IR::Type_Extern>()) {
        auto te = type->to<IR::Type_Extern>();
        bool changes = false;
        auto methods = new IR::Vector<IR::Method>();
        bool constructorFound = false;
        for (auto method : *te->methods) {
            if (method->name == te->name)
                constructorFound = true;
            auto fpType = canonicalize(method->type);
            if (fpType == nullptr)
                return type;

            if (fpType != method->type) {
                method = new IR::Method(method->srcInfo, method->name,
                                        fpType->to<IR::Type_Method>(), method->isAbstract);
                changes = true;
                setType(method, fpType);
            }

            methods->push_back(method);
        }
        if (!constructorFound) {
            // synthesize a default constructor if and only if none given
            auto ct = new IR::Type_Method(Util::SourceInfo(), new IR::TypeParameters(),
                                          nullptr, new IR::ParameterList());
            auto constructor = new IR::Method(Util::SourceInfo(), te->name, ct, false);
            setType(constructor, ct);
            methods->push_back(constructor);
            changes = true;
        }
        auto tps = canonicalize(te->typeParameters);
        const IR::Type* resultType = type;
        if (changes || tps != te->typeParameters)
            resultType = new IR::Type_Extern(te->srcInfo, te->name, tps, methods);
        return resultType;
    } else if (type->is<IR::Type_Method>()) {
        auto mt = type->to<IR::Type_Method>();
        const IR::Type* res = nullptr;
        if (mt->returnType != nullptr)
            res = canonicalize(mt->returnType);
        bool changes = res != mt->returnType;
        auto pl = canonicalize(mt->parameters);
        auto tps = canonicalize(mt->typeParameters);
        changes = changes || pl != mt->parameters || tps != mt->typeParameters;
        const IR::Type* resultType = mt;
        if (changes)
            resultType = new IR::Type_Method(mt->getSourceInfo(), tps, res, pl);
        return resultType;
    } else if (type->is<IR::Type_Header>()) {
        auto hdr = type->to<IR::Type_Header>();
        auto fields = canonicalizeFields(hdr);
        const IR::Type* canon;
        if (fields != hdr->fields)
            canon = new IR::Type_Header(hdr->srcInfo, hdr->name, hdr->annotations, fields);
        else
            canon = hdr;
        return canon;
    } else if (type->is<IR::Type_Struct>()) {
        auto str = type->to<IR::Type_Struct>();
        auto fields = canonicalizeFields(str);
        const IR::Type* canon;
        if (fields != str->fields)
            canon = new IR::Type_Struct(str->srcInfo, str->name, str->annotations, fields);
        else
            canon = str;
        return canon;
    } else if (type->is<IR::Type_Union>()) {
        auto str = type->to<IR::Type_Union>();
        auto fields = canonicalizeFields(str);
        const IR::Type* canon;
        if (fields != str->fields)
            canon = new IR::Type_Union(str->srcInfo, str->name, str->annotations, fields);
        else
            canon = str;
        return canon;
    } else if (type->is<IR::Type_Specialized>()) {
        auto st = type->to<IR::Type_Specialized>();
        auto baseCanon = canonicalize(st->baseType);
        if (st->arguments == nullptr)
            return baseCanon;

        if (!baseCanon->is<IR::IMayBeGenericType>()) {
            typeError("%1%: Type %2% is not generic and thus it"
                      " cannot be specialized using type arguments", type, baseCanon);
            return type;
        }

        auto gt = baseCanon->to<IR::IMayBeGenericType>();
        auto tp = gt->getTypeParameters();
        if (tp->size() != st->arguments->size()) {
            typeError("%1%: Type %2% has %3% type parameter(s), but it is specialized with %4%",
                      type, gt, tp->size(), st->arguments->size());
            return type;
        }

        IR::Vector<IR::Type> *args = new IR::Vector<IR::Type>();
        for (const IR::Type* a : *st->arguments) {
            const IR::Type* canon = canonicalize(a);
            args->push_back(canon);
        }
        auto specialized = gt->specialize(args);

        auto result = new IR::Type_SpecializedCanonical(
            type->srcInfo, baseCanon, args, specialized);
        // learn the types of all components of the specialized type
        LOG1("Scanning the specialized type");
        TypeInference tc(refMap, typeMap, false, true);
        (void)result->apply(tc);
        return result;
    }
    BUG("Unhandled type %1%", type);
    // unreachable
    return nullptr;
}

///////////////////////////////////// Visitor methods

const IR::Node* TypeInference::preorder(IR::P4Program* program) {
    if (typeMap->checkMap(getOriginal()) && readOnly) {
        LOG1("No need to typecheck");
        prune();
    }
    return program;
}

const IR::Node* TypeInference::postorder(IR::Declaration_Errors* decl) {
    if (done()) return decl;
    for (auto id : *decl->getDeclarations())
        setType(id->getNode(), IR::Type_Error::get());
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Declaration_MatchKind* decl) {
    if (done()) return decl;
    for (auto id : *decl->getDeclarations())
        setType(id->getNode(), IR::Type_MatchKind::get());
    return decl;
}

const IR::Node* TypeInference::postorder(IR::P4Control* cont) {
    if (done()) return cont;
    auto canon = canonicalize(cont);
    setType(getOriginal(), canon);
    setType(cont, canon);
    return cont;
}

const IR::Node* TypeInference::postorder(IR::P4Parser* cont) {
    if (done()) return cont;
    auto canon = canonicalize(cont);
    setType(getOriginal(), canon);
    setType(cont, canon);
    return cont;
}

const IR::Node* TypeInference::postorder(IR::P4Table* table) {
    if (done()) return table;
    auto type = new IR::Type_Table(Util::SourceInfo(), table);
    setType(getOriginal(), type);
    setType(table, type);
    return table;
}

const IR::Node* TypeInference::postorder(IR::P4Action* action) {
    if (done()) return action;
    auto pl = canonicalize(action->parameters);
    auto type = new IR::Type_Action(Util::SourceInfo(), new IR::TypeParameters(),
                                    nullptr, pl);

    bool foundDirectionless = false;
    for (auto p : *action->parameters->parameters) {
        auto ptype = getType(p);
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

const IR::Node* TypeInference::postorder(IR::Declaration_Variable* decl) {
    if (done())
        return decl;
    auto type = getType(decl->type);
    if (type == nullptr)
        return decl;

    if (type->is<IR::IMayBeGenericType>()) {
        // Check that there are no unbound type parameters
        const IR::IMayBeGenericType* gt = type->to<IR::IMayBeGenericType>();
        if (!gt->getTypeParameters()->empty()) {
            typeError("Unspecified type parameters for %1% in %2%", gt, decl);
            return decl;
        }
    }

    auto orig = getOriginal<IR::Declaration_Variable>();
    if (decl->initializer != nullptr) {
        auto init = assignment(decl, type, decl->initializer);
        if (decl->initializer != init) {
            decl->type = type;
            decl->initializer = init;
            LOG1("Created new declaration " << decl);
        }
    }
    setType(decl, type);
    setType(orig, type);
    return decl;
}

bool TypeInference::canCastBetween(const IR::Type* dest, const IR::Type* src) const {
    if (src == dest)
        return true;
    if (src->is<IR::Type_Bits>()) {
        auto f = src->to<IR::Type_Bits>();
        if (dest->is<IR::Type_Bits>()) {
            auto t = dest->to<IR::Type_Bits>();
            if (f->size == t->size)
                return true;
            else if (f->isSigned == t->isSigned)
                return true;
        } else if (dest->is<IR::Type_Boolean>()) {
            return f->size == 1 && !f->isSigned;
        }
    } else if (src->is<IR::Type_Boolean>()) {
        if (dest->is<IR::Type_Bits>()) {
            auto b = dest->to<IR::Type_Bits>();
            return b->size == 1 && !b->isSigned;
        }
    }
    return false;
}

const IR::Expression*
TypeInference::assignment(const IR::Node* errorPosition, const IR::Type* destType,
                          const IR::Expression* sourceExpression) {
    if (destType->is<IR::Type_Unknown>())
        BUG("Unknown destination type");
    const IR::Type* initType = getType(sourceExpression);
    if (initType == nullptr)
        return sourceExpression;

    if (initType == destType)
        return sourceExpression;

    if (canCastBetween(destType, initType)) {
        LOG1("Inserting cast in " << sourceExpression);
        bool isConst = isCompileTimeConstant(sourceExpression);
        sourceExpression = new IR::Cast(sourceExpression->srcInfo, destType, sourceExpression);
        setType(sourceExpression, destType);
        if (isConst)
            setCompileTimeConstant(sourceExpression);
        return sourceExpression;
    }

    auto tvs = unify(errorPosition, destType, initType, true);
    if (tvs == nullptr)
        // error already signalled
        return sourceExpression;
    if (tvs->isIdentity())
        return sourceExpression;

    ConstantTypeSubstitution cts(tvs, typeMap);
    auto newInit = cts.convert(sourceExpression);  // sets type
    return newInit;
}

const IR::Node* TypeInference::postorder(IR::Declaration_Constant* decl) {
    if (done()) return decl;
    auto type = getType(decl->type);
    if (type == nullptr)
        return decl;

    if (type->is<IR::Type_Extern>()) {
        typeError("%1%: Cannot declare constants of extern types", decl->name);
        return decl;
    }

    if (!isCompileTimeConstant(decl->initializer))
        typeError("%1%: Cannot evaluate initializer to a compile-time constant", decl->initializer);
    auto orig = getOriginal<IR::Declaration_Constant>();
    auto newInit = assignment(decl, type, decl->initializer);
    if (newInit != decl->initializer)
        decl = new IR::Declaration_Constant(decl->srcInfo, decl->name,
                                            decl->annotations, decl->type, newInit);
    setType(decl, type);
    setType(orig, type);
    return decl;
}

// Returns new arguments for constructor, which may have inserted casts
const IR::Vector<IR::Expression> *
TypeInference::checkExternConstructor(const IR::Node* errorPosition,
                                      const IR::Type_Extern* ext,
                                      const IR::Vector<IR::Expression> *arguments) {
    auto tp = ext->getTypeParameters();
    if (!tp->empty()) {
        typeError("%1%: Type parameters must be supplied for constructor", errorPosition);
        return nullptr;
    }
    auto constructor = ext->lookupMethod(ext->name.name, arguments->size());
    if (constructor == nullptr) {
        typeError("%1%: type %2% has no constructor with %3% arguments",
                  errorPosition, ext, arguments->size());
        return nullptr;
    }
    auto mt = getType(constructor);
    if (mt == nullptr)
        return nullptr;
    auto methodType = mt->to<IR::Type_Method>();
    BUG_CHECK(methodType != nullptr, "Constructor does not have a method type, but %1%", mt);

    bool changes = false;
    auto result = new IR::Vector<IR::Expression>();
    size_t i = 0;
    for (auto pi : *methodType->parameters->getEnumerator()) {
        auto arg = arguments->at(i++);
        if (!isCompileTimeConstant(arg))
            typeError("%1%: cannot evaluate to a compile-time constant", arg);
        auto argType = getType(arg);
        auto paramType = getType(pi);
        if (argType == nullptr || paramType == nullptr)
            return nullptr;

        auto tvs = unify(errorPosition, paramType, argType, true);
        if (tvs == nullptr) {
            // error already signalled
            return nullptr;
        }
        if (tvs->isIdentity()) {
            result->push_back(arg);
            continue;
        }

        ConstantTypeSubstitution cts(tvs, typeMap);
        auto newArg = cts.convert(arg);
        result->push_back(newArg);
        setType(newArg, paramType);
        changes = true;
    }
    if (changes)
        return result;
    else
        return arguments;
}

// Return true on success
bool TypeInference::checkAbstractMethods(const IR::Declaration_Instance* inst,
                                         const IR::Type_Extern* type) {
    // Make a list of the abstract methods
    IR::NameMap<IR::Method, ordered_map> virt;
    for (auto m : *type->methods)
        if (m->isAbstract)
            virt.addUnique(m->name, m);
    if (virt.size() == 0 && inst->initializer == nullptr)
        return true;
    if (virt.size() == 0 && inst->initializer != nullptr) {
        typeError("%1%: instance initializers for extern without abstract methods",
                  inst->initializer);
        return false;
    } else if (virt.size() != 0 && inst->initializer == nullptr) {
        typeError("%1%: must declare abstract methods for %2%", inst, type);
        return false;
    }

    for (auto d : *inst->initializer->components) {
        if (d->is<IR::Function>()) {
            auto func = d->to<IR::Function>();
            LOG1("Type checking " << func);
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
            auto tvs = unify(inst, methtype, ftype, true);
            if (tvs == nullptr)
                return false;
            BUG_CHECK(tvs->isIdentity(), "%1%: expected no type variables", tvs);
        }
    }

    if (virt.size() != 0) {
        typeError("%1%: %2% abstract method not implemented",
                  inst, virt.begin()->second);
        return false;
    }
    return true;
}

const IR::Node* TypeInference::preorder(IR::Declaration_Instance* decl) {
    // We need to control the order of the type-checking: we want to do first
    // the declaration, and then typecheck the initializer if present.
    if (done())
        return decl;
    visit(decl->type);
    visit(decl->arguments);

    auto type = canonicalize(decl->type);
    auto orig = getOriginal<IR::Declaration_Instance>();

    auto simpleType = type;
    if (type->is<IR::Type_SpecializedCanonical>())
        simpleType = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (simpleType->is<IR::Type_Extern>()) {
        auto et = simpleType->to<IR::Type_Extern>();
        setType(orig, type);
        setType(decl, type);

        if (decl->initializer != nullptr)
            visit(decl->initializer);
        // This will need the decl type to be already known
        bool s = checkAbstractMethods(decl, et);
        if (!s) {
            prune();
            return decl;
        }

        auto args = checkExternConstructor(decl, et, decl->arguments);
        if (args == nullptr) {
            prune();
            return decl;
        }
        if (args != decl->arguments)
            decl->arguments = args;
    } else if (simpleType->is<IR::IContainer>()) {
        if (decl->initializer != nullptr)
            typeError("%1%: initializers only allowed for extern instances", decl->initializer);
        type = containerInstantiation(decl, decl->arguments, simpleType->to<IR::IContainer>());
        if (type == nullptr) {
            prune();
            return decl;
        }
        setType(decl, type);
        setType(orig, type);
    } else {
        typeError("%1%: cannot allocate objects of type %2%", decl, type);
    }
    prune();
    return decl;
}

const IR::Type*
TypeInference::containerInstantiation(
    const IR::Node* node,  // can be Declaration_Instance or ConstructorCallExpression
    const IR::Vector<IR::Expression>* constructorArguments,
    const IR::IContainer* container) {
    CHECK_NULL(node); CHECK_NULL(constructorArguments); CHECK_NULL(container);

    auto constructor = container->getConstructorMethodType();

    // We build a type for the callExpression and unify it with the method expression
    // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
    auto args = new IR::Vector<IR::ArgumentInfo>();
    for (auto arg : *constructorArguments) {
        if (!isCompileTimeConstant(arg))
            typeError("%1%: cannot evaluate to a compile-time constant", arg);
        auto argType = getType(arg);
        if (argType == nullptr)
            return nullptr;
        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg, true, argType);
        args->push_back(argInfo);
    }
    auto rettype = new IR::Type_Var(Util::SourceInfo(), IR::ID(refMap->newName("R")));
    // There are never type arguments at this point; if they exist, they have been folded
    // into the constructor by type specialization.
    auto callType = new IR::Type_MethodCall(node->srcInfo,
                                            new IR::Vector<IR::Type>(),
                                            rettype, args);
    TypeConstraints constraints;
    constraints.addEqualityConstraint(constructor, callType);
    auto result = constraints.solve(node, true);
    if (result == nullptr)
        return nullptr;

    auto returnType = result->lookup(rettype);
    BUG_CHECK(returnType != nullptr, "Cannot infer constructor result type %1%", node);
    return returnType;
}

const IR::Node* TypeInference::preorder(IR::Function* function) {
    if (done()) return function;
    visit(function->type);
    auto type = getType(function->type);
    if (type == nullptr)
        return function;
    setType(getOriginal(), type);
    setType(function, type);
    visit(function->body);
    prune();
    return function;
}

const IR::Node* TypeInference::postorder(IR::Method* method) {
    if (done()) return method;
    auto type = getType(method->type);
    if (type == nullptr)
        return method;
    setType(getOriginal(), type);
    setType(method, type);
    return method;
}

//////////////////////////////////////////////  Types

const IR::Node* TypeInference::postorder(IR::Type_InfInt* type) {
    setType(getOriginal(), getOriginal<IR::Type_InfInt>());
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_ArchBlock* decl) {
    if (done()) return decl;
    auto canon = canonicalize(getOriginal<IR::Type_ArchBlock>());
    setType(getOriginal(), canon);
    setType(decl, canon);
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Package* decl) {
    if (done()) return decl;
    auto canon = canonicalize(getOriginal<IR::Type_Package>());
    setType(getOriginal(), canon);
    setType(decl, canon);
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Specialized *type) {
    if (done()) return type;
    auto canon = canonicalize(type);
    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_SpecializedCanonical *type) {
    // This is always canonical
    if (done()) return type;
    setType(getOriginal(), type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Name* typeName) {
    if (done()) return typeName;
    const IR::Type* type;

    if (typeName->path->isDontCare()) {
        type = IR::Type_Dontcare::get();
    } else {
        auto decl = refMap->getDeclaration(typeName->path, true);
        type = getType(decl->getNode());
        if (type == nullptr)
            return typeName;
        type = getType(type);
        if (type == nullptr)
            return typeName;
    }
    setType(typeName->path, type);
    setType(getOriginal(), type);
    setType(typeName, type);
    return typeName;
}

const IR::Node* TypeInference::postorder(IR::Type_ActionEnum* type) {
    if (done()) return type;
    setType(getOriginal(), type);
    setType(type, type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Enum* type) {
    if (done()) return type;
    for (auto e : *type->getDeclarations())
        setType(e->getNode(), type);
    setType(getOriginal(), type);
    setType(type, type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Var* typeVar) {
    if (done()) return typeVar;
    const IR::Type* type;
    if (typeVar->name.isDontCare())
        type = IR::Type_Dontcare::get();
    else
        type = getOriginal<IR::Type>();
    setType(getOriginal(), type);
    setType(typeVar, type);
    return typeVar;
}

const IR::Node* TypeInference::postorder(IR::Type_Tuple* type) {
    if (done()) return type;
    auto canon = canonicalize(getOriginal<IR::Type_Tuple>());
    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Extern* type) {
    if (done()) return type;
    auto canon = canonicalize(getOriginal()->to<IR::Type_Extern>())->to<IR::Type_Extern>();
    for (auto method : *canon->methods) {
        if (method->name == type->name) {  // constructor
            if (method->type->typeParameters != nullptr &&
                method->type->typeParameters->size() > 0) {
                typeError("%1%: Constructors cannot have type parameters",
                          method->type->typeParameters);
                return type;
            }
        }

        auto m = canon->lookupMethod(method->name, method->type->parameters->size());
        if (m == nullptr)  // duplicate method with this signature
            return type;
    }

    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Method* type) {
    if (done()) return type;
    auto canon = canonicalize(getOriginal<IR::Type_Method>());
    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Action* type) {
    if (done()) return type;
    // This type should already be canonical
    setType(getOriginal(), type);
    setType(type, type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Base* type) {
    if (done()) return type;
    auto canon = canonicalize(getOriginal<IR::Type_Base>());
    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Typedef* tdecl) {
    if (done()) return tdecl;
    auto type = getType(tdecl->type);
    if (type == nullptr)
        return tdecl;
    setType(getOriginal(), type);
    setType(tdecl, type);
    return tdecl;
}

const IR::Node* TypeInference::postorder(IR::Type_Stack* type) {
    if (done()) return type;
    if (!type->sizeKnown())
        typeError("%1%: Size of header stack type should be a constant", type);

    auto etype = getType(type->baseType);
    if (etype == nullptr)
        return type;

    if (!etype->is<IR::Type_Header>() && !etype->is<IR::Type_Union>())
        typeError("Header stack %1% used with non-header type %2%",
                  type, etype->toString());

    auto canon = new IR::Type_Stack(type->srcInfo, etype, type->size);
    setType(getOriginal(), canon);
    setType(type, canon);
    return type;
}

bool TypeInference::validateFields(const IR::Type_StructLike* type,
                                   std::function<bool(const IR::Type*)> checker) const {
    for (auto field : *type->fields) {
        if (!checker(field->type)) {
            typeError("Field %1% of %2% cannot have type %3%",
                      field, type->toString(), field->type);
            return false;
        }
    }
    return true;
}

const IR::Node* TypeInference::postorder(IR::StructField* field) {
    if (done()) return field;
    auto canon = getType(field->type);
    if (canon == nullptr)
        return field;

    setType(getOriginal(), canon);
    setType(field, canon);
    return field;
}

const IR::Node* TypeInference::postorder(IR::Type_Header* type) {
    if (done()) return type;
    auto orig = getOriginal<IR::Type_Header>();
    auto canon = canonicalize(orig)->to<IR::Type_Header>();
    auto validator = [] (const IR::Type* t)
            { return t->is<IR::Type_Bits>() || t->is<IR::Type_Varbits>(); };
    if (validateFields(canon, validator)) {
        if (canon != orig) {
            TypeInference tc(refMap, typeMap, false, true);
            (void)canon->apply(tc);
        }
        setType(getOriginal(), canon);
        setType(type, canon);
    }
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Struct* type) {
    if (done()) return type;
    auto orig = getOriginal<IR::Type_Struct>();
    auto canon = canonicalize(orig)->to<IR::Type_Struct>();
    auto validator = [] (const IR::Type* t) {
        return t->is<IR::Type_Struct>() || t->is<IR::Type_Bits>() ||
        t->is<IR::Type_Header>() || t->is<IR::Type_Union>() ||
        t->is<IR::Type_Enum>() || t->is<IR::Type_Error>() ||
        t->is<IR::Type_Boolean>() || t->is<IR::Type_Stack>() ||
        t->is<IR::Type_ActionEnum>(); };
    if (validateFields(canon, validator)) {
        // Learn the types of the canonical fields too
        if (canon != orig) {
            TypeInference tc(refMap, typeMap, false, true);
            (void)canon->apply(tc);
        }
        setType(getOriginal(), canon);
        setType(type, canon);
    }
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Union *type) {
    if (done()) return type;
    auto orig = getOriginal<IR::Type_Union>();
    auto canon = canonicalize(orig)->to<IR::Type_Union>();
    auto validator = [] (const IR::Type* t) { return t->is<IR::Type_Header>(); };
    if (validateFields(canon, validator)) {
        if (canon != orig) {
            TypeInference tc(refMap, typeMap, false, true);
            (void)canon->apply(tc);
        }
        setType(getOriginal(), canon);
        setType(type, canon);
    }
    return type;
}

////////////////////////////////////////////////// expressions

const IR::Node* TypeInference::postorder(IR::Parameter* param) {
    if (done()) return param;
    const IR::Type* paramType = getType(param->type);
    if (paramType == nullptr)
        return param;

    // The parameter type cannot have free type variables
    if (paramType->is<IR::IMayBeGenericType>()) {
        auto gen = paramType->to<IR::IMayBeGenericType>();
        auto tp = gen->getTypeParameters();
        if (!tp->empty()) {
            typeError("Type parameters needed for %1%", param->name);
            return param;
        }
    }
    setType(getOriginal(), paramType);
    setType(param, paramType);
    return param;
}

const IR::Node* TypeInference::postorder(IR::Constant* expression) {
    if (done()) return expression;
    auto type = getType(expression->type);
    if (type == nullptr)
        return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    setCompileTimeConstant(expression);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::BoolLiteral* expression) {
    if (done()) return expression;
    setType(getOriginal(), IR::Type_Boolean::get());
    setType(expression, IR::Type_Boolean::get());
    setCompileTimeConstant(getOriginal<IR::Expression>());
    setCompileTimeConstant(expression);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Operation_Relation* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    bool equTest = expression->is<IR::Equ>() || expression->is<IR::Neq>();

    if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_Bits>()) {
        auto cast = new IR::Cast(Util::SourceInfo(), rtype, expression->left);
        setType(cast, rtype);
        auto e = expression->clone();
        e->left = cast;
        expression = e;
        ltype = rtype;
    } else if (rtype->is<IR::Type_InfInt>() && ltype->is<IR::Type_Bits>()) {
        auto cast = new IR::Cast(Util::SourceInfo(), ltype, expression->right);
        setType(cast, ltype);
        auto e = expression->clone();
        e->right = cast;
        expression = e;
        rtype = ltype;
    }

    if (equTest) {
        bool defined = false;
        if (ltype == rtype && (!ltype->is<IR::Type_Void>() && !ltype->is<IR::Type_Varbits>()))
            defined = true;
        else if (ltype->is<IR::Type_Base>() && rtype->is<IR::Type_Base>() && ltype == rtype)
            defined = true;

        if (!defined) {
            typeError("%1%: not defined on %2% and %3%",
                      expression, ltype->toString(), rtype->toString());
            return expression;
        }
    } else {
        if (!ltype->is<IR::Type_Bits>() || !rtype->is<IR::Type_Bits>() || !(ltype == rtype)) {
            typeError("%1%: not defined on %2% and %3%",
                      expression, ltype->toString(), rtype->toString());
            return expression;
        }
    }
    setType(getOriginal(), IR::Type_Boolean::get());
    setType(expression, IR::Type_Boolean::get());
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Concat* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    if (ltype->is<IR::Type_InfInt>()) {
        typeError("Please specify a width for the operand %1% of a concatenation",
                  expression->left);
        return expression;
    }
    if (rtype->is<IR::Type_InfInt>()) {
        typeError("Please specify a width for the operand %1% of a concatenation",
                  expression->right);
        return expression;
    }
    if (!ltype->is<IR::Type_Bits>() || !rtype->is<IR::Type_Bits>() || !(*ltype == *rtype)) {
        typeError("%1%: Concatenation not defined on %2% and %3%",
                  expression, ltype->toString(), rtype->toString());
        return expression;
    }
    auto bl = ltype->to<IR::Type_Bits>();
    auto br = rtype->to<IR::Type_Bits>();
    const IR::Type* resultType = IR::Type_Bits::get(Util::SourceInfo(),
                                                    bl->size + br->size, bl->isSigned);
    resultType = canonicalize(resultType);
    setType(getOriginal(), resultType);
    setType(expression, resultType);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::ListExpression* expression) {
    if (done()) return expression;
    bool constant = true;
    auto components = new IR::Vector<IR::Type>();
    for (auto c : *expression->components) {
        if (!isCompileTimeConstant(c))
            constant = false;
        auto type = getType(c);
        if (type == nullptr)
            return expression;
        components->push_back(type);
    }

    auto tupleType = new IR::Type_Tuple(expression->srcInfo, components);
    auto type = canonicalize(tupleType);
    CHECK_NULL(type);
    setType(getOriginal(), type);
    setType(expression, type);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::ArrayIndex* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    if (!ltype->is<IR::Type_Stack>()) {
        typeError("Array indexing %1% applied to non-array type %2%",
                  expression, ltype->toString());
        return expression;
    }

    bool rightOpConstant = expression->right->is<IR::Constant>();
    if (!rtype->is<IR::Type_Bits>() && !rightOpConstant) {
        typeError("Array index %1% must be an integer, but it has type %2%",
                  expression->right, rtype->toString());
        return expression;
    }

    auto hst = ltype->to<IR::Type_Stack>();
    if (isLeftValue(expression->left)) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    }

    if (rightOpConstant) {
        auto cst = expression->right->to<IR::Constant>();
        if (!cst->fitsInt()) {
            typeError("Index too large: %1%", cst);
            return expression;
        }
        int index = cst->asInt();
        if (index < 0) {
            typeError("Negative array index %1%", cst);
            return expression;
        }
        if (hst->sizeKnown()) {
            int size = hst->getSize();
            if (index >= size) {
                typeError("Array index %1% larger than array size %2%",
                          cst, hst->size);
                return expression;
            }
        }
    }
    setType(getOriginal(), hst->baseType);
    setType(expression, hst->baseType);
    return expression;
}

const IR::Node* TypeInference::binaryBool(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    if (!ltype->is<IR::Type_Boolean>() || !rtype->is<IR::Type_Boolean>()) {
        typeError("%1%: not defined on %2% and %3%",
                  expression, ltype->toString(), rtype->toString());
        return expression;
    }
    setType(getOriginal(), IR::Type_Boolean::get());
    setType(expression, IR::Type_Boolean::get());
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::binaryArith(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    }

    const IR::Type* resultType = ltype;
    if (bl != nullptr && br != nullptr) {
        if (bl->size != br->size) {
            typeError("%1%: Cannot operate on values with different widths %2% and %3%",
                      expression, bl->size, br->size);
            return expression;
        }
        if (bl->isSigned != br->isSigned) {
            typeError("%1%: Cannot operate on values with different signs", expression);
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), rtype, expression->left);
        auto e = expression->clone();
        e->left = cast;
        expression = e;
        resultType = rtype;
        setType(cast, resultType);
        setType(expression, resultType);
    } else if (bl != nullptr && br == nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), ltype, expression->right);
        auto e = expression->clone();
        e->right = cast;
        expression = e;
        resultType = ltype;
        setType(cast, resultType);
        setType(expression, resultType);
    } else {
        setType(expression, resultType);
    }
    setType(getOriginal(), resultType);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::unsBinaryArith(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    if (bl != nullptr && bl->isSigned) {
        typeError("%1%: Cannot operate on signed values", expression);
        return expression;
    }
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (br != nullptr && br->isSigned) {
        typeError("%1%: Cannot operate on signed values", expression);
        return expression;
    }

    auto cleft = expression->left->to<IR::Constant>();
    if (cleft != nullptr) {
        if (sgn(cleft->value) < 0) {
            typeError("%1%: not defined on negative numbers", expression);
            return expression;
        }
    }
    auto cright = expression->right->to<IR::Constant>();
    if (cright != nullptr) {
        if (sgn(cright->value) < 0) {
            typeError("%1%: not defined on negative numbers", expression);
            return expression;
        }
    }

    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return binaryArith(expression);
}

const IR::Node* TypeInference::shift(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    if (!ltype->is<IR::Type_Bits>()) {
        typeError("%1% left operand of shift must be a numeric type, not %2%",
                  expression, ltype->toString());
        return expression;
    }

    auto lt = ltype->to<IR::Type_Bits>();
    if (expression->right->is<IR::Constant>()) {
        auto cst = expression->right->to<IR::Constant>();
        if (!cst->fitsInt()) {
            typeError("Shift amount too large: %1%", cst);
            return expression;
        }
        int shift = cst->asInt();
        if (shift < 0) {
            typeError("%1%: Negative shift amount %2%", expression, cst);
            return expression;
        }
        if (shift >= lt->size)
            ::warning("%1%: shifting value with %2% bits by %3%",
                      expression, lt->size, shift);
    }

    if (rtype->is<IR::Type_Bits>() && rtype->to<IR::Type_Bits>()->isSigned) {
        typeError("%1%: Shift amount must be an unsigned number",
                  expression->right);
        return expression;
    }

    setType(expression, ltype);
    setType(getOriginal(), ltype);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::bitwise(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    }

    const IR::Type* resultType = ltype;
    if (bl != nullptr && br != nullptr) {
        if (!(*bl == *br)) {
            typeError("%1%: Cannot operate on values with different types %2% and %3%",
                      expression, bl->toString(), br->toString());
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), rtype, expression->left);
        auto e = expression->clone();
        e->left = cast;
        expression = e;
        resultType = rtype;
        setType(cast, resultType);
        setType(expression, resultType);
    } else if (bl != nullptr && br == nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), ltype, expression->right);
        auto e = expression->clone();
        e->right = cast;
        expression = e;
        resultType = ltype;
        setType(cast, resultType);
        setType(expression, resultType);
    } else {
        setType(expression, resultType);
    }
    setType(getOriginal(), resultType);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

// Handle .. and &&&
const IR::Node* TypeInference::typeSet(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    // The following section is very similar to "binary()" above
    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to %2% of type %3%",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    }

    const IR::Type* sameType = ltype;
    if (bl != nullptr && br != nullptr) {
        if (!(*bl == *br)) {
            typeError("%1%: Cannot operate on values with different types %2% and %3%",
                      expression, bl->toString(), br->toString());
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), rtype, expression->left);
        auto e = expression->clone();
        e->left = cast;
        expression = e;
        sameType = rtype;
        setType(cast, sameType);
    } else if (bl != nullptr && br == nullptr) {
        auto cast = new IR::Cast(Util::SourceInfo(), ltype, expression->right);
        auto e = expression->clone();
        e->right = cast;
        expression = e;
        sameType = ltype;
        setType(cast, sameType);
    }

    auto resultType = new IR::Type_Set(sameType->srcInfo, sameType);
    typeMap->setType(expression, resultType);
    typeMap->setType(getOriginal(), resultType);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::LNot* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;
    if (!(*type == *IR::Type_Boolean::get())) {
        typeError("Cannot apply %1% to value %2% of type %3%",
                  expression->getStringOp(), expression->expr, type->toString());
    } else {
        setType(expression, IR::Type_Boolean::get());
        setType(getOriginal(), IR::Type_Boolean::get());
    }
    if (isCompileTimeConstant(expression->expr)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Neg* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (type->is<IR::Type_InfInt>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else if (type->is<IR::Type_Bits>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else {
        typeError("Cannot apply %1% to value %2% of type %3%",
                  expression->getStringOp(), expression->expr, type->toString());
    }
    if (isCompileTimeConstant(expression->expr)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Cmpl* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (type->is<IR::Type_InfInt>()) {
        typeError("%1% cannot be applied to an operand with an unknown width");
    } else if (type->is<IR::Type_Bits>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else {
        typeError("Cannot apply %1% to value %2% of type %3%",
                  expression->getStringOp(), expression->expr, type->toString());
    }
    if (isCompileTimeConstant(expression->expr)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Cast* expression) {
    if (done()) return expression;
    const IR::Type* sourceType = getType(expression->expr);
    const IR::Type* castType = getType(expression->destType);
    if (sourceType == nullptr || castType == nullptr)
        return expression;

    if (!canCastBetween(castType, sourceType)) {
        // This cast is not legal, but let's try to see whether
        // performing a substitution can help
        auto rhs = assignment(expression, castType, expression->expr);
        if (rhs == nullptr)
            // error
            return expression;
        if (rhs != expression->expr) {
            // if we are here we have performed a substitution on the rhs
            expression = new IR::Cast(expression->srcInfo, expression->destType, rhs);
            sourceType = expression->destType;
        }
        if (!canCastBetween(castType, sourceType))
            typeError("%1%: Illegal cast from %2% to %3%", expression, sourceType, castType);
    }
    setType(expression, castType);
    setType(getOriginal(), castType);
    if (isCompileTimeConstant(expression->expr)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::PathExpression* expression) {
    if (done()) return expression;
    auto decl = refMap->getDeclaration(expression->path, true)->getNode();
    const IR::Type* type = nullptr;

    if (decl->is<IR::ParserState>()) {
        type = IR::Type_State::get();
    } else if (decl->is<IR::Declaration_Variable>()) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    } else if (decl->is<IR::Parameter>()) {
        auto paramDecl = decl->to<IR::Parameter>();
        if (paramDecl->direction == IR::Direction::InOut ||
            paramDecl->direction == IR::Direction::Out) {
            setLeftValue(expression);
            setLeftValue(getOriginal<IR::Expression>());
        } else if (paramDecl->direction == IR::Direction::None) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
    } else if (decl->is<IR::Declaration_Constant>() ||
               decl->is<IR::Declaration_Instance>()) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    } else if (decl->is<IR::P4Action>()) {
        // Special handling for actions referred inside tables
        // action a(inout bit x, bit y) { ... }
        // table t () {
        //    actions = { a(z); }  << a typechecked as action a
        //    default_action = a(2);  << a typechecked as specialized in the actions list
        // }
        // This works only if the actions property has already been visited
        auto prop = findContext<IR::TableProperty>();
        if (prop != nullptr) {
            auto table = findContext<IR::P4Table>();
            BUG_CHECK(table != nullptr, "%1%: property not within a table?", prop);
            if (prop->name == IR::TableProperties::defaultActionPropertyName) {
                LOG1("Handling default_action" << prop);
                // Check that the default action appears in the list of actions.
                BUG_CHECK(prop->value->is<IR::ExpressionValue>(), "%1% not an expression", prop);
                auto def = prop->value->to<IR::ExpressionValue>()->expression;
                auto al = table->getActionList();
                if (al == nullptr) {
                    typeError("%1%: no action list, but %2% %3%",
                              table, IR::TableProperties::defaultActionPropertyName, prop);
                    return expression;
                }
                if (def->is<IR::MethodCallExpression>())
                    def = def->to<IR::MethodCallExpression>()->method;
                if (!def->is<IR::PathExpression>())
                    BUG("%1%: unexpected expression", def);
                auto pe = def->to<IR::PathExpression>();
                auto defdecl = refMap->getDeclaration(pe->path, true);
                auto ale = al->actionList->getDeclaration(defdecl->getName());
                if (ale == nullptr) {
                    typeError("%1% not present in action list", def);
                    return expression;
                }
                BUG_CHECK(ale->is<IR::ActionListElement>(),
                          "%1%: expected an ActionListElement", ale);
                auto elem = ale->to<IR::ActionListElement>();
                auto entrypath = elem->getPath();
                auto entrydecl = refMap->getDeclaration(entrypath, true);
                if (entrydecl != defdecl) {
                    typeError("%1% and %2% refer to different actions", def, elem);
                    return expression;
                }
                type = getType(elem);
                setType(getOriginal(), type);
                setType(expression, type);
                return expression;
            }
        }
    }

    if (type == nullptr) {
        type = getType(decl);
        if (type == nullptr)
            return expression;
    }

    setType(getOriginal(), type);
    setType(expression, type);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Slice* expression) {
    if (done()) return expression;
    const IR::Type* type = getType(expression->e0);
    if (type == nullptr)
        return expression;

    if (!type->is<IR::Type_Bits>()) {
        typeError("%1%: bit extraction only defined for bit<> types", expression);
        return expression;
    }

    auto bst = type->to<IR::Type_Bits>();
    if (!expression->e1->is<IR::Constant>() || !expression->e2->is<IR::Constant>()) {
        typeError("%1%: bit index values must be constants", expression);
        return expression;
    }

    auto msb = expression->e1->to<IR::Constant>();
    auto lsb = expression->e2->to<IR::Constant>();
    if (!msb->fitsInt()) {
        typeError("%1%: bit index too large", msb);
        return expression;
    }
    if (!lsb->fitsInt()) {
        typeError("%1%: bit index too large", lsb);
        return expression;
    }
    int m = msb->asInt();
    int l = lsb->asInt();
    if (m < 0) {
        typeError("%1%: negative bit index", msb);
        return expression;
    }
    if (l < 0) {
        typeError("%1%: negative bit index", msb);
        return expression;
    }
    if (m >= bst->size) {
        typeError("Bit index %1% greater than width %2%", msb, bst->size);
        return expression;
    }
    if (l >= bst->size) {
        typeError("Bit index %1% greater than width %2%", msb, bst->size);
        return expression;
    }
    if (l > m) {
        typeError("LSB index %1% greater than MSB index %2%", lsb, msb);
        return expression;
    }

    const IR::Type* result = IR::Type_Bits::get(bst->srcInfo, m - l + 1, bst->isSigned);
    result = canonicalize(result);
    setType(getOriginal(), result);
    setType(expression, result);
    if (isLeftValue(expression->e0)) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    }
    if (isCompileTimeConstant(expression->e0)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Mux* expression) {
    if (done()) return expression;
    const IR::Type* firstType = getType(expression->e0);
    const IR::Type* secondType = getType(expression->e1);
    const IR::Type* thirdType = getType(expression->e2);
    if (firstType == nullptr || secondType == nullptr || thirdType == nullptr)
        return expression;

    if (!firstType->is<IR::Type_Boolean>()) {
        typeError("Selector of %1% must be bool, not %2%",
                  expression->getStringOp(), firstType->toString());
        return expression;
    }

    if (secondType->is<IR::Type_InfInt>() && thirdType->is<IR::Type_InfInt>()) {
        typeError("Width must be specified for at least one of %1% or %2%",
                  expression->e1, expression->e2);
        return expression;
    }
    auto tvs = unify(expression, secondType, thirdType, true);
    if (tvs != nullptr) {
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap);
            auto e1 = cts.convert(expression->e1);
            auto e2 = cts.convert(expression->e2);
            expression->e1 = e1;
            expression->e2 = e2;
            secondType = typeMap->getType(e1);
        }
        setType(expression, secondType);
        setType(getOriginal(), secondType);
        if (isCompileTimeConstant(expression->e0) &&
            isCompileTimeConstant(expression->e1) &&
            isCompileTimeConstant(expression->e2)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::TypeNameExpression* expression) {
    if (done()) return expression;
    auto type = getType(expression->typeName);
    if (type == nullptr)
        return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Member* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (type->is<IR::Type_SpecializedCanonical>())
        type = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (type->is<IR::Type_Extern>()) {
        auto ext = type->to<IR::Type_Extern>();

        if (methodArguments.size() == 0) {
            // we are not within a call expression
            typeError("%1%: Methods can only be called", expression);
            return expression;
        }

        // Use number of arguments to disambiguate
        int argCount = methodArguments.back();
        auto method = ext->lookupMethod(expression->member, argCount);
        if (method == nullptr) {
            typeError("%1%: Interface %2% does not have a method named %3% with %4% arguments",
                      expression, ext->name, expression->member, argCount);
            return expression;
        }

        const IR::Type* methodType = getType(method);
        if (methodType == nullptr)
            return expression;
        setType(getOriginal(), methodType);
        setType(expression, methodType);
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    } else if (type->is<IR::Type_StructLike>()) {
        if (type->is<IR::Type_Header>()) {
            if (expression->member.name == IR::Type_Header::isValid) {
                // Built-in method
                auto type = new IR::Type_Method(
                    Util::SourceInfo(), new IR::TypeParameters(),
                    IR::Type_Boolean::get(), new IR::ParameterList());
                auto ctype = canonicalize(type);
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            } else if (expression->member.name == IR::Type_Header::setValid ||
                       expression->member.name == IR::Type_Header::setInvalid) {
                // Built-in method
                auto params = new IR::IndexedVector<IR::Parameter>();
                auto type = new IR::Type_Method(
                    Util::SourceInfo(), new IR::TypeParameters(), IR::Type_Void::get(),
                    new IR::ParameterList(Util::SourceInfo(), params));
                auto ctype = canonicalize(type);
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }

        auto stb = type->to<IR::Type_StructLike>();
        auto field = stb->getField(expression->member.name);
        if (field == nullptr) {
            typeError("Structure %1% does not have a field %2%",
                      stb, expression->member);
            return expression;
        }

        auto fieldType = getType(field->type);
        if (fieldType == nullptr)
            return expression;
        setType(getOriginal(), fieldType);
        setType(expression, fieldType);
        if (isLeftValue(expression->expr)) {
            setLeftValue(expression);
            setLeftValue(getOriginal<IR::Expression>());
        } else {
            LOG1("No left value " << expression->expr);
        }
        if (isCompileTimeConstant(expression->expr)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
        return expression;
    } else if (type->is<IR::IApply>()) {
        if (expression->member.name == IR::IApply::applyMethodName) {
            auto methodType = type->to<IR::IApply>()->getApplyMethodType();
            methodType = canonicalize(methodType)->to<IR::Type_Method>();
            // sometimes this is a synthesized type, so we have to crawl it to understand it
            TypeInference learn(refMap, typeMap, false, true);
            (void)methodType->apply(learn);

            setType(getOriginal(), methodType);
            setType(expression, methodType);
            return expression;
        }
    } else if (type->is<IR::Type_Stack>()) {
        if (expression->member.name == IR::Type_Stack::next ||
            expression->member.name == IR::Type_Stack::last) {
            auto control = findContext<IR::P4Control>();
            if (control != nullptr)
                typeError("%1%: 'last' and 'next' for stacks cannot be used in a control",
                          expression);
            auto stack = type->to<IR::Type_Stack>();
            setType(getOriginal(), stack->baseType);
            setType(expression, stack->baseType);
            if (isLeftValue(expression->expr)) {
                setLeftValue(expression);
                setLeftValue(getOriginal<IR::Expression>());
            }
            return expression;
        } else if (expression->member.name == IR::Type_Stack::empty ||
                   expression->member.name == IR::Type_Stack::full) {
            setType(getOriginal(), IR::Type_Boolean::get());
            setType(expression, IR::Type_Boolean::get());
            return expression;
        } else if (expression->member.name == IR::Type_Stack::push_front ||
                   expression->member.name == IR::Type_Stack::pop_front) {
            auto params = new IR::IndexedVector<IR::Parameter>();
            params->push_back(new IR::Parameter(Util::SourceInfo(), IR::ID("count"),
                                                IR::Annotations::empty, IR::Direction::In,
                                                new IR::Type_InfInt()));
            auto type = new IR::Type_Method(Util::SourceInfo(), new IR::TypeParameters(),
                                            IR::Type_Void::get(),
                                            new IR::ParameterList(Util::SourceInfo(), params));
            auto canon = canonicalize(type);
            setType(getOriginal(), canon);
            setType(expression, canon);
        }
    } else if (type->is<IR::ISimpleNamespace>()) {
        auto fbase = type->to<IR::ISimpleNamespace>();
        auto decl = fbase->getDeclByName(expression->member.name);
        if (decl == nullptr)
            return expression;
        auto ftype = getType(decl->getNode());
        if (ftype == nullptr)
            return expression;
        if (isCompileTimeConstant(expression->expr)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
        setType(getOriginal(), ftype);
        setType(expression, ftype);
    } else {
        typeError("Cannot extract field %1% from %2% which has type %3%",
                  expression->member, expression->expr, type->toString());
    }

    return expression;
}

const IR::Node* TypeInference::preorder(IR::MethodCallExpression* expression) {
    // enable method resolution based on number of arguments
    methodArguments.push_back(expression->arguments->size());
    return expression;
}

// If inActionList this call is made in the "action" property of a table
const IR::Expression*
TypeInference::actionCall(bool inActionList,
                          const IR::MethodCallExpression* actionCall) {
    // If a is an action with signature _(arg1, arg2, arg3)
    // Then the call a(arg1, arg2) is also an
    // action, with signature _(arg3)
    LOG1("Processing action " << actionCall);
    auto method = actionCall->method;
    auto methodType = getType(method);
    if (!methodType->is<IR::Type_Action>())
        typeError("%1%: must be an action", method);
    auto baseType = methodType->to<IR::Type_Action>();
    BUG_CHECK(method->is<IR::PathExpression>(), "%1%: unexpected call", method);
    auto arguments = actionCall->arguments;
    BUG_CHECK(baseType->returnType == nullptr,
              "%1%: action with return type?", baseType->returnType);
    if (!baseType->typeParameters->empty())
        typeError("%1%: Cannot supply type parameters for an action invocation",
                  baseType->typeParameters);

    TypeConstraints constraints;
    auto params = new IR::IndexedVector<IR::Parameter>();
    auto it = arguments->begin();
    for (auto p : *baseType->parameters->parameters) {
        LOG2("Action parameter " << p);
        if (it == arguments->end()) {
            params->push_back(p);
            if (p->direction != IR::Direction::None)
                typeError("%1%: parameter %2% must be bound", actionCall, p);
        } else {
            auto arg = *it;
            auto paramType = getType(p);
            auto argType = getType(arg);
            constraints.addEqualityConstraint(paramType, argType);
            if (p->direction == IR::Direction::None) {
                if (inActionList)
                    typeError("%1%: parameter %2% cannot be bound: it is set by the control plane",
                              arg, p);
                if (!isCompileTimeConstant(arg))
                    typeError("%1%: must be a compile-time constant", arg);
            } else if (p->direction == IR::Direction::Out ||
                       p->direction == IR::Direction::InOut) {
                if (!isLeftValue(arg))
                    typeError("%1%: must be a left-value", arg);
            }
            it++;
        }
    }
    if (it != arguments->end())
        typeError("%1% Too many parameters for action", *it);
    auto pl = new IR::ParameterList(Util::SourceInfo(), params);
    auto resultType = new IR::Type_Action(baseType->srcInfo, baseType->typeParameters, nullptr, pl);

    setType(getOriginal(), resultType);
    setType(actionCall, resultType);
    auto subst = constraints.solve(actionCall, true);
    if (subst == nullptr)
        return actionCall;
    ConstantTypeSubstitution cts(subst, typeMap);
    actionCall = cts.convert(actionCall)->to<IR::MethodCallExpression>();  // cast arguments
    LOG1("Converted action " << actionCall);
    setType(actionCall, resultType);
    return actionCall;
}

const IR::Node* TypeInference::postorder(IR::MethodCallExpression* expression) {
    if (done()) return expression;
    methodArguments.pop_back();

    LOG1("Solving method call " << expression);
    auto methodType = getType(expression->method);
    if (methodType == nullptr)
        return expression;
    auto ft = methodType->to<IR::Type_MethodBase>();
    if (ft == nullptr) {
        typeError("%1% is not a method", expression);
        return expression;
    }

    // Handle differently methods and actions: action invocations return actions
    // with different signatures
    if (methodType->is<IR::Type_Action>()) {
        bool inActionsList = false;
        auto prop = findContext<IR::TableProperty>();
        if (prop != nullptr && prop->name == IR::TableProperties::actionsPropertyName)
            inActionsList = true;
        return actionCall(inActionsList, expression);
    } else {
        // We build a type for the callExpression and unify it with the method expression
        // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
        auto rettype = new IR::Type_Var(Util::SourceInfo(), IR::ID(refMap->newName("R")));
        auto args = new IR::Vector<IR::ArgumentInfo>();
        for (auto arg : *expression->arguments) {
            auto argType = getType(arg);
            if (argType == nullptr)
                return expression;
            auto argInfo = new IR::ArgumentInfo(arg->srcInfo, isLeftValue(arg),
                                                isCompileTimeConstant(arg), argType);
            args->push_back(argInfo);
        }
        auto typeArgs = new IR::Vector<IR::Type>();
        for (auto ta : *expression->typeArguments) {
            auto taType = getType(ta);
            if (taType == nullptr)
                return expression;
            typeArgs->push_back(taType);
        }
        auto callType = new IR::Type_MethodCall(expression->srcInfo,
                                                typeArgs, rettype, args);

        TypeConstraints constraints;
        constraints.addEqualityConstraint(ft, callType);
        auto subst = constraints.solve(expression, true);
        if (subst == nullptr)
            return expression;

        LOG1("Method type before specialization " << methodType);
        IR::TypeVariableSubstitutionVisitor substVisitor(subst);
        auto specMethodType = methodType->apply(substVisitor);

        // construct types for the specMethodType, use a new typeChecker
        // that uses the same tables!
        TypeInference helper(refMap, typeMap, false, true);
        (void)specMethodType->apply(helper);  // TODO: should this be set as the type of the method?

        auto canon = getType(specMethodType);
        if (canon == nullptr)
            return expression;

        auto functionType = specMethodType->to<IR::Type_MethodBase>();
        BUG_CHECK(functionType != nullptr, "Method type is %1%", specMethodType);
        LOG1("Method type after specialization " << specMethodType);

        if (!functionType->is<IR::Type_Method>())
            BUG("Unexpected type for function %1%", functionType);

        auto returnType = subst->lookup(rettype);
        if (returnType == nullptr) {
            typeError("Cannot infer return type %1%", expression);
            return expression;
        }

        setType(getOriginal(), returnType);
        setType(expression, returnType);
        ConstantTypeSubstitution cts(subst, typeMap);
        auto result = cts.convert(expression)->to<IR::MethodCallExpression>();  // cast arguments

        setType(result, returnType);
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::ConstructorCallExpression* expression) {
    if (done()) return expression;
    auto type = getType(expression->constructedType);
    if (type == nullptr)
        return expression;

    auto simpleType = type;
    if (type->is<IR::Type_SpecializedCanonical>())
        simpleType = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (simpleType->is<IR::Type_Extern>()) {
        auto args = checkExternConstructor(expression, simpleType->to<IR::Type_Extern>(),
                                           expression->arguments);
        if (args == nullptr)
            return expression;
        if (args != expression->arguments)
            expression = new IR::ConstructorCallExpression(expression->srcInfo,
                                                           expression->constructedType, args);
        setType(getOriginal(), type);
        setType(expression, type);
    } else if (simpleType->is<IR::IContainer>()) {
        const IR::Type* conttype = containerInstantiation(expression, expression->arguments,
                                                          simpleType->to<IR::IContainer>());
        if (conttype == nullptr)
            return expression;
        if (type->is<IR::Type_SpecializedCanonical>()) {
            auto st = type->to<IR::Type_SpecializedCanonical>();
            conttype = new IR::Type_SpecializedCanonical(type->srcInfo, st->baseType,
                                                         st->arguments, conttype);
        }
        setType(expression, conttype);
        setType(getOriginal(), conttype);
    } else {
        typeError("%1%: Cannot invoke a constructor on type %2%",
                  expression, type->toString());
    }

    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::SelectCase*
TypeInference::matchCase(const IR::SelectExpression* select, const IR::Type_Tuple* selectType,
                         const IR::SelectCase* selectCase, const IR::Type* caseType) const {
    // The selectType is always a tuple
    // If the caseType is a set type, we unify the type of the set elements
    if (caseType->is<IR::Type_Set>())
        caseType = caseType->to<IR::Type_Set>()->elementType;
    // The caseType may be a simple type, and then we have to unwrap the selectType
    if (caseType->is<IR::Type_Dontcare>())
        return selectCase;

    const IR::Type* useSelType = selectType;
    if (!caseType->is<IR::Type_Tuple>()) {
        if (selectType->components->size() != 1) {
            typeError("Type mismatch %1% (%2%) vs %3% (%4%)",
                      select->select, selectType->toString(), selectCase, caseType->toString());
            return nullptr;
        }
        useSelType = selectType->components->at(0);
    }
    auto tvs = unify(select, useSelType, caseType, true);
    if (tvs == nullptr)
        return nullptr;
    ConstantTypeSubstitution cts(tvs, typeMap);
    auto ks = cts.convert(selectCase->keyset);
    if (ks != selectCase->keyset)
        selectCase = new IR::SelectCase(selectCase->srcInfo, ks, selectCase->state);
    return selectCase;
}

const IR::Node* TypeInference::postorder(IR::This* expression) {
    if (done()) return expression;
    auto decl = findContext<IR::Declaration_Instance>();
    if (findContext<IR::Function>() == nullptr || decl == nullptr)
        typeError("%1%: can only be used in the definition of an abstract method", expression);
    auto type = getType(decl);
    setType(expression, type);
    setType(getOriginal(), type);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::DefaultExpression* expression) {
    if (!done()) {
        setType(expression, IR::Type_Dontcare::get());
        setType(getOriginal(), IR::Type_Dontcare::get());
    }
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node* TypeInference::postorder(IR::SelectExpression* expression) {
    if (done()) return expression;
    auto selectType = getType(expression->select);
    if (selectType == nullptr)
        return expression;

    // Check that the selectType is determined
    if (!selectType->is<IR::Type_Tuple>())
        BUG("%1%: Expected a tuple type for the select expression, got %2%",
            expression, selectType);
    auto tuple = selectType->to<IR::Type_Tuple>();
    for (auto ct : *tuple->components) {
        if (ct->is<IR::ITypeVar>()) {
            typeError("Cannot infer type for %1%", ct);
            return expression;
        }
    }

    bool changes = false;
    auto vec = new IR::Vector<IR::SelectCase>();
    for (auto sc : *expression->selectCases)  {
        auto type = getType(sc->keyset);
        if (type == nullptr)
            return expression;
        auto newsc = matchCase(expression, tuple, sc, type);
        vec->push_back(newsc);
        if (newsc != sc)
            changes = true;
    }
    if (changes)
        expression = new IR::SelectExpression(expression->srcInfo, expression->select, vec);
    setType(expression, IR::Type_State::get());
    return expression;
}

///////////////////////////////////////// Statements et al.

const IR::Node* TypeInference::postorder(IR::IfStatement* conditional) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto type = getType(conditional->condition);
    if (type == nullptr)
        return conditional;
    if (!type->is<IR::Type_Boolean>())
        typeError("Condition of %1% does not evaluate to a bool but %2%",
                  conditional, type->toString());
    return conditional;
}

const IR::Node* TypeInference::postorder(IR::SwitchStatement* stat) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto type = getType(stat->expression);
    if (type == nullptr)
        return stat;
    if (!type->is<IR::Type_ActionEnum>()) {
        typeError("%1%: Switch condition can only be produced by table.apply()", stat);
        return stat;
    }
    auto ae = type->to<IR::Type_ActionEnum>();
    std::set<cstring> foundLabels;
    for (auto c : *stat->cases) {
        if (c->label->is<IR::DefaultExpression>())
            continue;
        auto pe = c->label->to<IR::PathExpression>();
        CHECK_NULL(pe);
        cstring label = pe->path->name.name;
        if (foundLabels.find(label) != foundLabels.end())
            typeError("%1%: duplicate switch label", c->label);
        foundLabels.emplace(label);
        if (!ae->contains(label))
            typeError("%1% is not a legal label (action name)", c->label);
    }
    return stat;
}

const IR::Node* TypeInference::postorder(IR::ReturnStatement* statement) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto func = findOrigCtxt<IR::Function>();
    if (func == nullptr) {
        if (statement->expression != nullptr)
            typeError("%1%: return with expression can only be used in a function", statement);
        return statement;
    }

    auto ftype = getType(func);
    if (ftype == nullptr)
        return statement;

    BUG_CHECK(ftype->is<IR::Type_Method>(), "%1%: expected a method type for function", ftype);
    auto mt = ftype->to<IR::Type_Method>();
    auto returnType = mt->returnType;
    CHECK_NULL(returnType);
    if (returnType->is<IR::Type_Void>()) {
        if (statement->expression != nullptr)
            typeError("%1%: return expression in function with void return", statement);
        return statement;
    }

    if (statement->expression == nullptr) {
        typeError("%1%: return with no expression in a function returning %2%",
                  statement, returnType->toString());
        return statement;
    }

    auto init = assignment(statement, returnType, statement->expression);
    if (init != statement->expression)
        statement->expression = init;
    return statement;
}

const IR::Node* TypeInference::postorder(IR::AssignmentStatement* assign) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto ltype = getType(assign->left);
    if (ltype == nullptr)
        return assign;

    if (!isLeftValue(assign->left)) {
        typeError("Expression %1% cannot be the target of an assignment", assign->left);
        LOG1(assign->left);
        return assign;
    }

    auto newInit = assignment(assign, ltype, assign->right);
    if (newInit != assign->right)
        assign = new IR::AssignmentStatement(assign->srcInfo, assign->left, newInit);
    return assign;
}

const IR::Node* TypeInference::postorder(IR::ActionListElement* elem) {
    if (done()) return elem;
    auto type = getType(elem->expression);
    if (type == nullptr)
        return elem;

    setType(elem, type);
    setType(getOriginal(), type);
    return elem;
}

const IR::Node* TypeInference::postorder(IR::SelectCase* sc) {
    auto type = getType(sc->state);
    if (type != nullptr && type != IR::Type_State::get())
        typeError("%1% must be state", sc);
    return sc;
}

const IR::Node* TypeInference::postorder(IR::KeyElement* elem) {
    auto ktype = getType(elem->expression);
    if (!ktype->is<IR::Type_Bits>() && !ktype->is<IR::Type_Enum>() &&
        !ktype->is<IR::Type_Error>() && !ktype->is<IR::Type_Boolean>())
        typeError("Key %1% field type must be a scalar type; it cannot be %2%",
                  elem->expression, ktype->toString());
    auto type = getType(elem->matchType);
    if (type != nullptr && type != IR::Type_MatchKind::get())
        typeError("%1% must be a %2% value", elem->matchType,
                  IR::Type_MatchKind::get()->toString());
    return elem;
}

const IR::Node* TypeInference::postorder(IR::TableProperty* prop) {
    if (prop->name == IR::TableProperties::defaultActionPropertyName) {
        auto pv = prop->value->to<IR::ExpressionValue>();
        if (pv == nullptr) {
            typeError("%1% table property should be an action", prop);
        } else {
            auto type = getType(pv->expression);
            if (type == nullptr)
                return prop;
            if (!type->is<IR::Type_Action>()) {
                typeError("%1% table property should be an action", prop);
                return prop;
            }
            auto at = type->to<IR::Type_Action>();
            if (at->parameters->size() != 0)
                typeError("Action for %1% has some unbound arguments", prop->value);
        }
    }
    return prop;
}

}  // namespace P4
