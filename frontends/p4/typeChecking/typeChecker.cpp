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

#include "lib/log.h"
#include "typeChecker.h"
#include "typeUnification.h"
#include "typeSubstitution.h"
#include "typeConstraints.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/toP4/toP4.h"
#include "syntacticEquivalence.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

namespace {
// Used to set the type of Constants after type inference
class ConstantTypeSubstitution : public Transform {
    TypeVariableSubstitution* subst;
    TypeMap                 * typeMap;
    TypeInference           * tc;

 public:
    ConstantTypeSubstitution(TypeVariableSubstitution* subst,
                             ReferenceMap*,
                             TypeMap* typeMap,
                             TypeInference* tc) :
        subst(subst), typeMap(typeMap), tc(tc) {
        CHECK_NULL(subst); CHECK_NULL(typeMap);
        CHECK_NULL(tc);
        LOG3("ConstantTypeSubstitution " << subst); }
    const IR::Node* postorder(IR::Constant* cst) override {
        auto cstType = typeMap->getType(getOriginal(), true);
        if (!cstType->is<IR::ITypeVar>())
            return cst;
        auto repl = cstType;
        while (repl != nullptr && repl->is<IR::ITypeVar>())
            repl = subst->get(repl->to<IR::ITypeVar>());
        if (repl != nullptr && !repl->is<IR::ITypeVar>()) {
            // maybe the substitution could not infer a width...
            LOG2("Inferred type " << repl << " for " << cst);
            cst = new IR::Constant(cst->srcInfo, repl, cst->value, cst->base);
        } else {
            LOG2("No type inferred for " << cst << " repl is " << repl);
        }
        return cst;
    }

    const IR::Expression* convert(const IR::Expression* expr) {
        auto result = expr->apply(*this)->to<IR::Expression>();
        if (result != expr && (::errorCount() == 0))
            tc->learn(result, this);
        return result;
    }
    const IR::Vector<IR::Expression>* convert(const IR::Vector<IR::Expression>* vec) {
        auto result = vec->apply(*this)->to<IR::Vector<IR::Expression>>();
        if (result != vec)
            tc->learn(result, this);
        return result;
    }
    const IR::Vector<IR::Argument>* convert(const IR::Vector<IR::Argument>* vec) {
        auto result = vec->apply(*this)->to<IR::Vector<IR::Argument>>();
        if (result != vec)
            tc->learn(result, this);
        return result;
    }
};
}  // namespace

TypeChecking::TypeChecking(ReferenceMap* refMap, TypeMap* typeMap,
                           bool updateExpressions) {
    addPasses({
       new P4::ResolveReferences(refMap),
       new P4::TypeInference(refMap, typeMap, true),
       updateExpressions ? new ApplyTypesToExpressions(typeMap) : nullptr,
       updateExpressions ? new P4::ResolveReferences(refMap) : nullptr });
    setStopOnError(true);
}

//////////////////////////////////////////////////////////////////////////

bool TypeInference::learn(const IR::Node* node, Visitor* caller) {
    auto *learner = clone();
    learner->setCalledBy(caller);
    unsigned previous = ::errorCount();
    (void)node->apply(*learner);
    unsigned errCount = ::errorCount();
    bool result = errCount > previous;
    if (result)
        typeError("Error while analyzing %1%", node);
    return result;
}

const IR::Expression* TypeInference::constantFold(const IR::Expression* expression) {
    if (readOnly)
        return expression;
    DoConstantFolding cf(refMap, typeMap, false);
    auto result = expression->apply(cf);
    LOG3("Folded " << expression << " into " << result);
    return result;
}

// Make a clone of the type where all type variables in
// the type parameters are replaced with fresh ones.
// This should only be applied to canonical types.
const IR::Type* TypeInference::cloneWithFreshTypeVariables(const IR::IMayBeGenericType* type) {
    TypeVariableSubstitution tvs;
    for (auto v : type->getTypeParameters()->parameters) {
        auto tv = new IR::Type_Var(v->srcInfo, v->getName());
        bool b = tvs.setBinding(v, tv);
        BUG_CHECK(b, "%1%: failed replacing %2% with %3%", type, v, tv);
    }

    TypeVariableSubstitutionVisitor sv(&tvs, true);
    sv.setCalledBy(this);
    auto cl = type->to<IR::Type>()->apply(sv);
    CHECK_NULL(cl);
    learn(cl, this);
    LOG3("Cloned for type variables " << type << " into " << cl);
    return cl->to<IR::Type>();
}

TypeInference::TypeInference(ReferenceMap* refMap, TypeMap* typeMap,
                             bool readOnly, bool checkArrays) :
        refMap(refMap), typeMap(typeMap),
        initialNode(nullptr), readOnly(readOnly), checkArrays(checkArrays) {
    CHECK_NULL(typeMap);
    CHECK_NULL(refMap);
    visitDagOnce = false;  // the done() method will take care of this
}

Visitor::profile_t TypeInference::init_apply(const IR::Node* node) {
    if (node->is<IR::P4Program>()) {
        LOG3("Reference map for type checker:" << std::endl << refMap);
        LOG2("TypeInference for " << dbp(node));
    }
    initialNode = node;
    refMap->validateMap(node);
    return Transform::init_apply(node);
}

void TypeInference::end_apply(const IR::Node* node) {
    if (readOnly && !(*node == *initialNode)) {
        BUG("At this point in the compilation typechecking "
            "should not infer new types anymore, but it did.");
    }
    typeMap->updateMap(node);
    if (node->is<IR::P4Program>())
        LOG3("Typemap: " << std::endl << typeMap);
}

TypeInference *TypeInference::clone() const {
    return new TypeInference(this->refMap, this->typeMap, true);
}

bool TypeInference::done() const {
    auto orig = getOriginal();
    bool done = typeMap->contains(orig);
    LOG3("TI Visiting " << dbp(orig) << (done ? " done" : ""));
    return done;
}

const IR::Type* TypeInference::getType(const IR::Node* element) const {
    const IR::Type* result = typeMap->getType(element);
    // This should be happening only when type-checking already failed
    // for some node; we are now just trying to typecheck a parent node.
    // So an error should have already been signalled.
    if ((result == nullptr) && (::errorCount() == 0))
        BUG("Could not find type of %1% %2%", dbp(element), element);
    return result;
}

const IR::Type* TypeInference::getTypeType(const IR::Node* element) const {
    const IR::Type* result = typeMap->getType(element);
    // See comment in getType() above.
    if (result == nullptr) {
        if (::errorCount() == 0)
            BUG("Could not find type of %1%", element);
        return nullptr;
    }
    BUG_CHECK(result->is<IR::Type_Type>(), "%1%: expected a TypeType", dbp(result));
    return result->to<IR::Type_Type>()->type;
}

void TypeInference::setType(const IR::Node* element, const IR::Type* type) {
    typeMap->setType(element, type);
}

void TypeInference::addSubstitutions(const TypeVariableSubstitution* tvs) {
    typeMap->addSubstitutions(tvs);
}

TypeVariableSubstitution* TypeInference::unify(
    const IR::Node* errorPosition, const IR::Type* destType, const IR::Type* srcType,
    cstring errorFormat, std::initializer_list<const IR::Node*> errorArgs) {
    CHECK_NULL(destType); CHECK_NULL(srcType);
    if (srcType == destType)
        return new TypeVariableSubstitution();

    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    auto constraint = new EqualityConstraint(destType, srcType, errorPosition);
    if (!errorFormat.isNullOrEmpty())
        constraint->setError(errorFormat, errorArgs);
    constraints.add(constraint);
    auto tvs = constraints.solve();
    addSubstitutions(tvs);
    return tvs;
}

const IR::Type*
TypeInference::canonicalizeFields(
    const IR::Type_StructLike* type,
    std::function<const IR::Type*(const IR::IndexedVector<IR::StructField>*)> constructor) {
    bool changes = false;
    auto fields = new IR::IndexedVector<IR::StructField>();
    for (auto field : type->fields) {
        auto ftype = canonicalize(field->type);
        if (ftype == nullptr)
            return nullptr;
        if (ftype != field->type)
            changes = true;
        BUG_CHECK(!ftype->is<IR::Type_Type>(), "%1%: TypeType in field type", ftype);
        auto newField = new IR::StructField(field->srcInfo, field->name, field->annotations, ftype);
        fields->push_back(newField);
    }
    if (changes)
        return constructor(fields);
    else
        return type;
}

const IR::ParameterList* TypeInference::canonicalizeParameters(const IR::ParameterList* params) {
    if (params == nullptr)
        return params;

    bool changes = false;
    auto vec = new IR::IndexedVector<IR::Parameter>();
    for (auto p : *params->getEnumerator()) {
        auto paramType = getTypeType(p->type);
        if (paramType == nullptr)
            return nullptr;
        BUG_CHECK(!paramType->is<IR::Type_Type>(), "%1%: Unexpected parameter type", paramType);
        if (paramType != p->type) {
            p = new IR::Parameter(p->srcInfo, p->name, p->annotations,
                                  p->direction, paramType, p->defaultValue);
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

bool TypeInference::checkParameters(
    const IR::ParameterList* paramList, bool forbidModules, bool forbidPackage) const {
    for (auto p : paramList->parameters) {
        auto type = getType(p);
        if (type == nullptr)
            return false;
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
            (type->is<IR::Type_Parser>() ||
             type->is<IR::Type_Control>() ||
             type->is<IR::P4Parser>() ||
             type->is<IR::P4Control>())) {
            typeError("%1%: parameter cannot have type %2%", p, type);
            return false;
        }
    }
    return true;
}

/**
 * Bind the parameters with the specified arguments.
 * For example, given a type
 * void _<T>(T data)
 * it can be specialized to
 * void _<int<32>>(int<32> data);
 */
const IR::Type* TypeInference::specialize(const IR::IMayBeGenericType* type,
                                          const IR::Vector<IR::Type>* arguments) {
    TypeVariableSubstitution* bindings = new TypeVariableSubstitution();
    bool success = bindings->setBindings(type->getNode(), type->getTypeParameters(), arguments);
    if (!success)
        return nullptr;

    LOG2("Translation map\n" << bindings);

    TypeVariableSubstitutionVisitor tsv(bindings);
    const IR::Node* result = type->getNode()->apply(tsv);
    if (result == nullptr)
        return nullptr;

    LOG2("Specialized " << type << "\n\tinto " << result);
    return result->to<IR::Type>();
}

// May return nullptr if a type error occurs.
const IR::Type* TypeInference::canonicalize(const IR::Type* type) {
    if (type == nullptr)
        return nullptr;

    auto exists = typeMap->getType(type);
    if (exists != nullptr) {
        if (exists->is<IR::Type_Type>())
            return exists->to<IR::Type_Type>()->type;
        return exists;
    }
    if (auto tt = type->to<IR::Type_Type>())
        type = tt->type;

    if (type->is<IR::Type_SpecializedCanonical>() ||
        type->is<IR::Type_InfInt>() ||
        type->is<IR::Type_Action>() ||
        type->is<IR::Type_Error>() ||
        type->is<IR::Type_Newtype>()) {
        return type;
    } else if (type->is<IR::Type_Dontcare>()) {
        return IR::Type_Dontcare::get();
    } else if (type->is<IR::Type_Base>()) {
        if (!type->is<IR::Type_Bits>())
            // all other base types are singletons
            return type;
        auto tb = type->to<IR::Type_Bits>();
        auto canon = IR::Type_Bits::get(tb->size, tb->isSigned);
        if (!typeMap->contains(canon))
            typeMap->setType(canon, new IR::Type_Type(canon));
        return canon;
    } else if (type->is<IR::Type_Enum>() ||
               type->is<IR::Type_SerEnum>() ||
               type->is<IR::Type_ActionEnum>() ||
               type->is<IR::Type_MatchKind>()) {
        return type;
    } else if (auto set = type->to<IR::Type_Set>()) {
        auto et = canonicalize(set->elementType);
        if (et == nullptr)
            return nullptr;
        if (et->is<IR::Type_Stack>() || et->is<IR::Type_Set>() ||
            et->is<IR::Type_HeaderUnion>())
            ::error(ErrorType::ERR_TYPE_ERROR,
                    "%1%: Sets of %2% are not supported", type, et);
        if (et == set->elementType)
            return type;
        const IR::Type *canon = new IR::Type_Set(type->srcInfo, et);
        return canon;
    } else if (auto stack = type->to<IR::Type_Stack>()) {
        auto et = canonicalize(stack->elementType);
        if (et == nullptr)
            return nullptr;
        const IR::Type* canon;
        if (et == stack->elementType)
            canon = type;
        else
            canon = new IR::Type_Stack(stack->srcInfo, et, stack->size);
        canon = typeMap->getCanonical(canon);
        return canon;
    } else if (auto list = type->to<IR::Type_List>()) {
        auto fields = new IR::Vector<IR::Type>();
        // list<set<a>, b> = set<tuple<a, b>>
        // TODO: this should not be done here.
        bool anySet = false;
        bool anyChange = false;
        for (auto t : list->components) {
            if (t->is<IR::Type_Set>()) {
                anySet = true;
                t = t->to<IR::Type_Set>()->elementType;
            }
            auto t1 = canonicalize(t);
            anyChange = anyChange || t != t1;
            if (t1 == nullptr)
                return nullptr;
            fields->push_back(t1);
        }
        const IR::Type *canon;
        if (anySet)
            canon = new IR::Type_Tuple(type->srcInfo, *fields);
        else if (anyChange)
            canon = new IR::Type_List(type->srcInfo, *fields);
        else
            canon = type;
        canon = typeMap->getCanonical(canon);
        if (anySet)
            canon = new IR::Type_Set(type->srcInfo, canon);
        return canon;
    } else if (auto tuple = type->to<IR::Type_Tuple>()) {
        auto fields = new IR::Vector<IR::Type>();
        bool anyChange = false;
        for (auto t : tuple->components) {
            auto t1 = canonicalize(t);
            anyChange = anyChange || t != t1;
            if (t1 == nullptr)
                return nullptr;
            fields->push_back(t1);
        }
        const IR::Type *canon;
        if (anyChange)
            canon = new IR::Type_Tuple(type->srcInfo, *fields);
        else
            canon = type;
        canon = typeMap->getCanonical(canon);
        return canon;
    } else if (auto tp = type->to<IR::Type_Parser>()) {
        auto pl = canonicalizeParameters(tp->applyParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages))
            return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Parser(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto tp = type->to<IR::Type_Control>()) {
        auto pl = canonicalizeParameters(tp->applyParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages))
            return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Control(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto tp = type->to<IR::Type_Package>()) {
        auto pl = canonicalizeParameters(tp->constructorParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (pl != tp->constructorParams || tps != tp->typeParameters)
            return new IR::Type_Package(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto cont = type->to<IR::P4Control>()) {
        auto ctype0 = getTypeType(cont->type);
        if (ctype0 == nullptr)
            return nullptr;
        auto ctype = ctype0->to<IR::Type_Control>();
        auto pl = canonicalizeParameters(cont->constructorParams);
        if (pl == nullptr)
            return nullptr;
        if (ctype != cont->type || pl != cont->constructorParams)
            return new IR::P4Control(cont->srcInfo, cont->name,
                                     ctype, pl, cont->controlLocals, cont->body);
        return type;
    } else if (auto p = type->to<IR::P4Parser>()) {
        auto ctype0 = getTypeType(p->type);
        if (ctype0 == nullptr)
            return nullptr;
        auto ctype = ctype0->to<IR::Type_Parser>();
        auto pl = canonicalizeParameters(p->constructorParams);
        if (pl == nullptr)
            return nullptr;
        if (ctype != p->type || pl != p->constructorParams)
            return new IR::P4Parser(p->srcInfo, p->name, ctype,
                                    pl, p->parserLocals, p->states);
        return type;
    } else if (auto te = type->to<IR::Type_Extern>()) {
        bool changes = false;
        auto methods = new IR::Vector<IR::Method>();
        for (auto method : te->methods) {
            auto fpType = canonicalize(method->type);
            if (fpType == nullptr)
                return nullptr;

            if (fpType != method->type) {
                method = new IR::Method(method->srcInfo, method->name,
                                        fpType->to<IR::Type_Method>(), method->isAbstract,
                                        method->annotations);
                changes = true;
                setType(method, fpType);
            }

            methods->push_back(method);
        }
        auto tps = te->typeParameters;
        if (tps == nullptr)
            return nullptr;
        const IR::Type* resultType = type;
        if (changes || tps != te->typeParameters)
            resultType = new IR::Type_Extern(te->srcInfo, te->name, tps, *methods);
        return resultType;
    } else if (auto mt = type->to<IR::Type_Method>()) {
        const IR::Type* res = nullptr;
        if (mt->returnType != nullptr) {
            res = canonicalize(mt->returnType);
            if (res == nullptr)
                return nullptr;
        }
        bool changes = res != mt->returnType;
        auto pl = canonicalizeParameters(mt->parameters);
        auto tps = mt->typeParameters;
        if (pl == nullptr)
            return nullptr;
        if (!checkParameters(pl))
            return nullptr;
        changes = changes || pl != mt->parameters || tps != mt->typeParameters;
        const IR::Type* resultType = mt;
        if (changes)
            resultType = new IR::Type_Method(mt->getSourceInfo(), tps, res, pl, mt->name);
        return resultType;
    } else if (auto hdr = type->to<IR::Type_Header>()) {
        return canonicalizeFields(hdr, [hdr](const IR::IndexedVector<IR::StructField>* fields) {
            return new IR::Type_Header(
                hdr->srcInfo, hdr->name, hdr->annotations, hdr->typeParameters, *fields);
            });
    } else if (auto str = type->to<IR::Type_Struct>()) {
        return canonicalizeFields(str, [str](const IR::IndexedVector<IR::StructField>* fields) {
                return new IR::Type_Struct(
                    str->srcInfo, str->name, str->annotations, str->typeParameters, *fields);
            });
    } else if (auto hu = type->to<IR::Type_HeaderUnion>()) {
        return canonicalizeFields(hu, [hu](const IR::IndexedVector<IR::StructField>* fields) {
                return new IR::Type_HeaderUnion(
                    hu->srcInfo, hu->name, hu->annotations, hu->typeParameters, *fields);
            });
    } else if (auto su = type->to<IR::Type_UnknownStruct>()) {
        return canonicalizeFields(su, [su](const IR::IndexedVector<IR::StructField>* fields) {
                return new IR::Type_UnknownStruct(su->srcInfo, su->name, su->annotations, *fields);
            });
    } else if (auto st = type->to<IR::Type_Specialized>()) {
        auto baseCanon = canonicalize(st->baseType);
        if (baseCanon == nullptr)
            return nullptr;
        if (st->arguments == nullptr)
            return baseCanon;

        if (!baseCanon->is<IR::IMayBeGenericType>()) {
            typeError("%1%: Type %2% is not generic and thus it"
                      " cannot be specialized using type arguments", type, baseCanon);
            return nullptr;
        }

        auto gt = baseCanon->to<IR::IMayBeGenericType>();
        auto tp = gt->getTypeParameters();
        if (tp->size() != st->arguments->size()) {
            typeError("%1%: Type %2% has %3% type parameter(s), but it is specialized with %4%",
                      type, gt, tp->size(), st->arguments->size());
            return nullptr;
        }

        IR::Vector<IR::Type> *args = new IR::Vector<IR::Type>();
        for (const IR::Type* a : *st->arguments) {
            auto atype = getTypeType(a);
            if (atype == nullptr)
                return nullptr;
            auto checkType = atype;
            if (auto tsc = atype->to<IR::Type_SpecializedCanonical>())
                checkType = tsc->baseType;
            if (checkType->is<IR::Type_Control>() ||
                checkType->is<IR::Type_Parser>() ||
                checkType->is<IR::Type_Package>() ||
                checkType->is<IR::P4Parser>() ||
                checkType->is<IR::P4Control>()) {
                typeError("%1%: Cannot use %2% as a type parameter", type, checkType);
                return nullptr;
            }
            const IR::Type* canon = canonicalize(atype);
            if (canon == nullptr)
                return nullptr;
            args->push_back(canon);
        }
        auto specialized = specialize(gt, args);

        auto result = new IR::Type_SpecializedCanonical(
            type->srcInfo, baseCanon, args, specialized);
        LOG2("Scanning the specialized type");
        learn(result, this);
        return result;
    } else {
        BUG_CHECK(::errorCount(), "Unexpected type %1%", dbp(type));
    }

    // If we reach this point some type error must have occurred, because
    // the typeMap lookup at the beginning of the function has failed.
    return nullptr;
}

///////////////////////////////////// Visitor methods

const IR::Node* TypeInference::preorder(IR::P4Program* program) {
    if (typeMap->checkMap(getOriginal()) && readOnly) {
        LOG2("No need to typecheck");
        prune();
    }
    return program;
}

const IR::Node* TypeInference::postorder(IR::Type_Error* decl) {
    (void)setTypeType(decl);
    for (auto id : *decl->getDeclarations())
        setType(id->getNode(), decl);
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Declaration_MatchKind* decl) {
    if (done()) return decl;
    for (auto id : *decl->getDeclarations())
        setType(id->getNode(), IR::Type_MatchKind::get());
    return decl;
}

const IR::Node* TypeInference::postorder(IR::P4Table* table) {
    if (done()) return table;
    auto type = new IR::Type_Table(table);
    setType(getOriginal(), type);
    setType(table, type);
    return table;
}

const IR::Node* TypeInference::postorder(IR::P4Action* action) {
    if (done()) return action;
    auto pl = canonicalizeParameters(action->parameters);
    if (pl == nullptr)
        return action;
    if (!checkParameters(action->parameters, forbidModules, forbidPackages))
        return action;
    auto type = new IR::Type_Action(new IR::TypeParameters(), nullptr, pl);

    bool foundDirectionless = false;
    for (auto p : action->parameters->parameters) {
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
    auto type = getTypeType(decl->type);
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

    const IR::Type* baseType = type;
    if (auto sc = type->to<IR::Type_SpecializedCanonical>())
        baseType = sc->baseType;
    if (baseType->is<IR::IContainer>() || baseType->is<IR::Type_Extern>()) {
        typeError("%1%: cannot declare variables of type %2% (consider using an instantiation)",
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
            decl->type = type;
            decl->initializer = init;
            LOG2("Created new declaration " << decl);
        }
    }
    setType(decl, type);
    setType(orig, type);
    return decl;
}

bool TypeInference::canCastBetween(const IR::Type* dest, const IR::Type* src) const {
    if (src->is<IR::Type_Action>())
        return false;
    if (src == dest)
        return true;

    if (dest->is<IR::Type_Newtype>()) {
        auto dt = getTypeType(dest->to<IR::Type_Newtype>()->type);
        if (typeMap->equivalent(dt, src))
            return true;
    }

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
        } else if (auto de = dest->to<IR::Type_SerEnum>()) {
            return typeMap->equivalent(src, getTypeType(de->type));
        } else if (dest->is<IR::Type_InfInt>()) {
            return true;
        }
    } else if (src->is<IR::Type_Boolean>()) {
        if (dest->is<IR::Type_Bits>()) {
            auto b = dest->to<IR::Type_Bits>();
            return b->size == 1 && !b->isSigned;
        }
    } else if (src->is<IR::Type_InfInt>()) {
        return dest->is<IR::Type_Bits>() ||
                dest->is<IR::Type_Boolean>() ||
                dest->is<IR::Type_InfInt>();
    } else if (src->is<IR::Type_Newtype>()) {
        auto st = getTypeType(src->to<IR::Type_Newtype>()->type);
        return typeMap->equivalent(dest, st);
    } else if (auto se = src->to<IR::Type_SerEnum>()) {
        auto set = getTypeType(se->type);
        if (auto db = dest->to<IR::Type_Bits>()) {
            return typeMap->equivalent(set, db);
        }
        if (auto de = dest->to<IR::Type_SerEnum>()) {
            return typeMap->equivalent(set, getTypeType(de->type));
        }
    }
    return false;
}

const IR::Expression*
TypeInference::assignment(const IR::Node* errorPosition, const IR::Type* destType,
                          const IR::Expression* sourceExpression) {
    if (destType->is<IR::Type_Unknown>())
        BUG("Unknown destination type");
    if (destType->is<IR::Type_Dontcare>())
        return sourceExpression;
    const IR::Type* initType = getType(sourceExpression);
    if (initType == nullptr)
        return sourceExpression;

    auto tvs = unify(errorPosition, destType, initType,
                     "Source expression '%1%' produces a result of type '%2%' which cannot be "
                     "assigned to a left-value with type '%3%'",
                     { sourceExpression, initType, destType });
    if (tvs == nullptr)
        // error already signalled
        return sourceExpression;
    if (!tvs->isIdentity()) {
        ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
        sourceExpression = cts.convert(sourceExpression);  // sets type
    }
    if (destType->is<IR::Type_SerEnum>() &&
        !typeMap->equivalent(destType, initType)) {
        typeError("%1%: values of type '%2%' cannot be implicitly cast to type '%3%'",
                  errorPosition, initType, destType);
        return sourceExpression;
    }

    if (initType->is<IR::Type_InfInt>() && !destType->is<IR::Type_InfInt>()) {
        auto toType = destType->getP4Type();
        sourceExpression = new IR::Cast(toType, sourceExpression);
        setType(toType, new IR::Type_Type(destType));
        setType(sourceExpression, destType);
        setCompileTimeConstant(sourceExpression);
    }
    auto concreteType = destType;
    if (auto tsc = destType->to<IR::Type_SpecializedCanonical>())
        concreteType = tsc->substituted;
    bool cst = isCompileTimeConstant(sourceExpression);
    if (auto ts = concreteType->to<IR::Type_StructLike>()) {
        auto si = sourceExpression->to<IR::StructExpression>();
        auto type = destType->getP4Type();
        if (initType->is<IR::Type_UnknownStruct>() ||
            (si != nullptr && initType->is<IR::Type_Struct>())) {
            // Even if the structure is a struct expression with the right type,
            // we still need to recurse over its fields; they many not have
            // the right type.
            CHECK_NULL(si);
            if (ts->fields.size() != si->components.size()) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, ts->fields.size(), si->components.size());
                return sourceExpression;
            }
            IR::IndexedVector<IR::NamedExpression> vec;
            bool changes = false;
            for (size_t i = 0; i < ts->fields.size(); i++) {
                auto fieldI = ts->fields.at(i);
                auto compI = si->components.at(i)->expression;
                auto src = assignment(sourceExpression, fieldI->type, compI);
                if (src != compI)
                    changes = true;
                vec.push_back(new IR::NamedExpression(fieldI->name, src));
            }
            if (!changes)
                vec = si->components;
            if (initType->is<IR::Type_UnknownStruct>() || changes) {
                sourceExpression = new IR::StructExpression(type, type, vec);
                setType(sourceExpression, destType);
            }
        } else if (auto li = sourceExpression->to<IR::ListExpression>()) {
            if (ts->fields.size() != li->components.size()) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, ts->fields.size(), li->components.size());
                return sourceExpression;
            }
            IR::IndexedVector<IR::NamedExpression> vec;
            for (size_t i = 0; i < ts->fields.size(); i++) {
                auto fieldI = ts->fields.at(i);
                auto compI = li->components.at(i);
                auto src = assignment(sourceExpression, fieldI->type, compI);
                vec.push_back(new IR::NamedExpression(fieldI->name, src));
            }
            sourceExpression = new IR::StructExpression(type, type, vec);
            setType(sourceExpression, destType);
        }
        // else this is some other expression that evaluates to a struct
        if (cst)
            setCompileTimeConstant(sourceExpression);
    } else if (auto tt = concreteType->to<IR::Type_Tuple>()) {
        if (auto li = sourceExpression->to<IR::ListExpression>()) {
            if (tt->getSize() != li->components.size()) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, tt->getSize(), li->components.size());
                return sourceExpression;
            }
            bool changed = false;
            IR::Vector<IR::Expression> vec;
            for (size_t i = 0; i < tt->getSize(); i++) {
                auto typeI = tt->at(i);
                auto compI = li->components.at(i);
                auto src = assignment(sourceExpression, typeI, compI);
                if (src != compI)
                    changed = true;
                vec.push_back(src);
            }
            if (changed)
                sourceExpression = new IR::ListExpression(vec);
        }
        // else this is some other expression that evaluates to a tuple
        setType(sourceExpression, destType);
        if (cst)
            setCompileTimeConstant(sourceExpression);
    }

    return sourceExpression;
}

const IR::Node* TypeInference::postorder(IR::Annotation* annotation) {
    auto checkAnnotation = [this] (const IR::Expression* e) {
        if (!isCompileTimeConstant(e))
            typeError("%1%: structured annotation must be compile-time constant values", e);
        auto t = getType(e);
        if (!t->is<IR::Type_InfInt>() &&
            !t->is<IR::Type_String>() &&
            !t->is<IR::Type_Boolean>())
            typeError("%1%: illegal type for structured annotation; must be int, string or bool",
                      e);
    };

    if (annotation->structured) {
        // If it happens here it was created in the compiler, so it's a bug, not an error.
        BUG_CHECK(annotation->expr.empty() || annotation->kv.empty(),
                  "%1%: structured annotations cannot contain expressions and kv-pairs",
                  annotation);
        for (auto e : annotation->expr)
            checkAnnotation(e);
        for (auto e : annotation->kv)
            checkAnnotation(e->expression);
    }
    return annotation;
}

const IR::Node* TypeInference::postorder(IR::Declaration_Constant* decl) {
    if (done()) return decl;
    auto type = getTypeType(decl->type);
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
const IR::Vector<IR::Argument> *
TypeInference::checkExternConstructor(const IR::Node* errorPosition,
                                      const IR::Type_Extern* ext,
                                      const IR::Vector<IR::Argument> *arguments) {
    auto constructor = ext->lookupConstructor(arguments);
    if (constructor == nullptr) {
        typeError("%1%: type %2% has no matching constructor",
                  errorPosition, ext);
        return nullptr;
    }
    auto mt = getType(constructor);
    if (mt == nullptr)
        return nullptr;
    auto methodType = mt->to<IR::Type_Method>();
    BUG_CHECK(methodType != nullptr, "Constructor does not have a method type, but %1%", mt);
    methodType = cloneWithFreshTypeVariables(methodType)->to<IR::Type_Method>();
    CHECK_NULL(methodType);

    auto args = new IR::Vector<IR::ArgumentInfo>();
    size_t i = 0;
    for (auto pi : *methodType->parameters->getEnumerator()) {
        if (i >= arguments->size()) {
            BUG_CHECK(pi->isOptional() || pi->defaultValue != nullptr,
                      "Missing nonoptional arg %s", pi);
            break; }
        auto arg = arguments->at(i++);
        if (!isCompileTimeConstant(arg->expression))
            typeError("%1%: cannot evaluate to a compile-time constant", arg);
        auto argType = getType(arg->expression);
        auto paramType = getType(pi);
        if (argType == nullptr || paramType == nullptr)
            return nullptr;

        if (paramType->is<IR::Type_Control>() ||
            paramType->is<IR::Type_Parser>() ||
            paramType->is<IR::P4Control>() ||
            paramType->is<IR::P4Parser>() ||
            paramType->is<IR::Type_Package>())
            typeError("%1%: parameter cannot have type %2%", pi, paramType);

        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg->expression, true, argType, arg);
        args->push_back(argInfo);
    }

    auto rettype = new IR::Type_Var(IR::ID(refMap->newName("R"), "<returned type>"));
    auto callType = new IR::Type_MethodCall(
        errorPosition->srcInfo, new IR::Vector<IR::Type>(), rettype, args);
    auto tvs = unify(errorPosition, mt, callType,
                     "Constructor invocation %1% does not match constructor declaration %2%",
                     { callType, constructor });
    BUG_CHECK(tvs != nullptr || ::errorCount(), "Unification failed with no error");
    if (tvs == nullptr)
        return nullptr;

    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    auto newArgs = cts.convert(arguments);
    return newArgs;
}

// Return true on success
bool TypeInference::checkAbstractMethods(const IR::Declaration_Instance* inst,
                                         const IR::Type_Extern* type) {
    // Make a list of the abstract methods
    IR::NameMap<IR::Method, ordered_map> virt;
    for (auto m : type->methods)
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

    for (auto d : inst->initializer->components) {
        if (d->is<IR::Function>()) {
            auto func = d->to<IR::Function>();
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
            auto tvs = unify(
                inst, methtype, ftype,
                "Method '%1%' does not have the expected type '%2%'",
                { func, methtype });
            if (tvs == nullptr)
                return false;
            BUG_CHECK(errorCount() > 0 || tvs->isIdentity(),
                      "%1%: expected no type variables", tvs);
        }
    }
    bool rv = true;
    for (auto &vm : virt) {
        if (!vm.second->annotations->getSingle("optional")) {
            typeError("%1%: %2% abstract method not implemented", inst, vm.second);
            rv = false; } }
    return rv;
}

const IR::Node* TypeInference::preorder(IR::Declaration_Instance* decl) {
    // We need to control the order of the type-checking: we want to do first
    // the declaration, and then typecheck the initializer if present.
    if (done())
        return decl;
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
    if (type->is<IR::Type_SpecializedCanonical>())
        simpleType = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (auto et = simpleType->to<IR::Type_Extern>()) {
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
        if (decl->initializer != nullptr) {
            typeError("%1%: initializers only allowed for extern instances", decl->initializer);
            prune();
            return decl;
        }
        if (!simpleType->is<IR::Type_Package>() && (findContext<IR::IContainer>() == nullptr)) {
            ::error(ErrorType::ERR_INVALID, "%1%: cannot instantiate at top-level", decl);
            return decl;
        }
        auto typeAndArgs = containerInstantiation(
            decl, decl->arguments, simpleType->to<IR::IContainer>());
        auto type = typeAndArgs.first;
        auto args = typeAndArgs.second;
        if (type == nullptr || args == nullptr) {
            prune();
            return decl;
        }
        learn(type, this);
        if (args != decl->arguments)
            decl->arguments = args;
        setType(decl, type);
        setType(orig, type);
    } else {
        typeError("%1%: cannot allocate objects of type %2%", decl, type);
    }
    prune();
    return decl;
}

/// @returns: A pair containing the type returned by the constructor and the new arguments
///           (which may change due to insertion of casts).
std::pair<const IR::Type*, const IR::Vector<IR::Argument>*>
TypeInference::containerInstantiation(
    const IR::Node* node,  // can be Declaration_Instance or ConstructorCallExpression
    const IR::Vector<IR::Argument>* constructorArguments,
    const IR::IContainer* container) {
    CHECK_NULL(node); CHECK_NULL(constructorArguments); CHECK_NULL(container);
    auto constructor = container->getConstructorMethodType();
    constructor = cloneWithFreshTypeVariables(
        constructor->to<IR::IMayBeGenericType>())->to<IR::Type_Method>();
    CHECK_NULL(constructor);
    bool isPackage = container->is<IR::Type_Package>();
    auto params = constructor->parameters;
    checkParameters(params, !forbidModules, !isPackage);
    // only packages can have packages as parameters

    // We build a type for the callExpression and unify it with the method expression
    // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
    auto args = new IR::Vector<IR::ArgumentInfo>();
    for (auto aarg : *constructorArguments) {
        auto arg = aarg->expression;
        if (!isCompileTimeConstant(arg))
            typeError("%1%: cannot evaluate to a compile-time constant", arg);
        auto argType = getType(arg);
        if (argType == nullptr)
            return std::pair<const IR::Type*, const IR::Vector<IR::Argument>*>(nullptr, nullptr);
        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg, true, argType, aarg);
        args->push_back(argInfo);
    }
    auto rettype = new IR::Type_Var(IR::ID(refMap->newName("<any>")));
    // There are never type arguments at this point; if they exist, they have been folded
    // into the constructor by type specialization.
    auto callType = new IR::Type_MethodCall(
        node->srcInfo, new IR::Vector<IR::Type>(), rettype, args);

    auto tvs = unify(container->getNode(), constructor, callType,
                     "Constructor invocation '%1%' does not match declaration '%2%'",
                     { callType, constructor });
    BUG_CHECK(tvs != nullptr || ::errorCount(), "Null substitution");
    if (tvs == nullptr)
        return std::pair<const IR::Type*, const IR::Vector<IR::Argument>*>(nullptr, nullptr);

    // Infer Dont_Care for type vars used only in not-present optional params
    auto dontCares = new TypeVariableSubstitution();
    auto typeParams = constructor->typeParameters;
    for (auto p : params->parameters) {
        if (!p->isOptional()) continue;
        forAllMatching<IR::Type_Var>(p, [tvs, dontCares, typeParams](const IR::Type_Var *tv) {
            if (tvs->lookup(tv)) return;  // already bound
            if (typeParams->getDeclByName(tv->name) != tv) return;  // not a tv of this call
            dontCares->setBinding(tv, new IR::Type_Dontcare);
        });
    }
    addSubstitutions(dontCares);

    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    auto newArgs = cts.convert(constructorArguments);

    auto returnType = tvs->lookup(rettype);
    BUG_CHECK(returnType != nullptr, "Cannot infer constructor result type %1%", node);
    return std::pair<const IR::Type*, const IR::Vector<IR::Argument>*>(returnType, newArgs);
}

const IR::Node* TypeInference::preorder(IR::Function* function) {
    if (done()) return function;
    visit(function->type);
    auto type = getTypeType(function->type);
    if (type == nullptr)
        return function;
    setType(getOriginal(), type);
    setType(function, type);
    visit(function->body);
    prune();
    return function;
}

const IR::Node* TypeInference::postorder(IR::Argument* arg) {
    if (done()) return arg;
    auto type = getType(arg->expression);
    if (type == nullptr)
        return arg;
    setType(getOriginal(), type);
    setType(arg, type);
    return arg;
}

const IR::Node* TypeInference::postorder(IR::Method* method) {
    if (done()) return method;
    auto type = getTypeType(method->type);
    if (type == nullptr)
        return method;
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

//////////////////////////////////////////////  Types

const IR::Type* TypeInference::setTypeType(const IR::Type* type, bool learn) {
    if (done()) return type;
    const IR::Type* typeToCanonicalize;
    if (readOnly)
        typeToCanonicalize = getOriginal<IR::Type>();
    else
        typeToCanonicalize = type;
    auto canon = canonicalize(typeToCanonicalize);
    if (canon != nullptr) {
        // Learn the new type
        if (canon != typeToCanonicalize && learn) {
            bool errs = this->learn(canon, this);
            if (errs)
                return nullptr;
        }
        auto tt = new IR::Type_Type(canon);
        setType(getOriginal(), tt);
        setType(type, tt);
    }
    return canon;
}

const IR::Node* TypeInference::postorder(IR::Type_Type* type) {
    BUG("Should never be found in IR: %1%", type);
}

const IR::Node* TypeInference::postorder(IR::P4Control* cont) {
    (void)setTypeType(cont, false);
    return cont;
}

const IR::Node* TypeInference::postorder(IR::P4Parser* parser) {
    (void)setTypeType(parser, false);
    return parser;
}

const IR::Node* TypeInference::postorder(IR::Type_InfInt* type) {
    if (done()) return type;
    auto tt = new IR::Type_Type(getOriginal<IR::Type>());
    setType(getOriginal(), tt);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_ArchBlock* decl) {
    (void)setTypeType(decl);
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Package* decl) {
    auto canon = setTypeType(decl);
    if (canon != nullptr) {
        for (auto p : decl->getConstructorParameters()->parameters) {
            auto ptype = getType(p);
            if (ptype == nullptr)
                // error
                return decl;
            if (ptype->is<IR::P4Parser>() || ptype->is<IR::P4Control>())
                typeError("%1%: Invalid package parameter type", p);
        }
    }
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Specialized *type) {
    // Check for recursive type specializations, e.g.,
    // extern e<T> {};  e<e<bit>> x;
    auto ctx = getContext();
    while (ctx) {
        if (auto ts = ctx->node->to<IR::Type_Specialized>()) {
            if (type->baseType->path->equiv(*ts->baseType->path)) {
                typeError("%1%: recursive type specialization", type->baseType);
                return type;
            }
        }
        ctx = ctx->parent;
    }
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_SpecializedCanonical *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Name* typeName) {
    if (done()) return typeName;
    const IR::Type* type;

    if (typeName->path->isDontCare()) {
        auto t = IR::Type_Dontcare::get();
        type = new IR::Type_Type(t);
    } else {
        auto decl = refMap->getDeclaration(typeName->path, true);
        // Check for references of a control or parser within itself.
        auto ctrl = findContext<IR::P4Control>();
        if (ctrl != nullptr && ctrl->name == decl->getName()) {
            typeError("%1%: Cannot refer to control inside itself", typeName);
            return typeName;
        }
        auto parser = findContext<IR::P4Parser>();
        if (parser != nullptr && parser->name == decl->getName()) {
            typeError("%1%: Cannot refer parser inside itself", typeName);
            return typeName;
        }

        type = getType(decl->getNode());
        if (type == nullptr)
            return typeName;
        BUG_CHECK(type->is<IR::Type_Type>(), "%1%: should be a Type_Type", type);
    }
    setType(typeName->path, type->to<IR::Type_Type>()->type);
    setType(getOriginal(), type);
    setType(typeName, type);
    return typeName;
}

const IR::Node* TypeInference::postorder(IR::Type_ActionEnum* type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Enum* type) {
    auto canon = setTypeType(type);
    for (auto e : *type->getDeclarations())
        setType(e->getNode(), canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_SerEnum* type) {
    if (::errorCount())
        // If we failed to typecheck a SerEnumMember we do not
        // want to set the type for the SerEnum either.
        return type;
    auto canon = setTypeType(type);
    for (auto e : *type->getDeclarations())
        setType(e->getNode(), canon);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Var* typeVar) {
    if (done()) return typeVar;
    const IR::Type* type;
    if (typeVar->name.isDontCare())
        type = IR::Type_Dontcare::get();
    else
        type = getOriginal<IR::Type>();
    auto tt = new IR::Type_Type(type);
    setType(getOriginal(), tt);
    setType(typeVar, tt);
    return typeVar;
}

const IR::Node* TypeInference::postorder(IR::Type_List* type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Tuple* type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Set* type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::SerEnumMember* member) {
    if (done())
        return member;
    auto serEnum = findContext<IR::Type_SerEnum>();
    CHECK_NULL(serEnum);
    auto type = getTypeType(serEnum->type);
    if (!type->is<IR::Type_Bits>()) {
        typeError("%1%: Illegal type for enum; only bit<> and int<> are allowed", serEnum->type);
        return member;
    }
    auto exprType = getType(member->value);
    auto tvs = unify(member, type, exprType,
                     "Enum member '%1%' has type '%2%' and not the expected type '%2%'",
                     { member, exprType, type });
    if (tvs == nullptr)
        // error already signalled
        return member;
    if (tvs->isIdentity())
        return member;

    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    member->value = cts.convert(member->value);  // sets type
    return member;
}

const IR::Node* TypeInference::postorder(IR::P4ValueSet* decl) {
    if (done())
        return decl;
    // This is a specialized version of setTypeType
    auto canon = canonicalize(decl->elementType);
    if (canon != nullptr) {
        if (canon != decl->elementType) {
            bool errs = learn(canon, this);
            if (errs)
                return nullptr;
        }
        if (!canon->is<IR::Type_Newtype>() &&
            !canon->is<IR::Type_Bits>() &&
            !canon->is<IR::Type_SerEnum>() &&
            !canon->is<IR::Type_Boolean>() &&
            !canon->is<IR::Type_Enum>() &&
            !canon->is<IR::Type_Struct>() &&
            !canon->is<IR::Type_Tuple>())
            typeError("%1%: Illegal type for value_set element type", decl->elementType);

        auto tt = new IR::Type_Set(canon);
        setType(getOriginal(), tt);
        setType(decl, tt);
    }
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Extern* type) {
    if (done()) return type;
    setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Method* type) {
    auto methodType = type;
    if (auto ext = findContext<IR::Type_Extern>()) {
        auto extName = ext->name.name;
        if (auto method = findContext<IR::Method>()) {
            auto name = method->name.name;
            if (methodType->returnType &&
                (methodType->returnType->is<IR::Type_InfInt>() ||
                 methodType->returnType->is<IR::Type_String>()))
                typeError("%1%: illegal return type for method", method->type->returnType);
            if (name == extName) {
                // This is a constructor.
                if (method->type->typeParameters != nullptr &&
                    method->type->typeParameters->size() > 0) {
                    typeError("%1%: Constructors cannot have type parameters",
                              method->type->typeParameters);
                    return type;
                }
                // For constructors we add the type variables of the
                // enclosing extern as type parameters.  Given
                // extern e<E> { e(); }
                // the type of method e is in fact e<T>();
                methodType = new IR::Type_Method(
                    type->srcInfo, ext->typeParameters, type->returnType, type->parameters, name);
            }
        }
    }
    (void)setTypeType(methodType);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Action* type) {
    (void)setTypeType(type);
    BUG_CHECK(type->typeParameters->size() == 0, "%1%: Generic action?", type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Base* type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Newtype* type) {
    (void)setTypeType(type);
    auto argType = getTypeType(type->type);
    if (!argType->is<IR::Type_Bits>() &&
        !argType->is<IR::Type_Boolean>() &&
        !argType->is<IR::Type_Newtype>())
        typeError("%1%: `type' can only be applied to base types", type);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Typedef* tdecl) {
    if (done()) return tdecl;
    auto type = getType(tdecl->type);
    if (type == nullptr)
        return tdecl;
    BUG_CHECK(type->is<IR::Type_Type>(), "%1%: expected a TypeType", type);
    auto stype = type->to<IR::Type_Type>()->type;
    if (auto gen = stype->to<IR::IMayBeGenericType>()) {
        if (gen->getTypeParameters()->size() != 0) {
            typeError("%1%: no type parameters supplied for generic type", tdecl->type);
            return tdecl;
        }
    }
    setType(getOriginal(), type);
    setType(tdecl, type);
    return tdecl;
}

const IR::Node* TypeInference::postorder(IR::Type_Stack* type) {
    auto canon = setTypeType(type);
    if (canon == nullptr)
        return type;

    auto etype = canon->to<IR::Type_Stack>()->elementType;
    if (etype == nullptr)
        return type;

    if (!etype->is<IR::Type_Header>() && !etype->is<IR::Type_HeaderUnion>() &&
        !etype->is<IR::Type_SpecializedCanonical>())
        typeError("Header stack %1% used with non-header type %2%",
                  type, etype->toString());
    return type;
}

/// Validate the fields of a struct type using the supplied checker.
/// The checker returns "false" when a field is invalid.
/// Return true on success
bool TypeInference::validateFields(const IR::Type* type,
                                   std::function<bool(const IR::Type*)> checker) const {
    if (type == nullptr)
        return false;
    BUG_CHECK(type->is<IR::Type_StructLike>(), "%1%; expected a Struct-like", type);
    auto strct = type->to<IR::Type_StructLike>();
    bool err = false;
    for (auto field : strct->fields) {
        auto ftype = getType(field);
        if (ftype == nullptr)
            return false;
        if (!checker(ftype)) {
            typeError("Field '%1%' of '%2%' cannot have type '%3%'",
                      field, type->toString(), field->type);
            err = true;
        }
    }
    return !err;
}

const IR::Node* TypeInference::postorder(IR::StructField* field) {
    if (done()) return field;
    auto canon = getTypeType(field->type);
    if (canon == nullptr)
        return field;

    setType(getOriginal(), canon);
    setType(field, canon);
    return field;
}

const IR::Node* TypeInference::postorder(IR::Type_Header* type) {
    auto canon = setTypeType(type);
    auto validator = [this] (const IR::Type* t) {
        while (t->is<IR::Type_Newtype>())
            t = getTypeType(t->to<IR::Type_Newtype>()->type);
        return t->is<IR::Type_Bits>() || t->is<IR::Type_Varbits>() ||
               (t->is<IR::Type_Struct>() && onlyBitsOrBitStructs(t)) ||
               t->is<IR::Type_SerEnum>() || t->is<IR::Type_Boolean>() ||
                t->is<IR::Type_Var>() || t->is<IR::Type_SpecializedCanonical>(); };
    validateFields(canon, validator);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Struct* type) {
    auto canon = setTypeType(type);
    auto validator = [this] (const IR::Type* t) {
        while (t->is<IR::Type_Newtype>())
            t = getTypeType(t->to<IR::Type_Newtype>()->type);
        return t->is<IR::Type_Struct>() || t->is<IR::Type_Bits>() ||
        t->is<IR::Type_Header>() || t->is<IR::Type_HeaderUnion>() ||
        t->is<IR::Type_Enum>() || t->is<IR::Type_Error>() ||
        t->is<IR::Type_Boolean>() || t->is<IR::Type_Stack>() ||
        t->is<IR::Type_Varbits>() || t->is<IR::Type_ActionEnum>() ||
        t->is<IR::Type_Tuple>() || t->is<IR::Type_SerEnum>() ||
        t->is<IR::Type_Var>() || t->is<IR::Type_SpecializedCanonical>() ||
        t->is<IR::Type_MatchKind>(); };
    (void)validateFields(canon, validator);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_HeaderUnion *type) {
    auto canon = setTypeType(type);
    auto validator = [] (const IR::Type* t) { return t->is<IR::Type_Header>() ||
        t->is<IR::Type_Var>() || t->is<IR::Type_SpecializedCanonical>(); };
    (void)validateFields(canon, validator);
    return type;
}

////////////////////////////////////////////////// expressions

const IR::Node* TypeInference::postorder(IR::Parameter* param) {
    if (done()) return param;
    const IR::Type* paramType = getTypeType(param->type);
    if (paramType == nullptr)
        return param;
    BUG_CHECK(!paramType->is<IR::Type_Type>(), "%1%: unexpected type", paramType);

    if (paramType->is<IR::P4Control>() || paramType->is<IR::P4Parser>()) {
        typeError("%1%: parameter cannot have type %2%", param, paramType);
        return param;
    }

    if (!readOnly && paramType->is<IR::Type_InfInt>()) {
        // We only give these errors if we are no in 'readOnly' mode:
        // this prevents giving a confusing error message to the user.
        if (param->direction != IR::Direction::None) {
            typeError("%1%: parameters with type %2% must be directionless",
                      param, paramType);
            return param;
        }
        if (findContext<IR::P4Action>()) {
            typeError("%1%: actions cannot have parameters with type %2%",
                      param, paramType);
            return param;
        }
    }

    // The parameter type cannot have free type variables
    if (paramType->is<IR::IMayBeGenericType>()) {
        auto gen = paramType->to<IR::IMayBeGenericType>();
        auto tp = gen->getTypeParameters();
        if (!tp->empty()) {
            typeError("Type parameters needed for %1%", param->name);
            return param;
        }
    }

    if (param->defaultValue) {
        if (!typeMap->isCompileTimeConstant(param->defaultValue))
            typeError("%1%: expression must be a compile-time constant",
                      param->defaultValue);
    }

    setType(getOriginal(), paramType);
    setType(param, paramType);
    return param;
}

const IR::Node* TypeInference::postorder(IR::Constant* expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->type);
    if (type == nullptr)
        return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    setCompileTimeConstant(expression);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::StringLiteral* expression) {
    if (done()) return expression;
    setType(getOriginal(), IR::Type_String::get());
    setType(expression, IR::Type_String::get());
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
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

// Returns nullptr on error
bool TypeInference::compare(const IR::Node* errorPosition,
                            const IR::Type* ltype,
                            const IR::Type* rtype,
                            Comparison* compare) {
    if (ltype->is<IR::Type_Action>() || rtype->is<IR::Type_Action>()) {
        // Actions return Type_Action instead of void.
        typeError("%1%: cannot be applied to action results", errorPosition);
        return false;
    }

    bool defined = false;
    if (typeMap->equivalent(ltype, rtype) &&
        (!ltype->is<IR::Type_Void>() && !ltype->is<IR::Type_Varbits>())
         && !ltype->to<IR::Type_UnknownStruct>()) {
        defined = true;
    } else if (ltype->is<IR::Type_Base>() && rtype->is<IR::Type_Base>() &&
               typeMap->equivalent(ltype, rtype)) {
        defined = true;
    } else if (ltype->is<IR::Type_BaseList>() && rtype->is<IR::Type_BaseList>()) {
        auto tvs = unify(errorPosition, ltype, rtype);
        if (tvs == nullptr)
            return false;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
            compare->left = cts.convert(compare->left);
            compare->right = cts.convert(compare->right);
        }
        defined = true;
    } else {
        auto ls = ltype->to<IR::Type_UnknownStruct>();
        auto rs = rtype->to<IR::Type_UnknownStruct>();
        if (ls != nullptr || rs != nullptr) {
            if (ls != nullptr && rs != nullptr) {
                typeError("%1%: cannot compare structure-valued expressions with unknown types",
                          errorPosition);
                return false;
            }

            bool lcst = isCompileTimeConstant(compare->left);
            bool rcst = isCompileTimeConstant(compare->right);
            TypeVariableSubstitution* tvs;
            if (ls == nullptr) {
                tvs = unify(errorPosition, ltype, rtype);
            } else {
                tvs = unify(errorPosition, rtype, ltype);
            }
            if (tvs == nullptr)
                return false;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
                compare->left = cts.convert(compare->left);
                compare->right = cts.convert(compare->right);
            }

            if (ls != nullptr) {
                auto l = compare->left->to<IR::StructExpression>();
                CHECK_NULL(l);  // struct initializers are the only expressions that can
                // have StructUnknown types
                BUG_CHECK(rtype->is<IR::Type_StructLike>(), "%1%: expected a struct", rtype);
                auto type = new IR::Type_Name(rtype->to<IR::Type_StructLike>()->name);
                compare->left = new IR::StructExpression(
                    compare->left->srcInfo, type, type, l->components);
                setType(compare->left, rtype);
                if (lcst)
                    setCompileTimeConstant(compare->left);
            } else {
                auto r = compare->right->to<IR::StructExpression>();
                CHECK_NULL(r);  // struct initializers are the only expressions that can
                // have StructUnknown types
                BUG_CHECK(ltype->is<IR::Type_StructLike>(), "%1%: expected a struct", ltype);
                auto type = new IR::Type_Name(ltype->to<IR::Type_StructLike>()->name);
                compare->right = new IR::StructExpression(
                    compare->right->srcInfo, type, type, r->components);
                setType(compare->right, rtype);
                if (rcst)
                    setCompileTimeConstant(compare->right);
            }
            defined = true;
        }

        // comparison between structs and list expressions is allowed
        if ((ltype->is<IR::Type_StructLike>() && rtype->is<IR::Type_List>()) ||
            (ltype->is<IR::Type_List>() && rtype->is<IR::Type_StructLike>())) {
            if (!ltype->is<IR::Type_StructLike>()) {
                // swap
                auto type = ltype;
                ltype = rtype;
                rtype = type;
            }

            auto tvs = unify(errorPosition, ltype, rtype);
            if (tvs == nullptr)
                return false;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
                compare->left = cts.convert(compare->left);
                compare->right = cts.convert(compare->right);
            }
            defined = true;
        }
    }

    if (!defined) {
        typeError("%1%: not defined on %2% and %3%",
                  errorPosition, ltype->toString(), rtype->toString());
        return false;
    }
    return true;
}

const IR::Node* TypeInference::postorder(IR::Operation_Relation* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    bool equTest = expression->is<IR::Equ>() || expression->is<IR::Neq>();
    if (auto l = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(l->type);
    if (auto r = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(r->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_InfInt>()) {
        // This can happen because we are replacing some constant functions with
        // constants during type checking
        auto result = constantFold(expression);
        setType(getOriginal(), IR::Type_Boolean::get());
        setType(result, IR::Type_Boolean::get());
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    } else if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_Bits>()) {
        auto e = expression->clone();
        auto cst = expression->left->to<IR::Constant>();
        CHECK_NULL(cst);
        e->left = new IR::Constant(cst->srcInfo, rtype, cst->value, cst->base);
        setType(e->left, rtype);
        ltype = rtype;
        expression = e;
    } else if (rtype->is<IR::Type_InfInt>() && ltype->is<IR::Type_Bits>()) {
        auto e = expression->clone();
        auto cst = expression->right->to<IR::Constant>();
        CHECK_NULL(cst);
        e->right = new IR::Constant(cst->srcInfo, ltype, cst->value, cst->base);
        setType(e->right, ltype);
        rtype = ltype;
        expression = e;
    }

    if (equTest) {
        Comparison c;
        c.left = expression->left;
        c.right = expression->right;
        auto b = compare(expression, ltype, rtype, &c);
        if (!b)
            return expression;
        expression->left = c.left;
        expression->right = c.right;
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
    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(se->type);
    if (ltype == nullptr || rtype == nullptr) {
        // getTypeType should have already taken care of the error message
        return expression;
    }
    if (!ltype->is<IR::Type_Bits>() || !rtype->is<IR::Type_Bits>()) {
        typeError("%1%: Concatenation not defined on %2% and %3%",
                  expression, ltype->toString(), rtype->toString());
        return expression;
    }
    auto bl = ltype->to<IR::Type_Bits>();
    auto br = rtype->to<IR::Type_Bits>();
    const IR::Type* resultType = IR::Type_Bits::get(bl->size + br->size, bl->isSigned);
    resultType = canonicalize(resultType);
    if (resultType != nullptr) {
        setType(getOriginal(), resultType);
        setType(expression, resultType);
        if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
    }
    return expression;
}

/**
 * compute the type of table keys.
 * Used to typecheck pre-defined entries.
 */
const IR::Node* TypeInference::postorder(IR::Key* key) {
    // compute the type and store it in typeMap
    auto keyTuple = new IR::Type_Tuple;
    for (auto ke : key->keyElements) {
        auto kt = typeMap->getType(ke->expression);
        if (kt == nullptr) {
            LOG2("Bailing out for " << dbp(ke));
            return key;
        }
        keyTuple->components.push_back(kt);
    }
    LOG2("Setting key type to " << dbp(keyTuple));
    setType(key, keyTuple);
    // installing also for the original because we cannot tell which one will survive in the ir
    LOG2("Setting key type to " << dbp(getOriginal()));
    setType(getOriginal(), keyTuple);
    return key;
}

/**
 *  typecheck a table initializer entry list
 */
const IR::Node* TypeInference::preorder(IR::EntriesList* el) {
    if (done()) return el;
    auto table = findContext<IR::P4Table>();
    BUG_CHECK(table != nullptr, "%1% entries not within a table", el);
    const IR::Key* key = table->getKey();
    if (key == nullptr) {
        typeError("Could not find key for table %1%", table);
        prune();
        return el;
    }
    auto keyTuple = typeMap->getType(key);  // direct typeMap call to skip checks
    if (keyTuple == nullptr) {
        // The keys have to be before the entries list.  If they are not,
        // at this point they have not yet been type-checked.
        if (key->srcInfo.isValid() && el->srcInfo.isValid() && key->srcInfo >= el->srcInfo) {
            typeError("%1%: Entries list must be after table key %2%",
                      el, key);
            prune();
            return el;
        }
        // otherwise the type-checking of the keys must have failed
    }
    return el;
}

/**
 *  typecheck a table initializer entry
 *
 *  The invariants are:
 *  - table keys and entry keys must have the same length
 *  - entry key elements must be compile time constants
 *  - actionRefs in entries must be in the action list
 *  - table keys must have been type checked before entries
 *
 *  Moreover, the EntriesList visitor should have checked for the table
 *  invariants.
 */
const IR::Node* TypeInference::postorder(IR::Entry* entry) {
    if (done()) return entry;
    auto table = findContext<IR::P4Table>();
    if (table == nullptr)
        return entry;
    const IR::Key* key = table->getKey();
    if (key == nullptr)
        return entry;
    auto keyTuple = getType(key);
    if (keyTuple == nullptr)
        return entry;

    auto entryKeyType = getType(entry->keys);
    if (entryKeyType == nullptr)
        return entry;
    if (entryKeyType->is<IR::Type_Set>())
        entryKeyType = entryKeyType->to<IR::Type_Set>()->elementType;

    auto keyset = entry->getKeys();
    if (keyset == nullptr || !(keyset->is<IR::ListExpression>())) {
        typeError("%1%: key expression must be tuple", keyset);
        return entry;
    } else if (keyset->components.size() < key->keyElements.size()) {
        typeError("%1%: Size of entry keyset must match the table key set size", keyset);
        return entry;
    }

    bool nonConstantKeys = false;
    for (auto ke : keyset->components)
        if (!isCompileTimeConstant(ke)) {
            typeError("Key entry must be a compile time constant: %1%", ke);
            nonConstantKeys = true;
        }
    if (nonConstantKeys)
        return entry;

    TypeVariableSubstitution *tvs = unify(
        entry, keyTuple, entryKeyType,
        "Table entry has type '%1%' which is not the expected type '%2%'",
        { keyTuple, entryKeyType });
    if (tvs == nullptr)
        return entry;
    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    auto ks = cts.convert(keyset);
    if (::errorCount() > 0)
        return entry;

    if (ks != keyset)
        entry = new IR::Entry(entry->srcInfo, entry->annotations,
                              ks->to<IR::ListExpression>(), entry->action);

    auto actionRef = entry->getAction();
    auto ale = validateActionInitializer(actionRef, table);
    if (ale != nullptr) {
        auto anno = ale->getAnnotation(IR::Annotation::defaultOnlyAnnotation);
        if (anno != nullptr) {
            typeError("%1%: Action marked with %2% used in table",
                      entry, IR::Annotation::defaultOnlyAnnotation);
            return entry;
        }
    }
    return entry;
}

const IR::Node* TypeInference::postorder(IR::ListExpression* expression) {
    if (done()) return expression;
    bool constant = true;
    auto components = new IR::Vector<IR::Type>();
    for (auto c : expression->components) {
        if (!isCompileTimeConstant(c))
            constant = false;
        auto type = getType(c);
        if (type == nullptr)
            return expression;
        components->push_back(type);
    }

    auto tupleType = new IR::Type_List(expression->srcInfo, *components);
    auto type = canonicalize(tupleType);
    if (type == nullptr)
        return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::StructExpression* expression) {
    if (done()) return expression;
    bool constant = true;
    auto components = new IR::IndexedVector<IR::StructField>();
    for (auto c : expression->components) {
        if (!isCompileTimeConstant(c->expression))
            constant = false;
        auto type = getType(c->expression);
        if (type == nullptr)
            return expression;
        components->push_back(new IR::StructField(c->name, type));
    }

    // This is the type inferred by looking at the fields.
    const IR::Type* structType = new IR::Type_UnknownStruct(
        expression->srcInfo, "unknown struct", *components);
    structType = canonicalize(structType);

    const IR::Expression* result = expression;
    if (expression->structType != nullptr) {
        // We know the exact type of the initializer
        auto desired = getTypeType(expression->structType);
        if (desired == nullptr)
            return expression;
        auto tvs = unify(expression, desired, structType,
                         "Initializer type '%1%' does not match expected type '%2%'",
                         { structType, desired });
        if (tvs == nullptr)
            return expression;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
            result = cts.convert(expression);
        }
        structType = desired;
    }
    setType(getOriginal(), structType);
    setType(expression, structType);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return result;
}

const IR::Node* TypeInference::postorder(IR::ArrayIndex* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;
    auto hst = ltype->to<IR::Type_Stack>();

    int index = -1;
    if (auto cst = expression->right->to<IR::Constant>()) {
        if (hst && checkArrays && !cst->fitsInt()) {
            typeError("Index too large: %1%", cst);
            return expression;
        }
        index = cst->asInt();
        if (hst && checkArrays && index < 0) {
            typeError("%1%: Negative array index %2%", expression, cst);
            return expression;
        }
    }
    // if index is negative here it means it's not a constant

    if ((index < 0) && !rtype->is<IR::Type_Bits>()
        && !rtype->is<IR::Type_SerEnum>()
        && !rtype->is<IR::Type_InfInt>()) {
        typeError("Array index %1% must be an integer, but it has type %2%",
                  expression->right, rtype->toString());
        return expression;
    }

    const IR::Type* type = nullptr;
    if (hst) {
        if (checkArrays && hst->sizeKnown()) {
            int size = hst->getSize();
            if (index >= 0 && index >= size) {
                typeError("Array index %1% larger or equal to array size %2%",
                          expression->right, hst->size);
                return expression;
            }
        }
        type = hst->elementType;
    } else if (auto tup = ltype->to<IR::Type_Tuple>()) {
        if (index < 0) {
            typeError("Tuple index %1% must be constant", expression->right);
            return expression;
        }
        if (static_cast<size_t>(index) >= tup->getSize()) {
            typeError("Tuple index %1% larger than tuple size %2%",
                      expression->right, tup->getSize());
            return expression;
        }
        type = tup->components.at(index);
        if (isCompileTimeConstant(expression->left)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
    } else {
        typeError("Indexing %1% applied to non-array and non-tuple type %2%",
                  expression, ltype->toString());
        return expression;
    }
    if (isLeftValue(expression->left)) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    }
    setType(getOriginal(), type);
    setType(expression, type);
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

    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(se->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    } else if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_InfInt>()) {
        auto t = new IR::Type_InfInt();
        auto result = constantFold(expression);
        setType(result, t);
        setType(getOriginal(), t);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
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
        auto e = expression->clone();
        auto cst = expression->left->to<IR::Constant>();
        CHECK_NULL(cst);
        e->left = new IR::Constant(cst->srcInfo, rtype, cst->value, cst->base);
        setType(e->left, rtype);
        expression = e;
        resultType = rtype;
        setType(expression, resultType);
    } else if (bl != nullptr && br == nullptr) {
        auto e = expression->clone();
        auto cst = expression->right->to<IR::Constant>();
        CHECK_NULL(cst);
        e->right = new IR::Constant(cst->srcInfo, ltype, cst->value, cst->base);
        setType(e->right, ltype);
        expression = e;
        resultType = ltype;
        setType(expression, resultType);
    } else {
        setType(expression, resultType);
    }
    setType(getOriginal(), resultType);
    setType(expression, resultType);
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

    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(se->type);

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
        if (cleft->value < 0) {
            typeError("%1%: not defined on negative numbers", expression);
            return expression;
        }
    }
    auto cright = expression->right->to<IR::Constant>();
    if (cright != nullptr) {
        if (cright->value < 0) {
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

    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (ltype == nullptr) {
        // getTypeType should have already taken care of the error message
        return expression;
    }
    auto lt = ltype->to<IR::Type_Bits>();
    if (auto cst = expression->right->to<IR::Constant>()) {
        if (!cst->fitsInt()) {
            typeError("Shift amount too large: %1%", cst);
            return expression;
        }
        int shift = cst->asInt();
        if (shift < 0) {
            typeError("%1%: Negative shift amount %2%", expression, cst);
            return expression;
        }
        if (lt != nullptr && shift >= lt->size)
            warn(ErrorType::WARN_OVERFLOW, "%1%: shifting value with %2% bits by %3%",
                 expression, lt->size, shift);
        // If the amount is signed but positive, make it unsigned
        if (auto bt = rtype->to<IR::Type_Bits>()) {
            if (bt->isSigned) {
                rtype = new IR::Type_Bits(rtype->srcInfo, bt->width_bits(), false);
                auto amt = new IR::Constant(cst->srcInfo, rtype, cst->value, cst->base);
                if (expression->is<IR::Shl>()) {
                    expression = new IR::Shl(expression->srcInfo, expression->left, amt);
                } else {
                    expression = new IR::Shr(expression->srcInfo, expression->left, amt);
                }
                setCompileTimeConstant(expression->right);
                setType(expression->right, rtype);
            }
        }
    }

    if (rtype->is<IR::Type_Bits>() && rtype->to<IR::Type_Bits>()->isSigned) {
        typeError("%1%: Shift amount must be an unsigned number",
                  expression->right);
        return expression;
    }

    if (!lt && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1% left operand of shift must be a numeric type, not %2%",
                  expression, ltype->toString());
        return expression;
    }

    if (ltype->is<IR::Type_InfInt>() && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: width of left operand of shift needs to be specified", expression);
        return expression;
    }

    setType(expression, ltype);
    setType(getOriginal(), ltype);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        auto result = constantFold(expression);
        setType(result, ltype);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::bitwise(const IR::Operation_Binary* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(se->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expressio '%2%' with type '%3%'",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    } else if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_InfInt>()) {
        auto t = new IR::Type_InfInt();
        setType(getOriginal(), t);
        auto result = constantFold(expression);
        setType(result, t);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }

    const IR::Type* resultType = ltype;
    if (bl != nullptr && br != nullptr) {
        if (!typeMap->equivalent(bl, br)) {
            typeError("%1%: Cannot operate on values with different types %2% and %3%",
                      expression, bl->toString(), br->toString());
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto e = expression->clone();
        auto cst = expression->left->to<IR::Constant>();
        CHECK_NULL(cst);
        e->left = new IR::Constant(cst->srcInfo, rtype, cst->value, cst->base);
        setType(e->left, rtype);
        setCompileTimeConstant(e->left);
        expression = e;
        resultType = rtype;
    } else if (bl != nullptr && br == nullptr) {
        auto e = expression->clone();
        auto cst = expression->right->to<IR::Constant>();
        CHECK_NULL(cst);
        e->right = new IR::Constant(cst->srcInfo, ltype, cst->value, cst->base);
        setType(e->right, ltype);
        setCompileTimeConstant(e->right);
        expression = e;
        resultType = ltype;
    }
    setType(expression, resultType);
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

    auto leftType = ltype;  // save original type
    if (auto se = ltype->to<IR::Type_SerEnum>())
        ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>())
        rtype = getTypeType(se->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    // The following section is very similar to "binaryArith()" above
    const IR::Type_Bits* bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits* br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    }

    const IR::Type* sameType = leftType;
    if (bl != nullptr && br != nullptr) {
        if (!typeMap->equivalent(bl, br)) {
            typeError("%1%: Cannot operate on values with different types %2% and %3%",
                      expression, bl->toString(), br->toString());
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto e = expression->clone();
        auto cst = expression->left->to<IR::Constant>();
        e->left = new IR::Constant(cst->srcInfo, rtype, cst->value, cst->base);
        setCompileTimeConstant(e->left);
        expression = e;
        sameType = rtype;
        setType(e->left, sameType);
    } else if (bl != nullptr && br == nullptr) {
        auto e = expression->clone();
        auto cst = expression->right->to<IR::Constant>();
        e->right = new IR::Constant(cst->srcInfo, ltype, cst->value, cst->base);
        setCompileTimeConstant(e->right);
        expression = e;
        setType(e->right, ltype);
        sameType = leftType;  // Not ltype: SerEnum &&& Bit is Set<SerEnum>
    } else {
        // both are InfInt: use same exact type for both sides, so it is properly
        // set after unification
        auto r = expression->right->clone();
        auto e = expression->clone();
        if (isCompileTimeConstant(expression->right))
            setCompileTimeConstant(r);
        e->right = r;
        expression = e;
        setType(r, sameType);
    }

    auto resultType = new IR::Type_Set(sameType->srcInfo, sameType);
    typeMap->setType(expression, resultType);
    typeMap->setType(getOriginal(), resultType);

    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }

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
        auto result = constantFold(expression);
        setType(result, IR::Type_Boolean::get());
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Neg* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (auto se = type->to<IR::Type_SerEnum>())
        type = getTypeType(se->type);
    BUG_CHECK(type, "Invalid Type_SerEnum/getTypeType");

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
        auto result = constantFold(expression);
        setType(result, type);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::UPlus* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (auto se = type->to<IR::Type_SerEnum>())
        type = getTypeType(se->type);
    BUG_CHECK(type, "Invalid Type_SerEnum/getTypeType");

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
        auto result = constantFold(expression);
        setType(result, type);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Cmpl* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;

    if (auto se = type->to<IR::Type_SerEnum>())
        type = getTypeType(se->type);
    BUG_CHECK(type, "Invalid Type_SerEnum/getTypeType");

    if (type->is<IR::Type_InfInt>()) {
        typeError("'%1%' cannot be applied to an operand with an unknown width");
    } else if (type->is<IR::Type_Bits>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else {
        typeError("Cannot apply operation '%1%' to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->expr, type->toString());
    }
    if (isCompileTimeConstant(expression->expr)) {
        auto result = constantFold(expression);
        setType(result, type);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Cast* expression) {
    if (done()) return expression;
    const IR::Type* sourceType = getType(expression->expr);
    const IR::Type* castType = getTypeType(expression->destType);
    if (sourceType == nullptr || castType == nullptr)
        return expression;

    auto concreteType = castType;
    if (auto tsc = castType->to<IR::Type_SpecializedCanonical>())
        concreteType = tsc->substituted;
    if (auto st = concreteType->to<IR::Type_StructLike>()) {
        if (auto se = expression->expr->to<IR::StructExpression>()) {
            // Interpret (S) { kvpairs } as a struct initializer expression
            // instead of a cast to a struct.
            if (se->type == nullptr || se->type->is<IR::Type_Unknown>() ||
                se->type->is<IR::Type_UnknownStruct>()) {
                auto type = castType->getP4Type();
                setType(type, new IR::Type_Type(st));
                auto sie = new IR::StructExpression(
                    se->srcInfo, type, se->components);
                auto result = postorder(sie);  // may insert casts
                setType(result, st);
                return result;
            } else {
                typeError("%1%: cast not supported", expression->destType);
                return expression;
            }
        } else if (auto le = expression->expr->to<IR::ListExpression>()) {
            if (st->fields.size() == le->size()) {
                IR::IndexedVector<IR::NamedExpression> vec;
                for (size_t i = 0; i < st->fields.size(); i++) {
                    auto fieldI = st->fields.at(i);
                    auto compI = le->components.at(i);
                    auto src = assignment(expression, fieldI->type, compI);
                    vec.push_back(new IR::NamedExpression(fieldI->name, src));
                }
                auto setype = castType->getP4Type();
                setType(setype, new IR::Type_Type(st));
                auto result = new IR::StructExpression(
                    le->srcInfo, setype, setype, vec);
                setType(result, st);
                return result;
            } else {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          expression, st->fields.size(), le->components.size());
                return expression;
            }
        }
    }

    if (!castType->is<IR::Type_Bits>() &&
        !castType->is<IR::Type_Boolean>() &&
        !castType->is<IR::Type_Newtype>() &&
        !castType->is<IR::Type_SerEnum>() &&
        !castType->is<IR::Type_InfInt>() &&
        !castType->is<IR::Type_SpecializedCanonical>()) {
        typeError("%1%: cast not supported", expression->destType);
        return expression;
    }

    if (!canCastBetween(castType, sourceType)) {
        // This cast is not legal directly, but let's try to see whether
        // performing a substitution can help.  This will allow the use
        // of constants on the RHS.
        const IR::Type* destType = castType;
        while (destType->is<IR::Type_Newtype>())
            destType = getTypeType(destType->to<IR::Type_Newtype>()->type);

        auto tvs = unify(expression, destType, sourceType,
                         "Cannot cast from '%1%' to '%2%'", { sourceType, castType });
        if (tvs == nullptr)
            return expression;
        const IR::Expression* rhs = expression;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
            rhs = cts.convert(expression->expr);  // sets type
        }
        if (rhs != expression->expr) {
            // if we are here we have performed a substitution on the rhs
            expression = new IR::Cast(expression->srcInfo, expression->destType, rhs);
            sourceType = getTypeType(expression->destType);
        }
        if (!canCastBetween(castType, sourceType))
            typeError("%1%: Illegal cast from %2% to %3%",
                      expression, sourceType->toString(), castType->toString());
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
    auto decl = refMap->getDeclaration(expression->path, true);
    const IR::Type* type = nullptr;
    if (auto tbl = decl->to<IR::P4Table>()) {
        if (auto current = findContext<IR::P4Table>()) {
            if (current->name == tbl->name) {
                typeError("%1%: Cannot refer to the containing table %2%", expression, tbl);
                return expression;
            }
        }
    } else if (decl->is<IR::Type_Enum>() || decl->is<IR::Type_SerEnum>()) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    // For MatchKind and Errors all ids have a type that has been set
    // while processing Type_Error or Declaration_Matchkind
    auto declType = typeMap->getType(decl->getNode());
    if (decl->is<IR::Declaration_ID>() &&
        declType &&
        (declType->is<IR::Type_Error>() || declType->is<IR::Type_MatchKind>())) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }

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
    } else if (decl->is<IR::Method>() || decl->is<IR::Function>()) {
        type = getType(decl->getNode());
        // Each method invocation uses fresh type variables
        if (type != nullptr)
            // may be nullptr because typechecking may have failed
            type = cloneWithFreshTypeVariables(type->to<IR::Type_MethodBase>());
    }

    if (type == nullptr) {
        type = getType(decl->getNode());
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

    if (auto se = type->to<IR::Type_SerEnum>())
        type = getTypeType(se->type);

    if (!type->is<IR::Type_Bits>()) {
        typeError("%1%: bit extraction only defined for bit<> types", expression);
        return expression;
    }

    auto e1type = getType(expression->e1);
    if (e1type->is<IR::Type_SerEnum>()) {
        auto ei = EnumInstance::resolve(expression->e1, typeMap);
        CHECK_NULL(ei);
        auto sei = ei->to<SerEnumInstance>();
        if (sei == nullptr)
            typeError("%1%: slice bit index values must be constants", expression->e1);
        expression->e1 = sei->value;
    }
    auto e2type = getType(expression->e2);
    if (e2type->is<IR::Type_SerEnum>()) {
        auto ei = EnumInstance::resolve(expression->e2, typeMap);
        CHECK_NULL(ei);
        auto sei = ei->to<SerEnumInstance>();
        if (sei == nullptr)
            typeError("%1%: slice bit index values must be constants", expression->e2);
        expression->e2 = sei->value;
    }

    auto bst = type->to<IR::Type_Bits>();
    if (!expression->e1->is<IR::Constant>()) {
        typeError("%1%: slice bit index values must be constants", expression->e1);
        return expression;
    }
    if (!expression->e2->is<IR::Constant>()) {
        typeError("%1%: slice bit index values must be constants", expression->e2);
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
        typeError("%1%: negative bit index %2%", expression, msb);
        return expression;
    }
    if (l < 0) {
        typeError("%1%: negative bit index %2%", expression, lsb);
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

    const IR::Type* resultType = IR::Type_Bits::get(bst->srcInfo, m - l + 1, false);
    resultType = canonicalize(resultType);
    if (resultType == nullptr)
        return expression;
    setType(getOriginal(), resultType);
    setType(expression, resultType);
    if (isLeftValue(expression->e0)) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    }
    if (isCompileTimeConstant(expression->e0)) {
        auto result = constantFold(expression);
        setType(result, resultType);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
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
    auto tvs = unify(expression, secondType, thirdType,
                     "The expressions in a ?: conditional have different types '%1%' and '%2%'",
                     { secondType, thirdType });
    if (tvs != nullptr) {
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
            auto e1 = cts.convert(expression->e1);
            auto e2 = cts.convert(expression->e2);
            if (::errorCount() > 0)
                return expression;
            expression->e1 = e1;
            expression->e2 = e2;
            secondType = typeMap->getType(e1);
        }
        setType(expression, secondType);
        setType(getOriginal(), secondType);
        if (isCompileTimeConstant(expression->e0) &&
            isCompileTimeConstant(expression->e1) &&
            isCompileTimeConstant(expression->e2)) {
            auto result = constantFold(expression);
            setType(result, secondType);
            setCompileTimeConstant(result);
            setCompileTimeConstant(getOriginal<IR::Expression>());
            return result;
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

    cstring member = expression->member.name;
    if (type->is<IR::Type_SpecializedCanonical>())
        type = type->to<IR::Type_SpecializedCanonical>()->substituted;

    if (type->is<IR::Type_Extern>()) {
        auto ext = type->to<IR::Type_Extern>();

        auto call = findContext<IR::MethodCallExpression>();
        if (call == nullptr) {
            typeError("%1%: Methods can only be called", expression);
            return expression;
        }
        auto method = ext->lookupMethod(expression->member, call->arguments);
        if (method == nullptr) {
            typeError("%1%: extern %2% does not have method matching this call",
                      expression, ext->name);
            return expression;
        }

        const IR::Type* methodType = getType(method);
        if (methodType == nullptr)
            return expression;
        // Each method invocation uses fresh type variables
        methodType = cloneWithFreshTypeVariables(methodType->to<IR::IMayBeGenericType>());

        setType(getOriginal(), methodType);
        setType(expression, methodType);
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return expression;
    }

    bool inMethod = getParent<IR::MethodCallExpression>() != nullptr;
    // Built-in methods
    if (inMethod && (member == IR::Type::minSizeInBits ||
                     member == IR::Type::minSizeInBytes ||
                     member == IR::Type::maxSizeInBits ||
                     member == IR::Type::maxSizeInBytes)) {
        auto type = new IR::Type_Method(
            new IR::Type_InfInt(), new IR::ParameterList(), member);
        auto ctype = canonicalize(type);
        if (ctype == nullptr)
            return expression;
        setType(getOriginal(), ctype);
        setType(expression, ctype);
        return expression;
    }

    if (type->is<IR::Type_StructLike>()) {
        cstring typeStr = "structure ";
        if (type->is<IR::Type_Header>() || type->is<IR::Type_HeaderUnion>()) {
            typeStr = "";
            if (inMethod && (member == IR::Type_Header::isValid)) {
                // Built-in method
                auto type = new IR::Type_Method(
                    IR::Type_Boolean::get(), new IR::ParameterList(), member);
                auto ctype = canonicalize(type);
                if (ctype == nullptr)
                    return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }
        if (type->is<IR::Type_Header>()) {
            if (inMethod && (member == IR::Type_Header::setValid ||
                             member == IR::Type_Header::setInvalid)) {
                if (!isLeftValue(expression->expr))
                    typeError("%1%: must be applied to a left-value", expression);
                // Built-in method
                auto type = new IR::Type_Method(
                    IR::Type_Void::get(), new IR::ParameterList, member);
                auto ctype = canonicalize(type);
                if (ctype == nullptr)
                    return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }

        auto stb = type->to<IR::Type_StructLike>();
        auto field = stb->getField(member);
        if (field == nullptr) {
            typeError("Field %1% is not a member of %2%%3%",
                      expression->member, typeStr, stb);
            return expression;
        }

        auto fieldType = getTypeType(field->type);
        if (fieldType == nullptr)
            return expression;
        setType(getOriginal(), fieldType);
        setType(expression, fieldType);
        if (isLeftValue(expression->expr)) {
            setLeftValue(expression);
            setLeftValue(getOriginal<IR::Expression>());
        } else {
            LOG2("No left value " << expression->expr);
        }
        if (isCompileTimeConstant(expression->expr)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
        return expression;
    }

    if (type->is<IR::IApply>() &&
        member == IR::IApply::applyMethodName) {
        auto methodType = type->to<IR::IApply>()->getApplyMethodType();
        methodType = canonicalize(methodType)->to<IR::Type_Method>();
        if (methodType == nullptr)
            return expression;
        learn(methodType, this);
        setType(getOriginal(), methodType);
        setType(expression, methodType);
        return expression;
    }

    if (type->is<IR::Type_Stack>()) {
        auto parser = findContext<IR::P4Parser>();
        if (member == IR::Type_Stack::next ||
            member == IR::Type_Stack::last) {
            if (parser == nullptr) {
                typeError("%1%: 'last' and 'next' for stacks can only be used in a parser",
                          expression);
                return expression;
            }
            auto stack = type->to<IR::Type_Stack>();
            setType(getOriginal(), stack->elementType);
            setType(expression, stack->elementType);
            if (isLeftValue(expression->expr) && member == IR::Type_Stack::next) {
                setLeftValue(expression);
                setLeftValue(getOriginal<IR::Expression>());
            }
            return expression;
        } else if (member == IR::Type_Stack::arraySize) {
            setType(getOriginal(), IR::Type_Bits::get(32));
            setType(expression, IR::Type_Bits::get(32));
            return expression;
        } else if (member == IR::Type_Stack::lastIndex) {
            setType(getOriginal(), IR::Type_Bits::get(32, false));
            setType(expression, IR::Type_Bits::get(32, false));
            return expression;
        } else if (member == IR::Type_Stack::push_front ||
                   member == IR::Type_Stack::pop_front) {
            if (parser != nullptr)
                typeError("%1%: '%2%' and '%3%' for stacks cannot be used in a parser",
                          expression, IR::Type_Stack::push_front, IR::Type_Stack::pop_front);
            if (!isLeftValue(expression->expr))
                typeError("%1%: must be applied to a left-value", expression);
            auto params = new IR::IndexedVector<IR::Parameter>();
            auto param = new IR::Parameter(IR::ID("count", nullptr), IR::Direction::None,
                                           new IR::Type_InfInt());
            auto tt = new IR::Type_Type(param->type);
            setType(param->type, tt);
            setType(param, param->type);
            params->push_back(param);
            auto type = new IR::Type_Method(
                IR::Type_Void::get(), new IR::ParameterList(*params), member);
            auto canon = canonicalize(type);
            if (canon == nullptr)
                return expression;
            setType(getOriginal(), canon);
            setType(expression, canon);
            return expression;
        }
    }

    if (type->is<IR::Type_Type>()) {
        auto base = type->to<IR::Type_Type>()->type;
        if (base->is<IR::Type_Error>() || base->is<IR::Type_Enum>() ||
            base->is<IR::Type_SerEnum>()) {
            if (isCompileTimeConstant(expression->expr)) {
                setCompileTimeConstant(expression);
                setCompileTimeConstant(getOriginal<IR::Expression>()); }
            auto fbase = base->to<IR::ISimpleNamespace>();
            if (auto decl = fbase->getDeclByName(member)) {
                if (auto ftype = getType(decl->getNode())) {
                    setType(getOriginal(), ftype);
                    setType(expression, ftype); }
            } else {
                typeError("%1%: Invalid enum tag", expression);
                setType(getOriginal(), type);
                setType(expression, type); }
            return expression;
        }
    }

    typeError("Cannot extract field %1% from %2% which has type %3%",
              expression->member, expression->expr, type->toString());
    // unreachable
    return expression;
}

// If inActionList this call is made in the "action" property of a table
const IR::Expression*
TypeInference::actionCall(bool inActionList,
                          const IR::MethodCallExpression* actionCall) {
    // If a is an action with signature _(arg1, arg2, arg3)
    // Then the call a(arg1, arg2) is also an
    // action, with signature _(arg3)
    LOG2("Processing action " << dbp(actionCall));

    if (findContext<IR::P4Parser>()) {
        typeError("%1%: Action calls are not allowed within parsers", actionCall);
        return actionCall;
    }
    auto method = actionCall->method;
    auto methodType = getType(method);
    if (!methodType->is<IR::Type_Action>())
        typeError("%1%: must be an action", method);
    auto baseType = methodType->to<IR::Type_Action>();
    LOG2("Action type " << baseType);
    BUG_CHECK(method->is<IR::PathExpression>(), "%1%: unexpected call", method);
    BUG_CHECK(baseType->returnType == nullptr,
              "%1%: action with return type?", baseType->returnType);
    if (!baseType->typeParameters->empty())
        typeError("%1%: Cannot supply type parameters for an action invocation",
                  baseType->typeParameters);

    bool inTable = findContext<IR::P4Table>() != nullptr;

    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    auto params = new IR::ParameterList;

    // keep track of parameters that have not been matched yet
    std::map<cstring, const IR::Parameter*> left;
    for (auto p : baseType->parameters->parameters)
        left.emplace(p->name, p);

    auto paramIt = baseType->parameters->parameters.begin();
    for (auto arg : *actionCall->arguments) {
        cstring argName = arg->name.name;
        bool named = !argName.isNullOrEmpty();
        const IR::Parameter* param;

        if (named) {
            param = baseType->parameters->getParameter(argName);
            if (param == nullptr) {
                typeError("%1%: No parameter named %2%", baseType->parameters, arg->name);
                return actionCall;
            }
        } else {
            if (paramIt == baseType->parameters->parameters.end()) {
                typeError("%1%: Too many arguments for action", actionCall);
                return actionCall;
            }
            param = *paramIt;
        }

        LOG2("Action parameter " << dbp(param));
        auto leftIt = left.find(param->name.name);
        // This shold have been checked by the CheckNamedArgs pass.
        BUG_CHECK(leftIt != left.end(), "%1%: Duplicate argument name?", param->name);
        left.erase(leftIt);

        auto paramType = getType(param);
        auto argType = getType(arg);
        if (paramType == nullptr || argType == nullptr)
            // type checking failed before
            return actionCall;
        constraints.addEqualityConstraint(actionCall, paramType, argType);
        if (param->direction == IR::Direction::None) {
            if (inActionList) {
                typeError("%1%: parameter %2% cannot be bound: it is set by the control plane",
                          arg, param);
            } else if (inTable) {
                // For actions None parameters are treated as IN
                // parameters when the action is called directly.  We
                // don't require them to be bound to a compile-time
                // constant.  But if the action is instantiated in a
                // table (as default_action or entries), then the
                // arguments do have to be compile-time constants.
                if (!isCompileTimeConstant(arg->expression))
                    typeError(
                        "%1%: action argument must be a compile-time constant", arg->expression);
            }
        } else if (param->direction == IR::Direction::Out ||
                   param->direction == IR::Direction::InOut) {
            if (!isLeftValue(arg->expression))
                typeError("%1%: must be a left-value", arg->expression);
        }
        if (!named)
            ++paramIt;
    }

    // Check remaining parameters: they must be all non-directional
    bool error = false;
    for (auto p : left) {
        if (p.second->direction != IR::Direction::None &&
            p.second->defaultValue == nullptr) {
            typeError("%1%: Parameter %2% must be bound", actionCall, p.second);
            error = true;
        }
    }
    if (error)
        return actionCall;

    auto resultType = new IR::Type_Action(baseType->srcInfo, baseType->typeParameters, params);

    setType(getOriginal(), resultType);
    setType(actionCall, resultType);
    auto tvs = constraints.solve();
    if (tvs == nullptr)
        return actionCall;
    addSubstitutions(tvs);

    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    actionCall = cts.convert(actionCall)->to<IR::MethodCallExpression>();  // cast arguments
    if (::errorCount() > 0)
        return actionCall;

    LOG2("Converted action " << actionCall);
    setType(actionCall, resultType);
    return actionCall;
}

bool hasVarbitsOrUnions(const TypeMap* typeMap, const IR::Type* type) {
    // called for a canonical type
    if (type->is<IR::Type_HeaderUnion>() || type->is<IR::Type_Varbits>()) {
        return true;
    } else if (auto ht = type->to<IR::Type_StructLike>()) {
        const IR::StructField* varbit = nullptr;
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr)
                continue;
            if (ftype->is<IR::Type_Varbits>()) {
                if (varbit == nullptr) {
                    varbit = f;
                } else {
                    typeError("%1% and %2%: multiple varbit fields in a header",
                              varbit, f);
                    return type;
                }
            }
        }
        return varbit != nullptr;
    } else if (auto at = type->to<IR::Type_Stack>()) {
        return hasVarbitsOrUnions(typeMap, at->elementType);
    } else if (auto tpl = type->to<IR::Type_Tuple>()) {
        for (auto f : tpl->components) {
            if (hasVarbitsOrUnions(typeMap, f))
                return true;
        }
    }
    return false;
}

bool TypeInference::onlyBitsOrBitStructs(const IR::Type* type) const {
    // called for a canonical type
    if (type->is<IR::Type_Bits>() || type->is<IR::Type_Boolean>() ||
        type->is<IR::Type_SerEnum>()) {
        return true;
    } else if (auto ht = type->to<IR::Type_Struct>()) {
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            BUG_CHECK((ftype != nullptr), "onlyBitsOrBitStructs check could not find type "
                      "for %1%", f);
            if (!onlyBitsOrBitStructs(ftype))
                return false;
        }
        return true;
    }
    return false;
}

const IR::Node* TypeInference::postorder(IR::MethodCallExpression* expression) {
    if (done()) return expression;
    LOG2("Solving method call " << dbp(expression));
    auto methodType = getType(expression->method);
    if (methodType == nullptr)
        return expression;
    auto methodBaseType = methodType->to<IR::Type_MethodBase>();
    if (methodBaseType == nullptr) {
        typeError("%1% is not a method", expression);
        return expression;
    }

    // Handle differently methods and actions: action invocations return actions
    // with different signatures
    if (methodType->is<IR::Type_Action>()) {
        if (findContext<IR::Function>()) {
            typeError("%1%: Functions cannot call actions", expression);
            return expression;
        }
        bool inActionsList = false;
        auto prop = findContext<IR::Property>();
        if (prop != nullptr && prop->name == IR::TableProperties::actionsPropertyName)
            inActionsList = true;
        return actionCall(inActionsList, expression);
    } else {
        // Constant-fold constant expressions
        if (auto mem = expression->method->to<IR::Member>()) {
            auto type = typeMap->getType(mem->expr, true);
            if (((mem->member == IR::Type::minSizeInBits ||
                 mem->member == IR::Type::minSizeInBytes ||
                 mem->member == IR::Type::maxSizeInBits ||
                 mem->member == IR::Type::maxSizeInBytes)) &&
                !type->is<IR::Type_Extern>() &&
                expression->typeArguments->size() == 0 &&
                expression->arguments->size() == 0) {
                auto max = mem->member.name.startsWith("max");
                int w = typeMap->widthBits(type, expression, max);
                LOG3("Folding " << mem << " to " << w);
                if (w < 0)
                    return expression;
                if (mem->member.name.endsWith("Bytes"))
                    w = ROUNDUP(w, 8);
                auto result = new IR::Constant(w);
                auto tt = new IR::Type_Type(result->type);
                setType(result->type, tt);
                setType(result, result->type);
                setCompileTimeConstant(result);
                return result;
            }
        }

        // We build a type for the callExpression and unify it with the method expression
        // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
        auto rettype = new IR::Type_Var(IR::ID(refMap->newName("R"), "<returned type>"));
        auto args = new IR::Vector<IR::ArgumentInfo>();
        bool constArgs = true;
        for (auto aarg : *expression->arguments) {
            auto arg = aarg->expression;
            auto argType = getType(arg);
            if (argType == nullptr)
                return expression;
            auto argInfo = new IR::ArgumentInfo(arg->srcInfo, isLeftValue(arg),
                                                isCompileTimeConstant(arg), argType, aarg);
            args->push_back(argInfo);
            constArgs = constArgs && isCompileTimeConstant(arg);
        }
        auto typeArgs = new IR::Vector<IR::Type>();
        for (auto ta : *expression->typeArguments) {
            auto taType = getTypeType(ta);
            if (taType == nullptr)
                return expression;
            typeArgs->push_back(taType);
        }
        auto callType = new IR::Type_MethodCall(
            expression->srcInfo, typeArgs, rettype, args);

        auto tvs = unify(expression, methodBaseType, callType,
                         "Function type '%1%' does not match invocation type '%2%'",
                         { methodBaseType, callType });
        if (tvs == nullptr)
            return expression;

        // Infer Dont_Care for type vars used only in not-present optional params
        auto dontCares = new TypeVariableSubstitution();
        auto typeParams = methodBaseType->typeParameters;
        for (auto p : *methodBaseType->parameters) {
            if (!p->isOptional()) continue;
            forAllMatching<IR::Type_Var>(
                p, [tvs, dontCares, typeParams, this](const IR::Type_Var *tv) {
                       if (typeMap->getSubstitutions()->lookup(tv) != nullptr)
                           return;  // already bound
                       if (tvs->lookup(tv)) return;  // already bound
                       if (typeParams->getDeclByName(tv->name) != tv)
                           return;  // not a tv of this call
                       dontCares->setBinding(tv, new IR::Type_Dontcare);
                   });
        }
        addSubstitutions(dontCares);

        LOG2("Method type before specialization " << methodType << " with " << tvs);
        TypeVariableSubstitutionVisitor substVisitor(tvs);
        substVisitor.setCalledBy(this);
        auto specMethodType = methodType->apply(substVisitor);
        LOG2("Method type after specialization " << specMethodType);
        learn(specMethodType, this);

        auto canon = getType(specMethodType);
        if (canon == nullptr)
            return expression;

        auto functionType = specMethodType->to<IR::Type_MethodBase>();
        BUG_CHECK(functionType != nullptr, "Method type is %1%", specMethodType);

        if (!functionType->is<IR::Type_Method>())
            BUG("Unexpected type for function %1%", functionType);

        auto returnType = tvs->lookup(rettype);
        if (returnType == nullptr) {
            typeError("Cannot infer a concrete return type for this call of %1%", expression);
            return expression;
        }
        // The return type may also contain type variables
        returnType = returnType->apply(substVisitor)->to<IR::Type>();
        learn(returnType, this);
        if (returnType->is<IR::Type_Control>() ||
            returnType->is<IR::Type_Parser>() ||
            returnType->is<IR::P4Parser>() ||
            returnType->is<IR::P4Control>() ||
            returnType->is<IR::Type_Package>() ||
            (returnType->is<IR::Type_Extern>() && !constArgs)) {
            // Experimental: methods with all constant arguments can return an extern
            // instance as a factory method evaluated at compile time.
            typeError("%1%: illegal return type %2%", expression, returnType);
            return expression;
        }

        setType(getOriginal(), returnType);
        setType(expression, returnType);

        ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
        auto result = expression;
        // Arguments may need to be cast, e.g., list expression to a
        // header type.
        auto paramIt = functionType->parameters->begin();
        auto newArgs = new IR::Vector<IR::Argument>();
        bool changed = false;
        for (auto arg : *expression->arguments) {
            cstring argName = arg->name.name;
            bool named = !argName.isNullOrEmpty();
            const IR::Parameter* param;

            if (named) {
                param = functionType->parameters->getParameter(argName);
            } else {
                param = *paramIt;
            }

            auto newExpr = arg->expression;
            if (param->direction == IR::Direction::In) {
                // This is like an assignment; may make additional conversions.
                newExpr = assignment(arg, param->type, arg->expression);
            } else {
                // Insert casts for 'int' values.
                newExpr = cts.convert(newExpr)->to<IR::Expression>();
            }
            if (::errorCount() > 0)
                return expression;
            if (newExpr != arg->expression) {
                LOG2("Changing method argument to " << newExpr);
                changed = true;
                newArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, newExpr));
            } else {
                newArgs->push_back(arg);
            }
            if (!named)
                ++paramIt;
        }

        if (changed)
            result = new IR::MethodCallExpression(
                result->srcInfo, result->type, result->method, result->typeArguments, newArgs);
        setType(result, returnType);

        auto mi = MethodInstance::resolve(result, refMap, typeMap, nullptr, true);
        if (mi->isApply() && findContext<IR::P4Action>()) {
            typeError("%1%: apply cannot be called from actions", expression);
            return expression;
        }

        if (mi->is<ExternFunction>() && constArgs) {
           // extern functions with constant args are compile-time constants
           setCompileTimeConstant(expression);
           setCompileTimeConstant(getOriginal<IR::Expression>());
        }

        auto bi = mi->to<BuiltInMethod>();
        if ((findContext<IR::SelectCase>()) &&
            (!bi || (bi->name == IR::Type_Stack::pop_front ||
                     bi->name == IR::Type_Stack::push_front))) {
            typeError("%1%: no function calls allowed in this context", expression);
            return expression;
        }
        return result;
    }
    return expression;
}

const IR::Node* TypeInference::postorder(IR::ConstructorCallExpression* expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->constructedType);
    if (type == nullptr)
        return expression;

    auto simpleType = type;
    CHECK_NULL(simpleType);
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
        auto typeAndArgs = containerInstantiation(expression, expression->arguments,
                                                  simpleType->to<IR::IContainer>());
        auto conttype = typeAndArgs.first;
        auto args = typeAndArgs.second;
        if (conttype == nullptr || args == nullptr)
            return expression;
        if (type->is<IR::Type_SpecializedCanonical>()) {
            auto st = type->to<IR::Type_SpecializedCanonical>();
            conttype = new IR::Type_SpecializedCanonical(
                type->srcInfo, st->baseType, st->arguments, conttype);
        }
        if (args != expression->arguments)
            expression->arguments = args;
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

static void convertStructToTuple(const IR::Type_StructLike* structType, IR::Type_Tuple *tuple) {
    for (auto field : structType->fields) {
        if (auto ft = field->type->to<IR::Type_Bits>()) {
            tuple->components.push_back(ft);
        } else if (auto ft = field->type->to<IR::Type_StructLike>()) {
            convertStructToTuple(ft, tuple);
        } else if (auto ft = field->type->to<IR::Type_InfInt>()) {
            tuple->components.push_back(ft);
        } else {
            BUG("Unexpected type %1% for struct field %2%", field->type, field);
        }
    }
}

const IR::SelectCase*
TypeInference::matchCase(const IR::SelectExpression* select, const IR::Type_BaseList* selectType,
                         const IR::SelectCase* selectCase, const IR::Type* caseType) {
    // The selectType is always a tuple
    // If the caseType is a set type, we unify the type of the set elements
    if (caseType->is<IR::Type_Set>())
        caseType = caseType->to<IR::Type_Set>()->elementType;
    // The caseType may be a simple type, and then we have to unwrap the selectType
    if (caseType->is<IR::Type_Dontcare>())
        return selectCase;

    if (caseType->is<IR::Type_StructLike>()) {
        auto tupleType = new IR::Type_Tuple();
        convertStructToTuple(caseType->to<IR::Type_StructLike>(), tupleType);
        caseType = tupleType;
    }
    const IR::Type* useSelType = selectType;
    if (!caseType->is<IR::Type_BaseList>()) {
        if (selectType->components.size() != 1) {
            typeError("Type mismatch %1% (%2%) vs %3% (%4%)",
                      select->select, selectType->toString(), selectCase, caseType->toString());
            return nullptr;
        }
        useSelType = selectType->components.at(0);
    }
    auto tvs = unify(select, useSelType, caseType,
                     "'match' case label type '%1%' does not match expected type '%2%'",
                     { caseType, useSelType });
    if (tvs == nullptr)
        return nullptr;
    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
    auto ks = cts.convert(selectCase->keyset);
    if (::errorCount() > 0)
        return selectCase;

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

bool TypeInference::containsHeader(const IR::Type* type) {
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Stack>() ||
        type->is<IR::Type_HeaderUnion>())
        return true;
    if (type->is<IR::Type_Struct>()) {
        auto st = type->to<IR::Type_Struct>();
        for (auto f : st->fields)
            if (containsHeader(f->type))
                return true;
    }
    return false;
}

/// Expressions that appear in a select expression are restricted to a small
/// number of types: bits, enums, serializable enums, and booleans.
static bool validateSelectTypes(const IR::Type* type, const IR::SelectExpression* expression) {
    if (auto tuple = type->to<IR::Type_BaseList>()) {
        for (auto ct : tuple->components) {
            auto check = validateSelectTypes(ct, expression);
            if (!check)
                return false;
        }
        return true;
    } else if (type->is<IR::Type_Bits>() || type->is<IR::Type_SerEnum>() ||
               type->is<IR::Type_Boolean>() || type->is<IR::Type_Enum>()) {
        return true;
    }
    typeError("Expression '%1%' with a component of type '%2%' cannot be used in 'select'",
              expression->select, type);
    return false;
}

const IR::Node* TypeInference::postorder(IR::SelectExpression* expression) {
    if (done()) return expression;
    auto selectType = getType(expression->select);
    if (selectType == nullptr)
        return expression;

    // Check that the selectType is determined
    auto tuple = selectType->to<IR::Type_BaseList>();
    BUG_CHECK(tuple != nullptr, "%1%: Expected a tuple type for the select expression, got %2%",
              expression, selectType);
    if (!validateSelectTypes(selectType, expression))
        return expression;

    bool changes = false;
    IR::Vector<IR::SelectCase> vec;
    for (auto sc : expression->selectCases)  {
        auto type = getType(sc->keyset);
        if (type == nullptr)
            return expression;
        auto newsc = matchCase(expression, tuple, sc, type);
        vec.push_back(newsc);
        if (newsc != sc)
            changes = true;
    }
    if (changes)
        expression = new IR::SelectExpression(expression->srcInfo, expression->select,
                                              std::move(vec));
    setType(expression, IR::Type_State::get());
    setType(getOriginal(), IR::Type_State::get());
    return expression;
}

const IR::Node* TypeInference::postorder(IR::AttribLocal* local) {
    setType(local, local->type);
    setType(getOriginal(), local->type);
    return local;
}

///////////////////////////////////////// Statements et al.

const IR::Node* TypeInference::postorder(IR::IfStatement* conditional) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto type = getType(conditional->condition);
    if (type == nullptr)
        return conditional;
    if (!type->is<IR::Type_Boolean>())
        typeError("Condition of %1% does not evaluate to a bool but %2%",
                  conditional, type->toString());
    return conditional;
}

const IR::Node* TypeInference::postorder(IR::SwitchStatement* stat) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto type = getType(stat->expression);
    if (type == nullptr)
        return stat;

    if (auto ae = type->to<IR::Type_ActionEnum>()) {
        // switch (table.apply(...))
        std::set<cstring> foundLabels;
        for (auto c : stat->cases) {
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
    } else {
        // switch (expression)
        Comparison comp;
        comp.left = stat->expression;
        if (isCompileTimeConstant(stat->expression))
            warn(ErrorType::WARN_MISMATCH, "%1%: constant expression in switch",
                 stat->expression);

        for (auto &c : stat->cases) {
            if (!isCompileTimeConstant(c->label))
                typeError("%1%: must be a compile-time constant", c->label);
            auto lt = getType(c->label);
            if (lt == nullptr)
                continue;
            if (lt->is<IR::Type_InfInt>() && type->is<IR::Type_Bits>()) {
                auto cst = c->label->to<IR::Constant>();
                CHECK_NULL(cst);
                c = new IR::SwitchCase(
                    c->srcInfo,
                    new IR::Constant(cst->srcInfo, type, cst->value, cst->base), c->statement);
                setType(c->label, type);
                setCompileTimeConstant(c->label);
                continue;
            } else if (c->label->is<IR::DefaultExpression>()) {
                continue;
            }
            comp.right = c->label;
            bool b = compare(stat, type, lt, &comp);
            if (b && comp.right != c->label) {
                c = new IR::SwitchCase(c->srcInfo, comp.right, c->statement);
                setCompileTimeConstant(c->label);
            }
        }
    }
    return stat;
}

const IR::Node* TypeInference::postorder(IR::ReturnStatement* statement) {
    LOG3("TI Visiting " << dbp(getOriginal()));
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
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto ltype = getType(assign->left);
    if (ltype == nullptr)
        return assign;

    if (!isLeftValue(assign->left)) {
        typeError("Expression %1% cannot be the target of an assignment", assign->left);
        LOG2(assign->left);
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
    if (ktype == nullptr)
        return elem;
    while (ktype->is<IR::Type_Newtype>())
        ktype = getTypeType(ktype->to<IR::Type_Newtype>()->type);
    if (!ktype->is<IR::Type_Bits>() && !ktype->is<IR::Type_SerEnum>() &&
        !ktype->is<IR::Type_Error>() && !ktype->is<IR::Type_Boolean>() &&
        !ktype->is<IR::Type_Enum>())
        typeError("Key %1% field type must be a scalar type; it cannot be %2%",
                  elem->expression, ktype->toString());
    auto type = getType(elem->matchType);
    if (type != nullptr && type != IR::Type_MatchKind::get())
        typeError("%1% must be a %2% value", elem->matchType,
                  IR::Type_MatchKind::get()->toString());
    if (isCompileTimeConstant(elem->expression) && !readOnly)
        warn(ErrorType::WARN_IGNORE_PROPERTY, "%1%: constant key element", elem);
    return elem;
}

const IR::ActionListElement* TypeInference::validateActionInitializer(
    const IR::Expression* actionCall,
    const IR::P4Table* table) {

    auto al = table->getActionList();
    if (al == nullptr) {
        typeError("%1% has no action list, so it cannot have %2%",
                  table, actionCall);
        return nullptr;
    }

    auto call = actionCall->to<IR::MethodCallExpression>();
    if (call == nullptr) {
        typeError("%1%: expected an action call", actionCall);
        return nullptr;
    }
    auto method = call->method;
    if (!method->is<IR::PathExpression>())
        BUG("%1%: unexpected expression", method);
    auto pe = method->to<IR::PathExpression>();
    auto decl = refMap->getDeclaration(pe->path, true);
    auto ale = al->actionList.getDeclaration(decl->getName());
    if (ale == nullptr) {
        typeError("%1% not present in action list", call);
        return nullptr;
    }

    BUG_CHECK(ale->is<IR::ActionListElement>(),
              "%1%: expected an ActionListElement", ale);
    auto elem = ale->to<IR::ActionListElement>();
    auto entrypath = elem->getPath();
    auto entrydecl = refMap->getDeclaration(entrypath, true);
    if (entrydecl != decl) {
        typeError("%1% and %2% refer to different actions", actionCall, elem);
        return nullptr;
    }

    // Check that the data-plane parameters
    // match the data-plane parameters for the same action in
    // the actions list.
    auto actionListCall = elem->expression->to<IR::MethodCallExpression>();
    CHECK_NULL(actionListCall);
    auto type = typeMap->getType(actionListCall->method);
    if (type == nullptr) {
        typeError("%1%: action invocation should be after the `actions` list", actionCall);
        return nullptr;
    }

    if (actionListCall->arguments->size() > call->arguments->size()) {
        typeError("%1%: not enough arguments", call);
        return nullptr;
    }

    SameExpression se(refMap, typeMap);
    auto callInstance = MethodInstance::resolve(call, refMap, typeMap, nullptr, true);
    auto listInstance = MethodInstance::resolve(actionListCall, refMap, typeMap, nullptr, true);

    for (auto param : *listInstance->substitution.getParametersInArgumentOrder()) {
        auto aa = listInstance->substitution.lookup(param);
        auto da = callInstance->substitution.lookup(param);
        if (da == nullptr) {
            typeError("%1%: parameter should be assigned in call %2%",
                      param, call);
            return nullptr;
        }
        bool same = se.sameExpression(aa->expression, da->expression);
        if (!same) {
            typeError("%1%: argument does not match declaration in actions list: %2%",
                      da, aa);
            return nullptr;
        }
    }

    for (auto param : *callInstance->substitution.getParametersInOrder()) {
        auto da = callInstance->substitution.lookup(param);
        if (da == nullptr) {
            typeError("%1%: parameter should be assigned in call %2%",
                      param, call);
            return nullptr;
        }
    }

    return elem;
}

const IR::Node* TypeInference::postorder(IR::Property* prop) {
    // Handle the default_action
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
            if (at->parameters->size() != 0) {
                typeError("%1%: parameter %2% does not have a corresponding argument",
                          prop->value, at->parameters->parameters.at(0));
                return prop;
            }

            auto table = findContext<IR::P4Table>();
            BUG_CHECK(table != nullptr, "%1%: property not within a table?", prop);
            // Check that the default action appears in the list of actions.
            BUG_CHECK(prop->value->is<IR::ExpressionValue>(), "%1% not an expression", prop);
            auto def = prop->value->to<IR::ExpressionValue>()->expression;
            auto ale = validateActionInitializer(def, table);
            if (ale != nullptr) {
                auto anno = ale->getAnnotation(IR::Annotation::tableOnlyAnnotation);
                if (anno != nullptr) {
                    typeError("%1%: Action marked with %2% used as default action",
                              prop, IR::Annotation::tableOnlyAnnotation);
                    return prop;
                }
            }
        }
    }
    return prop;
}

}  // namespace P4
