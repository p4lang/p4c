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
#include "frontends/p4/methodInstance.h"

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
        if (result != expr && (::errorCount() == 0)) {
            auto *learn = tc->clone();
            (void)result->apply(*learn);
        }
        return result;
    }
    const IR::Vector<IR::Expression>* convert(const IR::Vector<IR::Expression>* vec) {
        auto result = vec->apply(*this)->to<IR::Vector<IR::Expression>>();
        if (result != vec) {
            auto *learn = tc->clone();
            (void)result->apply(*learn);
        }
        return result;
    }
    const IR::Vector<IR::Argument>* convert(const IR::Vector<IR::Argument>* vec) {
        auto result = vec->apply(*this)->to<IR::Vector<IR::Argument>>();
        if (result != vec) {
            auto *learn = tc->clone();
            (void)result->apply(*learn);
        }
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
    auto cl = type->to<IR::Type>()->apply(sv);
    CHECK_NULL(cl);
    // Learn this new type
    auto *tc = clone();
    (void)cl->apply(*tc);
    LOG3("Cloned for type variables " << type << " into " << cl);
    return cl->to<IR::Type>();
}

TypeInference::TypeInference(ReferenceMap* refMap, TypeMap* typeMap, bool readOnly) :
        refMap(refMap), typeMap(typeMap),
        initialNode(nullptr), readOnly(readOnly) {
    CHECK_NULL(typeMap);
    CHECK_NULL(refMap);
    visitDagOnce = false;  // the done() method will take care of this
}

Visitor::profile_t TypeInference::init_apply(const IR::Node* node) {
    if (node->is<IR::P4Program>()) {
        LOG2("Reference map for type checker:" << std::endl << refMap);
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
        BUG("Could not find type of %1%", element);
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
    if (readOnly)
        // we only need to do this the first time
        return;
    typeMap->addSubstitutions(tvs);
}

TypeVariableSubstitution* TypeInference::unify(const IR::Node* errorPosition,
                                               const IR::Type* destType,
                                               const IR::Type* srcType) {
    CHECK_NULL(destType); CHECK_NULL(srcType);
    if (srcType == destType)
        return new TypeVariableSubstitution();

    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    constraints.addEqualityConstraint(destType, srcType);
    auto tvs = constraints.solve(errorPosition);
    addSubstitutions(tvs);
    return tvs;
}

const IR::IndexedVector<IR::StructField>*
TypeInference::canonicalizeFields(const IR::Type_StructLike* type) {
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
        return fields;
    else
        return &type->fields;
}

const IR::ParameterList* TypeInference::canonicalizeParameters(const IR::ParameterList* params) {
    if (params == nullptr)
        return params;

    bool changes = false;
    auto vec = new IR::IndexedVector<IR::Parameter>();
    for (auto p : *params->getEnumerator()) {
        auto paramType = getType(p);
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
        if (p->direction != IR::Direction::None && type->is<IR::Type_Extern>()) {
            typeError("%1%: a parameter with an extern type cannot have a direction", p);
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
        return canon;
    } else if (type->is<IR::Type_Enum>() ||
               type->is<IR::Type_SerEnum>() ||
               type->is<IR::Type_ActionEnum>() ||
               type->is<IR::Type_MatchKind>()) {
        return type;
    } else if (type->is<IR::Type_Set>()) {
        auto set = type->to<IR::Type_Set>();
        auto et = canonicalize(set->elementType);
        if (et == nullptr)
            return nullptr;
        if (et->is<IR::Type_Stack>() || et->is<IR::Type_Set>() ||
            et->is<IR::Type_HeaderUnion>())
            ::error("%1%: Sets of %2% are not supported", type, et);
        if (et == set->elementType)
            return type;
        const IR::Type *canon = new IR::Type_Set(type->srcInfo, et);
        return canon;
    } else if (type->is<IR::Type_Stack>()) {
        auto stack = type->to<IR::Type_Stack>();
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
    } else if (type->is<IR::Type_Tuple>()) {
        auto tuple = type->to<IR::Type_Tuple>();
        auto fields = new IR::Vector<IR::Type>();
        // tuple<set<a>, b> = set<tuple<a, b>>
        // TODO: this should not be done here.
        bool anySet = false;
        bool anyChange = false;
        for (auto t : tuple->components) {
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
        if (anyChange || anySet)
            canon = new IR::Type_Tuple(type->srcInfo, *fields);
        else
            canon = type;
        canon = typeMap->getCanonical(canon);
        if (anySet)
            canon = new IR::Type_Set(type->srcInfo, canon);
        return canon;
    } else if (type->is<IR::Type_Parser>()) {
        auto tp = type->to<IR::Type_Parser>();
        auto pl = canonicalizeParameters(tp->applyParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages))
            return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Parser(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::Type_Control>()) {
        auto tp = type->to<IR::Type_Control>();
        auto pl = canonicalizeParameters(tp->applyParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages))
            return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Control(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::Type_Package>()) {
        auto tp = type->to<IR::Type_Package>();
        auto pl = canonicalizeParameters(tp->constructorParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr)
            return nullptr;
        if (pl != tp->constructorParams || tps != tp->typeParameters)
            return new IR::Type_Package(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (type->is<IR::P4Control>()) {
        auto cont = type->to<IR::P4Control>();
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
    } else if (type->is<IR::P4Parser>()) {
        auto p = type->to<IR::P4Parser>();
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
    } else if (type->is<IR::Type_Extern>()) {
        auto te = type->to<IR::Type_Extern>();
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
    } else if (type->is<IR::Type_Method>()) {
        auto mt = type->to<IR::Type_Method>();
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
            resultType = new IR::Type_Method(mt->getSourceInfo(), tps, res, pl);
        return resultType;
    } else if (type->is<IR::Type_Header>()) {
        auto hdr = type->to<IR::Type_Header>();
        auto fields = canonicalizeFields(hdr);
        if (fields == nullptr)
            return nullptr;
        const IR::Type* canon;
        if (fields != &hdr->fields)
            canon = new IR::Type_Header(hdr->srcInfo, hdr->name, hdr->annotations, *fields);
        else
            canon = hdr;
        return canon;
    } else if (type->is<IR::Type_Struct>()) {
        auto str = type->to<IR::Type_Struct>();
        auto fields = canonicalizeFields(str);
        if (fields == nullptr)
            return nullptr;
        const IR::Type* canon;
        if (fields != &str->fields)
            canon = new IR::Type_Struct(str->srcInfo, str->name, str->annotations, *fields);
        else
            canon = str;
        return canon;
    } else if (type->is<IR::Type_HeaderUnion>()) {
        auto str = type->to<IR::Type_HeaderUnion>();
        auto fields = canonicalizeFields(str);
        if (fields == nullptr)
            return nullptr;
        const IR::Type* canon;
        if (fields != &str->fields)
            canon = new IR::Type_HeaderUnion(str->srcInfo, str->name, str->annotations, *fields);
        else
            canon = str;
        return canon;
    } else if (type->is<IR::Type_Specialized>()) {
        auto st = type->to<IR::Type_Specialized>();
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
            if (atype->is<IR::Type_Control>() ||
                atype->is<IR::Type_Parser>() ||
                atype->is<IR::Type_Package>() ||
                atype->is<IR::P4Parser>() ||
                atype->is<IR::P4Control>()) {
                typeError("%1%: Cannot use %2% as a type parameter", type, atype);
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
        // learn the types of all components of the specialized type
        LOG2("Scanning the specialized type");
        auto *tc = clone();
        (void)result->apply(*tc);
        return result;
    } else {
        BUG("Unexpected type %1%", dbp(type));
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

    if (type->is<IR::Type_SpecializedCanonical>())
        type = type->to<IR::Type_SpecializedCanonical>()->baseType;

    if (type->is<IR::IContainer>() || type->is<IR::Type_Extern>()) {
        typeError("%1%: cannot declare variables of type %2% (consider using an instantiation)",
                  decl, type);
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
    if (src == dest)
        return true;

    if (dest->is<IR::Type_Newtype>()) {
        auto dt = getTypeType(dest->to<IR::Type_Newtype>()->type);
        if (TypeMap::equivalent(dt, src))
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
        } else if (auto se = dest->to<IR::Type_SerEnum>()) {
            return TypeMap::equivalent(src, se->type);
        }
    } else if (src->is<IR::Type_Boolean>()) {
        if (dest->is<IR::Type_Bits>()) {
            auto b = dest->to<IR::Type_Bits>();
            return b->size == 1 && !b->isSigned;
        }
    } else if (src->is<IR::Type_InfInt>()) {
        return dest->is<IR::Type_Bits>();
    } else if (src->is<IR::Type_Newtype>()) {
        auto st = getTypeType(src->to<IR::Type_Newtype>()->type);
        return TypeMap::equivalent(dest, st);
    } else if (auto se = src->to<IR::Type_SerEnum>()) {
        if (auto db = dest->to<IR::Type_Bits>()) {
            return TypeMap::equivalent(se->type, db);
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

    // Unification allows tuples to be assigned to headers and structs.
    // We should only allow this if the source expression is a ListExpression.
    if (destType->is<IR::Type_StructLike>() && initType->is<IR::Type_Tuple>() &&
        !sourceExpression->is<IR::ListExpression>()) {
        typeError("%1%: Cannot assign a %2% to a %3%", errorPosition, initType, destType);
        return sourceExpression;
    }

    auto tvs = unify(errorPosition, destType, initType);
    if (tvs == nullptr)
        // error already signalled
        return sourceExpression;
    if (!tvs->isIdentity()) {
        ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
        return cts.convert(sourceExpression);  // sets type
    }
    return sourceExpression;
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
    auto tp = ext->getTypeParameters();
    if (!tp->empty()) {
        typeError("%1%: Type parameters must be supplied for constructor", errorPosition);
        return nullptr;
    }
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

    bool changes = false;
    auto result = new IR::Vector<IR::Argument>();
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

        auto tvs = unify(errorPosition, paramType, argType);
        if (tvs == nullptr) {
            // error already signalled
            return nullptr;
        }
        if (tvs->isIdentity()) {
            result->push_back(arg);
            continue;
        }

        ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
        auto newArg = cts.convert(arg->expression);
        if (::errorCount() > 0)
            return arguments;

        result->push_back(new IR::Argument(arg->srcInfo, arg->name, newArg));
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
            auto tvs = unify(inst, methtype, ftype);
            if (tvs == nullptr)
                return false;
            BUG_CHECK(tvs->isIdentity(), "%1%: expected no type variables", tvs);
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
        if (decl->initializer != nullptr) {
            typeError("%1%: initializers only allowed for extern instances", decl->initializer);
            prune();
            return decl;
        }
        if (!simpleType->is<IR::Type_Package>() && (findContext<IR::IContainer>() == nullptr)) {
            ::error("%1%: cannot instantiate at top-level", decl);
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
        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg, true, argType, aarg->name);
        args->push_back(argInfo);
    }
    auto rettype = new IR::Type_Var(IR::ID(refMap->newName("R")));
    // There are never type arguments at this point; if they exist, they have been folded
    // into the constructor by type specialization.
    auto callType = new IR::Type_MethodCall(node->srcInfo,
                                            new IR::Vector<IR::Type>(),
                                            rettype, args);
    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    constraints.addEqualityConstraint(constructor, callType);
    auto tvs = constraints.solve(node);
    if (tvs == nullptr)
        return std::pair<const IR::Type*, const IR::Vector<IR::Argument>*>(nullptr, nullptr);
    addSubstitutions(tvs);

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
            auto *tc = clone();
            unsigned e = ::errorCount();
            (void)canon->apply(*tc);
            if (::errorCount() > e)
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
    auto exprType = getType(member->value);
    auto tvs = unify(member, type, exprType);
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
        // Learn the new type
        if (canon != decl->elementType) {
            auto *tc = clone();
            unsigned e = ::errorCount();
            (void)canon->apply(*tc);
            if (::errorCount() > e)
                return nullptr;
        }
        auto tt = new IR::Type_Set(canon);
        setType(getOriginal(), tt);
        setType(decl, tt);
    }
    return decl;
}

const IR::Node* TypeInference::postorder(IR::Type_Extern* type) {
    if (done()) return type;
    auto canon = setTypeType(type);
    if (canon != nullptr) {
        auto te = canon->to<IR::Type_Extern>();
        CHECK_NULL(te);
        for (auto method : te->methods) {
            if (method->name == type->name) {  // constructor
                if (method->type->typeParameters != nullptr &&
                    method->type->typeParameters->size() > 0) {
                    typeError("%1%: Constructors cannot have type parameters",
                              method->type->typeParameters);
                    return type;
                }
            }
        }
    }
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_Method* type) {
    (void)setTypeType(type);
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
        !argType->is<IR::Type_Tuple>() &&
        !argType->is<IR::Type_Newtype>())
        ::error("%1%: `type' can only be applied to base types or tuple types",
                type);
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
    auto canon = setTypeType(type);
    if (canon == nullptr)
        return type;

    auto etype = canon->to<IR::Type_Stack>()->elementType;
    if (etype == nullptr)
        return type;

    if (!etype->is<IR::Type_Header>() && !etype->is<IR::Type_HeaderUnion>())
        typeError("Header stack %1% used with non-header type %2%",
                  type, etype->toString());
    return type;
}

void TypeInference::validateFields(const IR::Type* type,
                                   std::function<bool(const IR::Type*)> checker) const {
    if (type == nullptr)
        return;
    BUG_CHECK(type->is<IR::Type_StructLike>(), "%1%; expected a Struct-like", type);
    auto strct = type->to<IR::Type_StructLike>();
    for (auto field : strct->fields) {
        auto ftype = getType(field);
        if (ftype == nullptr)
            return;
        if (!checker(ftype))
            typeError("Field %1% of %2% cannot have type %3%",
                      field, type->toString(), field->type);
    }
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
               // Nested bit-vector struct inside a Header is supported
               // Experimental feature - see Issue 383.
               (t->is<IR::Type_Struct>() && onlyBitsOrBitStructs(t)) ||
               t->is<IR::Type_SerEnum>() || t->is<IR::Type_Boolean>(); };
    validateFields(canon, validator);

    const IR::StructField* varbit = nullptr;
    for (auto field : type->fields) {
        auto ftype = getType(field);
        if (ftype == nullptr)
            return type;
        if (ftype->is<IR::Type_Varbits>()) {
            if (varbit == nullptr) {
                varbit = field;
            } else {
                typeError("%1% and %2%: multiple varbit fields in a header",
                          varbit, field);
                return type;
            }
        }
    }
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
        t->is<IR::Type_Tuple>() || t->is<IR::Type_SerEnum>(); };
    validateFields(canon, validator);
    return type;
}

const IR::Node* TypeInference::postorder(IR::Type_HeaderUnion *type) {
    auto canon = setTypeType(type);
    auto validator = [] (const IR::Type* t) { return t->is<IR::Type_Header>(); };
    validateFields(canon, validator);
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

const IR::Node* TypeInference::postorder(IR::Operation_Relation* expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr)
        return expression;

    bool equTest = expression->is<IR::Equ>() || expression->is<IR::Neq>();

    if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_Bits>()) {
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
        bool defined = false;
        if (TypeMap::equivalent(ltype, rtype) &&
            (!ltype->is<IR::Type_Void>() && !ltype->is<IR::Type_Varbits>())) {
            defined = true;
        } else if (ltype->is<IR::Type_Base>() && rtype->is<IR::Type_Base>() &&
                 TypeMap::equivalent(ltype, rtype)) {
            defined = true;
        } else if (ltype->is<IR::Type_Tuple>() && rtype->is<IR::Type_Tuple>()) {
            auto tvs = unify(expression, ltype, rtype);
            if (tvs == nullptr)
                // error already signalled
                return expression;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
                expression->left = cts.convert(expression->left);
                expression->right = cts.convert(expression->right);
            }
            defined = true;
        } else {
            // comparison between structs and list expressions is allowed only
            // if the expression with tuple type is a list expression
            if ((ltype->is<IR::Type_StructLike>() &&
                 rtype->is<IR::Type_Tuple>() && expression->right->is<IR::ListExpression>()) ||
                 (ltype->is<IR::Type_Tuple>() && expression->left->is<IR::ListExpression>()
                  && rtype->is<IR::Type_StructLike>())) {
                if (!ltype->is<IR::Type_StructLike>()) {
                    // swap
                    auto type = ltype;
                    ltype = rtype;
                    rtype = type;
                }

                auto tvs = unify(expression, ltype, rtype);
                if (tvs == nullptr)
                    // error already signalled
                    return expression;
                if (!tvs->isIdentity()) {
                    ConstantTypeSubstitution cts(tvs, refMap, typeMap, this);
                    expression->left = cts.convert(expression->left);
                    expression->right = cts.convert(expression->right);
                }
                defined = true;
            }
        }

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
    if (keyset == nullptr || !keyset->is<IR::ListExpression>()) {
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

    TypeVariableSubstitution *tvs = unify(entry, keyTuple, entryKeyType);
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

    auto tupleType = new IR::Type_Tuple(expression->srcInfo, *components);
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

const IR::Node* TypeInference::postorder(IR::StructInitializerExpression* expression) {
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

    const IR::Type* structType;
    if (expression->isHeader)
        structType = new IR::Type_Header(
            expression->srcInfo, expression->name, *components);
    else
        structType = new IR::Type_Struct(
            expression->srcInfo, expression->name, *components);
    auto type = canonicalize(structType);
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
            typeError("%1%: Negative array index %2%", expression, cst);
            return expression;
        }
        if (hst->sizeKnown()) {
            int size = hst->getSize();
            if (index >= size) {
                typeError("Array index %1% larger or equal to array size %2%",
                          cst, hst->size);
                return expression;
            }
        }
    }
    setType(getOriginal(), hst->elementType);
    setType(expression, hst->elementType);
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
            ::warning(ErrorType::WARN_OVERFLOW, "%1%: shifting value with %2% bits by %3%",
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
        if (!TypeMap::equivalent(bl, br)) {
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

    // The following section is very similar to "binaryArith()" above
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
        if (!TypeMap::equivalent(bl, br)) {
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
        sameType = ltype;
        setType(e->right, sameType);
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
    const IR::Type* castType = getTypeType(expression->destType);
    if (sourceType == nullptr || castType == nullptr)
        return expression;

    if (!castType->is<IR::Type_Bits>() &&
        !castType->is<IR::Type_Boolean>() &&
        !castType->is<IR::Type_Newtype>() &&
        !castType->is<IR::Type_SerEnum>()) {
        ::error("%1%: cast not supported", expression->destType);
        return expression;
    }

    if (!canCastBetween(castType, sourceType)) {
        // This cast is not legal directly, but let's try to see whether
        // performing a substitution can help.  This will allow the use
        // of constants on the RHS.
        const IR::Type* destType = castType;
        while (destType->is<IR::Type_Newtype>())
            destType = getTypeType(destType->to<IR::Type_Newtype>()->type);
        auto rhs = assignment(expression, destType, expression->expr);
        if (rhs == nullptr)
            // error
            return expression;
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
    if (decl->is<IR::Function>()) {
        auto func = findContext<IR::Function>();
        if (func != nullptr && func->name == decl->getName()) {
            typeError("%1%: Recursive function call", expression);
            return expression;
        }
    } else if (decl->is<IR::P4Action>()) {
        auto act = findContext<IR::P4Action>();
        if (act != nullptr && act->name == decl->getName()) {
            typeError("%1%: Recursive action call", expression);
            return expression;
        }
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
    } else if (decl->is<IR::Method>()) {
        type = getType(decl->getNode());
        // Each method invocation uses fresh type variables
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

    const IR::Type* result = IR::Type_Bits::get(bst->srcInfo, m - l + 1, bst->isSigned);
    result = canonicalize(result);
    if (result == nullptr)
        return expression;
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
    auto tvs = unify(expression, secondType, thirdType);
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

    if (type->is<IR::Type_StructLike>()) {
        if (type->is<IR::Type_Header>() || type->is<IR::Type_HeaderUnion>()) {
            if (member == IR::Type_Header::isValid) {
                // Built-in method
                auto type = new IR::Type_Method(IR::Type_Boolean::get(), new IR::ParameterList());
                auto ctype = canonicalize(type);
                if (ctype == nullptr)
                    return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }
        if (type->is<IR::Type_Header>()) {
            if (member == IR::Type_Header::setValid ||
                member == IR::Type_Header::setInvalid) {
                if (!isLeftValue(expression->expr))
                    typeError("%1%: must be applied to a left-value", expression);
                // Built-in method
                auto type = new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList);
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
            typeError("Field %1% is not a member of structure %2%",
                      expression->member, stb);
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
        // sometimes this is a synthesized type, so we have to crawl it to understand it
        auto *learn = clone();
        (void)methodType->apply(*learn);

        setType(getOriginal(), methodType);
        setType(expression, methodType);
        return expression;
    }


    if (type->is<IR::Type_Stack>()) {
        if (member == IR::Type_Stack::next ||
            member == IR::Type_Stack::last) {
            auto control = findContext<IR::P4Control>();
            if (control != nullptr)
                typeError("%1%: 'last' and 'next' for stacks cannot be used in a control",
                          expression);
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
            auto parser = findContext<IR::P4Parser>();
            if (parser != nullptr)
                typeError("%1%: '%2%' and '%3%' for stacks cannot be used in a parser",
                          expression, IR::Type_Stack::push_front, IR::Type_Stack::pop_front);
            if (!isLeftValue(expression->expr))
                typeError("%1%: must be applied to a left-value", expression);
            auto params = new IR::IndexedVector<IR::Parameter>();
            auto param = new IR::Parameter(IR::ID("count", nullptr), IR::Direction::In,
                                           new IR::Type_InfInt());
            setType(param, param->type);
            params->push_back(param);
            auto type = new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList(*params));
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
        constraints.addEqualityConstraint(paramType, argType);
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
        if (p.second->direction != IR::Direction::None) {
            typeError("%1%: Parameter %2% must be bound", actionCall, p.second);
            error = true;
        }
    }
    if (error)
        return actionCall;

    auto resultType = new IR::Type_Action(baseType->srcInfo, baseType->typeParameters, params);

    setType(getOriginal(), resultType);
    setType(actionCall, resultType);
    auto tvs = constraints.solve(actionCall);
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

bool TypeInference::hasVarbitsOrUnions(const IR::Type* type) const {
    // called for a canonical type
    if (type->is<IR::Type_HeaderUnion>() || type->is<IR::Type_Varbits>()) {
        return true;
    } else if (auto ht = type->to<IR::Type_StructLike>()) {
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr)
                continue;
            if (hasVarbitsOrUnions(ftype))
                return true;
        }
    } else if (auto at = type->to<IR::Type_Stack>()) {
        return hasVarbitsOrUnions(at->elementType);
    } else if (auto tpl = type->to<IR::Type_Tuple>()) {
        for (auto f : tpl->components) {
            if (hasVarbitsOrUnions(f))
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

void TypeInference::checkEmitType(const IR::Expression* emit, const IR::Type* type) const {
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Stack>() ||
        type->is<IR::Type_HeaderUnion>())
        return;

    if (type->is<IR::Type_Struct>()) {
        for (auto f : type->to<IR::Type_Struct>()->fields) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr)
                continue;
            checkEmitType(emit, ftype);
        }
        return;
    }

    if (type->is<IR::Type_Tuple>()) {
        for (auto f : type->to<IR::Type_Tuple>()->components) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr)
                continue;
            checkEmitType(emit, ftype);
        }
        return;
    }

    typeError("%1%: argument must be a header, stack or union, or a struct or tuple of such types",
            emit);
}

void TypeInference::checkCorelibMethods(const ExternMethod* em) const {
    P4CoreLibrary &corelib = P4CoreLibrary::instance;
    auto et = em->actualExternType;
    auto mce = em->expr;
    unsigned argCount = mce->arguments->size();

    if (et->name == corelib.packetIn.name) {
        if (em->method->name == corelib.packetIn.extract.name) {
            if (argCount == 0) {
                // core.p4 is corrupted.
                typeError("%1%: Expected exactly 1 argument for %2% method",
                          mce, corelib.packetIn.extract.name);
                return;
            }

            auto arg0 = mce->arguments->at(0);
            auto argType = typeMap->getType(arg0, true);
            if (!argType->is<IR::Type_Header>() && !argType->is<IR::Type_Dontcare>()) {
                typeError("%1%: argument must be a header", mce->arguments->at(0));
                return;
            }

            if (argCount == 1) {
                if (hasVarbitsOrUnions(argType))
                    // This will never have unions, but may have varbits
                    typeError("%1%: argument cannot contain varbit fields", arg0);
            } else if (argCount == 2) {
                if (!hasVarbitsOrUnions(argType))
                    typeError("%1%: argument should contain a varbit field", arg0);
            } else {
                // core.p4 is corrupted.
                typeError("%1%: Expected 1 or 2 arguments for '%2%' method",
                          mce, corelib.packetIn.extract.name);
            }
        } else if (em->method->name == corelib.packetIn.lookahead.name) {
            // this is a call to packet_in.lookahead.
            if (mce->typeArguments->size() != 1) {
                typeError("Expected 1 type parameter for %1%", em->method);
                return;
            }
            auto targ = em->expr->typeArguments->at(0);
            auto typearg = typeMap->getTypeType(targ, true);
            if (hasVarbitsOrUnions(typearg)) {
                typeError("%1%: type argument must be a fixed-width type", targ);
                return;
            }
        }
    } else if (et->name == corelib.packetOut.name) {
        if (em->method->name == corelib.packetOut.emit.name) {
            if (argCount == 1) {
                auto arg = mce->arguments->at(0);
                auto argType = typeMap->getType(arg, true);
                checkEmitType(mce, argType);
            } else {
                // core.p4 is corrupted.
                typeError("%1%: Expected 1 argument for '%2%' method",
                          mce, corelib.packetOut.emit.name);
                return;
            }
        }
    }
}

const IR::Node* TypeInference::postorder(IR::MethodCallExpression* expression) {
    if (done()) return expression;
    LOG2("Solving method call " << dbp(expression));
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
        // We build a type for the callExpression and unify it with the method expression
        // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
        auto rettype = new IR::Type_Var(IR::ID(refMap->newName("R"), nullptr));
        auto args = new IR::Vector<IR::ArgumentInfo>();
        bool constArgs = true;
        for (auto aarg : *expression->arguments) {
            auto arg = aarg->expression;
            auto argType = getType(arg);
            if (argType == nullptr)
                return expression;
            auto argInfo = new IR::ArgumentInfo(arg->srcInfo, isLeftValue(arg),
                                                isCompileTimeConstant(arg), argType, aarg->name);
            args->push_back(argInfo);
            constArgs &= isCompileTimeConstant(arg);
        }
        auto typeArgs = new IR::Vector<IR::Type>();
        for (auto ta : *expression->typeArguments) {
            auto taType = getTypeType(ta);
            if (taType == nullptr)
                return expression;
            typeArgs->push_back(taType);
        }
        auto callType = new IR::Type_MethodCall(expression->srcInfo,
                                                typeArgs, rettype, args);

        TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
        constraints.addEqualityConstraint(ft, callType);
        auto tvs = constraints.solve(expression);
        if (tvs == nullptr)
            return expression;
        addSubstitutions(tvs);

        LOG2("Method type before specialization " << methodType << " with " << tvs);
        TypeVariableSubstitutionVisitor substVisitor(tvs);
        auto specMethodType = methodType->apply(substVisitor);
        LOG2("Method type after specialization " << specMethodType);

        // construct types for the specMethodType, use a new typeChecker
        // that uses the same tables!
        auto *learn = clone();
        (void)specMethodType->apply(*learn);

        auto canon = getType(specMethodType);
        if (canon == nullptr)
            return expression;

        auto functionType = specMethodType->to<IR::Type_MethodBase>();
        BUG_CHECK(functionType != nullptr, "Method type is %1%", specMethodType);

        if (!functionType->is<IR::Type_Method>())
            BUG("Unexpected type for function %1%", functionType);

        auto returnType = tvs->lookup(rettype);
        if (returnType == nullptr) {
            typeError("Cannot infer return type %1%", expression);
            return expression;
        }
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
        auto result = cts.convert(expression)->to<IR::MethodCallExpression>();  // cast arguments
        if (::errorCount() > 0)
            return expression;
        setType(result, returnType);

        // Hopefull by now we know enough about the type to use MethodInstance.
        auto mi = MethodInstance::resolve(result, refMap, typeMap);
        if (mi->isApply()) {
            auto a = mi->to<ApplyMethod>();
            if (a->isTableApply() && findContext<IR::P4Action>())
                typeError("%1%: tables cannot be invoked from actions", expression);
        }

        // Check that verify is only invoked from parsers.
        if (auto ef = mi->to<ExternFunction>()) {
            if (ef->method->name == IR::ParserState::verify)
                if (!findContext<IR::P4Parser>())
                    typeError("%1%: may only be invoked in parsers", ef->expr);
            if (constArgs) {
                // extern functions with constant args are compile-time constants
                setCompileTimeConstant(expression);
                setCompileTimeConstant(getOriginal<IR::Expression>());
            }
        }

        if (mi->is<ExternMethod>())
            checkCorelibMethods(mi->to<ExternMethod>());

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
        } else {
            BUG("Unexpected type %1% for struct field %2%", field->type, field);
        }
    }
}

const IR::SelectCase*
TypeInference::matchCase(const IR::SelectExpression* select, const IR::Type_Tuple* selectType,
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
    if (!caseType->is<IR::Type_Tuple>()) {
        if (selectType->components.size() != 1) {
            typeError("Type mismatch %1% (%2%) vs %3% (%4%)",
                      select->select, selectType->toString(), selectCase, caseType->toString());
            return nullptr;
        }
        useSelType = selectType->components.at(0);
    }
    auto tvs = unify(select, useSelType, caseType);
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
    for (auto ct : tuple->components) {
        if (ct->is<IR::ITypeVar>()) {
            typeError("Cannot infer type for %1%", ct);
            return expression;
        } else if (containsHeader(ct)) {
            typeError("Expression with type %1% cannot be used in select %2%", ct, expression);
            return expression;
        }
    }

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
    if (!type->is<IR::Type_ActionEnum>()) {
        typeError("%1%: Switch condition can only be produced by table.apply(...).action_run",
                  stat);
        return stat;
    }
    auto ae = type->to<IR::Type_ActionEnum>();
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
        ktype = ktype->to<IR::Type_Newtype>()->type;
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

    if (actionListCall->arguments->size() > call->arguments->size()) {
        typeError("%1%: not enough arguments", call);
        return nullptr;
    }

    SameExpression se(refMap, typeMap);
    auto callInstance = MethodInstance::resolve(call, refMap, typeMap);
    auto listInstance = MethodInstance::resolve(actionListCall, refMap, typeMap);

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
