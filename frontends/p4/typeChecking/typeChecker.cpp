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

#include "constantTypeSubstitution.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "lib/cstring.h"
#include "lib/log.h"
#include "typeConstraints.h"
#include "typeSubstitution.h"
#include "typeUnification.h"

namespace P4 {

TypeChecking::TypeChecking(ReferenceMap *refMap, TypeMap *typeMap, bool updateExpressions) {
    addPasses({new P4::TypeInference(typeMap, /* readOnly */ true, /* checkArrays */ true,
                                     /* errorOnNullDecls */ true),
               updateExpressions ? new ApplyTypesToExpressions(typeMap) : nullptr,
               refMap ? new P4::ResolveReferences(refMap) : nullptr});
    setStopOnError(true);
}

//////////////////////////////////////////////////////////////////////////

bool TypeInference::learn(const IR::Node *node, Visitor *caller, const Visitor::Context *ctxt) {
    auto *learner = clone();
    learner->setCalledBy(caller);
    learner->setName("TypeInference learner");
    unsigned previous = ::errorCount();
    (void)node->apply(*learner, ctxt);
    unsigned errCount = ::errorCount();
    bool result = errCount > previous;
    if (result) typeError("Error while analyzing %1%", node);
    return result;
}

const IR::Expression *TypeInference::constantFold(const IR::Expression *expression) {
    if (readOnly) return expression;
    DoConstantFolding cf(this, typeMap, false);
    auto result = expression->apply(cf, getChildContext());
    learn(result, this, getChildContext());
    LOG3("Folded " << expression << " into " << result);
    return result;
}

// Make a clone of the type where all type variables in
// the type parameters are replaced with fresh ones.
// This should only be applied to canonical types.
const IR::Type *TypeInference::cloneWithFreshTypeVariables(const IR::IMayBeGenericType *type) {
    TypeVariableSubstitution tvs;
    for (auto v : type->getTypeParameters()->parameters) {
        auto tv = new IR::Type_Var(v->srcInfo, v->getName());
        bool b = tvs.setBinding(v, tv);
        BUG_CHECK(b, "%1%: failed replacing %2% with %3%", type, v, tv);
    }

    TypeVariableSubstitutionVisitor sv(&tvs, true);
    sv.setCalledBy(this);
    auto cl = type->to<IR::Type>()->apply(sv, getChildContext());
    CHECK_NULL(cl);
    learn(cl, this, getChildContext());
    LOG3("Cloned for type variables " << type << " into " << cl);
    return cl->to<IR::Type>();
}

TypeInference::TypeInference(TypeMap *typeMap, bool readOnly, bool checkArrays,
                             bool errorOnNullDecls)
    : typeMap(typeMap),
      initialNode(nullptr),
      readOnly(readOnly),
      checkArrays(checkArrays),
      errorOnNullDecls(errorOnNullDecls),
      currentActionList(nullptr) {
    CHECK_NULL(typeMap);
    visitDagOnce = false;  // the done() method will take care of this
}

TypeInference::TypeInference(TypeMap *typeMap,
                             std::shared_ptr<MinimalNameGenerator> inheritedNameGen)
    : TypeInference(typeMap, true) {
    nameGen = inheritedNameGen;
}

Visitor::profile_t TypeInference::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    if (node->is<IR::P4Program>()) {
        LOG2("TypeInference for " << dbp(node));
    }
    initialNode = node;
    if (!nameGen) {
        nameGen = std::make_shared<MinimalNameGenerator>();
        node->apply(*nameGen);
    }

    return rv;
}

void TypeInference::end_apply(const IR::Node *node) {
    BUG_CHECK(!readOnly || node == initialNode,
              "At this point in the compilation typechecking should not infer new types anymore, "
              "but it did.");
    typeMap->updateMap(node);
    if (node->is<IR::P4Program>()) LOG3("Typemap: " << std::endl << typeMap);
    Transform::end_apply(node);
}

const IR::Node *TypeInference::apply_visitor(const IR::Node *orig, const char *name) {
    const auto *transformed = Transform::apply_visitor(orig, name);
    BUG_CHECK(!readOnly || orig == transformed,
              "At this point in the compilation typechecking should not infer new types anymore, "
              "but it did: node %1% changed to %2%",
              orig, transformed);
    return transformed;
}

TypeInference *TypeInference::clone() const { return new TypeInference(this->typeMap, nameGen); }

bool TypeInference::done() const {
    auto orig = getOriginal();
    bool done = typeMap->contains(orig);
    LOG3("TI Visiting " << dbp(orig) << (done ? " done" : ""));
    return done;
}

const IR::Type *TypeInference::getType(const IR::Node *element) const {
    const IR::Type *result = typeMap->getType(element);
    // This should be happening only when type-checking already failed
    // for some node; we are now just trying to typecheck a parent node.
    // So an error should have already been signalled.
    if ((result == nullptr) && (::errorCount() == 0))
        BUG("Could not find type of %1% %2%", dbp(element), element);
    return result;
}

const IR::Type *TypeInference::getTypeType(const IR::Node *element) const {
    const IR::Type *result = typeMap->getType(element);
    // See comment in getType() above.
    if (result == nullptr) {
        if (::errorCount() == 0) BUG("Could not find type of %1%", element);
        return nullptr;
    }
    BUG_CHECK(result->is<IR::Type_Type>(), "%1%: expected a TypeType", dbp(result));
    return result->to<IR::Type_Type>()->type;
}

void TypeInference::setType(const IR::Node *element, const IR::Type *type) {
    typeMap->setType(element, type);
}

void TypeInference::addSubstitutions(const TypeVariableSubstitution *tvs) {
    typeMap->addSubstitutions(tvs);
}

TypeVariableSubstitution *TypeInference::unifyBase(
    bool allowCasts, const IR::Node *errorPosition, const IR::Type *destType,
    const IR::Type *srcType, std::string_view errorFormat,
    std::initializer_list<const IR::Node *> errorArgs) {
    CHECK_NULL(destType);
    CHECK_NULL(srcType);
    if (srcType == destType) return new TypeVariableSubstitution();

    TypeConstraint *constraint;
    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    if (allowCasts)
        constraint = new CanBeImplicitlyCastConstraint(destType, srcType, errorPosition);
    else
        constraint = new EqualityConstraint(destType, srcType, errorPosition);
    if (!errorFormat.empty()) constraint->setError(errorFormat, errorArgs);
    constraints.add(constraint);
    auto tvs = constraints.solve();
    addSubstitutions(tvs);
    return tvs;
}

template <class Ctor>
const IR::Type *TypeInference::canonicalizeFields(const IR::Type_StructLike *type,
                                                  Ctor constructor) {
    bool changes = false;
    auto fields = new IR::IndexedVector<IR::StructField>();
    for (auto field : type->fields) {
        auto ftype = canonicalize(field->type);
        if (ftype == nullptr) return nullptr;
        if (ftype != field->type) changes = true;
        BUG_CHECK(!ftype->is<IR::Type_Type>(), "%1%: TypeType in field type", ftype);
        auto newField = new IR::StructField(field->srcInfo, field->name, field->annotations, ftype);
        fields->push_back(newField);
    }
    if (changes)
        return constructor(fields);
    else
        return type;
}

/**
 * Bind the parameters with the specified arguments.
 * For example, given a type
 * void _<T>(T data)
 * it can be specialized to
 * void _<int<32>>(int<32> data);
 */
const IR::Type *TypeInference::specialize(const IR::IMayBeGenericType *type,
                                          const IR::Vector<IR::Type> *arguments,
                                          const Visitor::Context *ctxt) {
    TypeVariableSubstitution *bindings = new TypeVariableSubstitution();
    bool success = bindings->setBindings(type->getNode(), type->getTypeParameters(), arguments);
    if (!success) return nullptr;

    LOG2("Translation map\n" << bindings);

    TypeVariableSubstitutionVisitor tsv(bindings);
    const IR::Node *result = type->getNode()->apply(tsv, ctxt);
    if (result == nullptr) return nullptr;

    LOG2("Specialized " << type << "\n\tinto " << result);
    return result->to<IR::Type>();
}

// May return nullptr if a type error occurs.
const IR::Type *TypeInference::canonicalize(const IR::Type *type) {
    if (type == nullptr) return nullptr;

    auto exists = typeMap->getType(type);
    if (exists != nullptr) {
        if (auto *tt = exists->to<IR::Type_Type>()) return tt->type;
        return exists;
    }
    if (auto tt = type->to<IR::Type_Type>()) type = tt->type;

    if (type->is<IR::Type_SpecializedCanonical>() || type->is<IR::Type_InfInt>() ||
        type->is<IR::Type_Action>() || type->is<IR::Type_Error>() || type->is<IR::Type_Newtype>() ||
        type->is<IR::Type_Table>() || type->is<IR::Type_Any>()) {
        return type;
    } else if (type->is<IR::Type_Dontcare>()) {
        return IR::Type_Dontcare::get();
    } else if (type->is<IR::Type_Base>()) {
        auto *tb = type->to<IR::Type_Bits>();
        if (!tb)
            // all other base types are singletons
            return type;
        auto canon = IR::Type_Bits::get(tb->size, tb->isSigned);
        if (!typeMap->contains(canon)) typeMap->setType(canon, new IR::Type_Type(canon));
        return canon;
    } else if (type->is<IR::Type_Enum>() || type->is<IR::Type_SerEnum>() ||
               type->is<IR::Type_ActionEnum>() || type->is<IR::Type_MatchKind>()) {
        return type;
    } else if (auto set = type->to<IR::Type_Set>()) {
        auto et = canonicalize(set->elementType);
        if (et == nullptr) return nullptr;
        if (et->is<IR::Type_Stack>() || et->is<IR::Type_Set>() || et->is<IR::Type_HeaderUnion>())
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: Sets of %2% are not supported", type, et);
        if (et == set->elementType) return type;
        const IR::Type *canon = new IR::Type_Set(type->srcInfo, et);
        return canon;
    } else if (auto stack = type->to<IR::Type_Stack>()) {
        auto et = canonicalize(stack->elementType);
        if (et == nullptr) return nullptr;
        const IR::Type *canon;
        if (et == stack->elementType)
            canon = type;
        else
            canon = new IR::Type_Stack(stack->srcInfo, et, stack->size);
        canon = typeMap->getCanonical(canon);
        return canon;
    } else if (auto vec = type->to<IR::Type_P4List>()) {
        auto et = canonicalize(vec->elementType);
        if (et == nullptr) return nullptr;
        const IR::Type *canon;
        if (et == vec->elementType)
            canon = type;
        else
            canon = new IR::Type_P4List(vec->srcInfo, et);
        canon = typeMap->getCanonical(canon);
        return canon;
    } else if (auto list = type->to<IR::Type_List>()) {
        auto fields = new IR::Vector<IR::Type>();
        // list<set<a>, b> = set<tuple<a, b>>
        // TODO: this should not be done here.
        bool anySet = false;
        bool anyChange = false;
        for (auto t : list->components) {
            if (auto *set = t->to<IR::Type_Set>()) {
                anySet = true;
                t = set->elementType;
            }
            auto t1 = canonicalize(t);
            anyChange = anyChange || t != t1;
            if (t1 == nullptr) return nullptr;
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
        if (anySet) canon = new IR::Type_Set(type->srcInfo, canon);
        return canon;
    } else if (auto tuple = type->to<IR::Type_Tuple>()) {
        auto fields = new IR::Vector<IR::Type>();
        bool anyChange = false;
        for (auto t : tuple->components) {
            auto t1 = canonicalize(t);
            anyChange = anyChange || t != t1;
            if (t1 == nullptr) return nullptr;
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
        if (pl == nullptr || tps == nullptr) return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages)) return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Parser(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto tp = type->to<IR::Type_Control>()) {
        auto pl = canonicalizeParameters(tp->applyParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr) return nullptr;
        if (!checkParameters(pl, forbidModules, forbidPackages)) return nullptr;
        if (pl != tp->applyParams || tps != tp->typeParameters)
            return new IR::Type_Control(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto tp = type->to<IR::Type_Package>()) {
        auto pl = canonicalizeParameters(tp->constructorParams);
        auto tps = tp->typeParameters;
        if (pl == nullptr || tps == nullptr) return nullptr;
        if (pl != tp->constructorParams || tps != tp->typeParameters)
            return new IR::Type_Package(tp->srcInfo, tp->name, tp->annotations, tps, pl);
        return type;
    } else if (auto cont = type->to<IR::P4Control>()) {
        auto ctype0 = getTypeType(cont->type);
        if (ctype0 == nullptr) return nullptr;
        auto ctype = ctype0->to<IR::Type_Control>();
        auto pl = canonicalizeParameters(cont->constructorParams);
        if (pl == nullptr) return nullptr;
        if (ctype != cont->type || pl != cont->constructorParams)
            return new IR::P4Control(cont->srcInfo, cont->name, ctype, pl, cont->controlLocals,
                                     cont->body);
        return type;
    } else if (auto p = type->to<IR::P4Parser>()) {
        auto ctype0 = getTypeType(p->type);
        if (ctype0 == nullptr) return nullptr;
        auto ctype = ctype0->to<IR::Type_Parser>();
        auto pl = canonicalizeParameters(p->constructorParams);
        if (pl == nullptr) return nullptr;
        if (ctype != p->type || pl != p->constructorParams)
            return new IR::P4Parser(p->srcInfo, p->name, ctype, pl, p->parserLocals, p->states);
        return type;
    } else if (auto te = type->to<IR::Type_Extern>()) {
        bool changes = false;
        auto methods = new IR::Vector<IR::Method>();
        for (auto method : te->methods) {
            auto fpType = canonicalize(method->type);
            if (fpType == nullptr) return nullptr;

            if (fpType != method->type) {
                method =
                    new IR::Method(method->srcInfo, method->name, fpType->to<IR::Type_Method>(),
                                   method->isAbstract, method->annotations);
                changes = true;
                setType(method, fpType);
            }

            methods->push_back(method);
        }
        auto tps = te->typeParameters;
        if (tps == nullptr) return nullptr;
        const IR::Type *resultType = type;
        if (changes || tps != te->typeParameters)
            resultType = new IR::Type_Extern(te->srcInfo, te->name, tps, *methods, te->annotations);
        return resultType;
    } else if (auto mt = type->to<IR::Type_Method>()) {
        const IR::Type *res = nullptr;
        if (mt->returnType != nullptr) {
            res = canonicalize(mt->returnType);
            if (res == nullptr) return nullptr;
        }
        bool changes = res != mt->returnType;
        auto pl = canonicalizeParameters(mt->parameters);
        auto tps = mt->typeParameters;
        if (pl == nullptr) return nullptr;
        if (!checkParameters(pl)) return nullptr;
        changes = changes || pl != mt->parameters || tps != mt->typeParameters;
        const IR::Type *resultType = mt;
        if (changes) resultType = new IR::Type_Method(mt->getSourceInfo(), tps, res, pl, mt->name);
        return resultType;
    } else if (auto hdr = type->to<IR::Type_Header>()) {
        return canonicalizeFields(hdr, [hdr](const IR::IndexedVector<IR::StructField> *fields) {
            return new IR::Type_Header(hdr->srcInfo, hdr->name, hdr->annotations,
                                       hdr->typeParameters, *fields);
        });
    } else if (auto str = type->to<IR::Type_Struct>()) {
        return canonicalizeFields(str, [str](const IR::IndexedVector<IR::StructField> *fields) {
            return new IR::Type_Struct(str->srcInfo, str->name, str->annotations,
                                       str->typeParameters, *fields);
        });
    } else if (auto hu = type->to<IR::Type_HeaderUnion>()) {
        return canonicalizeFields(hu, [hu](const IR::IndexedVector<IR::StructField> *fields) {
            return new IR::Type_HeaderUnion(hu->srcInfo, hu->name, hu->annotations,
                                            hu->typeParameters, *fields);
        });
    } else if (auto su = type->to<IR::Type_UnknownStruct>()) {
        return canonicalizeFields(su, [su](const IR::IndexedVector<IR::StructField> *fields) {
            return new IR::Type_UnknownStruct(su->srcInfo, su->name, su->annotations, *fields);
        });
    } else if (auto st = type->to<IR::Type_Specialized>()) {
        auto baseCanon = canonicalize(st->baseType);
        if (baseCanon == nullptr) return nullptr;
        if (st->arguments == nullptr) return baseCanon;

        if (!baseCanon->is<IR::IMayBeGenericType>()) {
            typeError(
                "%1%: Type %2% is not generic and thus it"
                " cannot be specialized using type arguments",
                type, baseCanon);
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
        for (const IR::Type *a : *st->arguments) {
            auto atype = getTypeType(a);
            if (atype == nullptr) return nullptr;
            auto checkType = atype;
            if (auto tsc = atype->to<IR::Type_SpecializedCanonical>()) checkType = tsc->baseType;
            if (checkType->is<IR::Type_Control>() || checkType->is<IR::Type_Parser>() ||
                checkType->is<IR::Type_Package>() || checkType->is<IR::P4Parser>() ||
                checkType->is<IR::P4Control>()) {
                typeError("%1%: Cannot use %2% as a type parameter", type, checkType);
                return nullptr;
            }
            const IR::Type *canon = canonicalize(atype);
            if (canon == nullptr) return nullptr;
            args->push_back(canon);
        }
        auto specialized = specialize(gt, args, getChildContext());

        auto result =
            new IR::Type_SpecializedCanonical(type->srcInfo, baseCanon, args, specialized);
        LOG2("Scanning the specialized type");
        learn(result, this, getChildContext());
        return result;
    } else {
        BUG_CHECK(::errorCount(), "Unexpected type %1%", dbp(type));
    }

    // If we reach this point some type error must have occurred, because
    // the typeMap lookup at the beginning of the function has failed.
    return nullptr;
}

///////////////////////////////////// Visitor methods

const IR::Node *TypeInference::preorder(IR::P4Program *program) {
    currentActionList = nullptr;
    if (typeMap->checkMap(getOriginal()) && readOnly) {
        LOG2("No need to typecheck");
        prune();
    }
    return program;
}

const IR::Node *TypeInference::postorder(IR::Declaration_MatchKind *decl) {
    if (done()) return decl;
    for (auto id : *decl->getDeclarations()) setType(id->getNode(), IR::Type_MatchKind::get());
    return decl;
}

bool TypeInference::canCastBetween(const IR::Type *dest, const IR::Type *src) const {
    if (src->is<IR::Type_Action>()) return false;
    if (typeMap->equivalent(src, dest)) return true;

    if (auto *nt = dest->to<IR::Type_Newtype>()) {
        auto dt = getTypeType(nt->type);
        if (typeMap->equivalent(dt, src)) return true;
    }

    if (auto *f = src->to<IR::Type_Bits>()) {
        if (auto *t = dest->to<IR::Type_Bits>()) {
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
        if (auto *b = dest->to<IR::Type_Bits>()) {
            return b->size == 1 && !b->isSigned;
        }
    } else if (src->is<IR::Type_InfInt>()) {
        return dest->is<IR::Type_Bits>() || dest->is<IR::Type_Boolean>() ||
               dest->is<IR::Type_InfInt>();
    } else if (auto *nt = src->to<IR::Type_Newtype>()) {
        auto st = getTypeType(nt->type);
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

const IR::Expression *TypeInference::assignment(const IR::Node *errorPosition,
                                                const IR::Type *destType,
                                                const IR::Expression *sourceExpression) {
    if (destType->is<IR::Type_Unknown>()) BUG("Unknown destination type");
    if (destType->is<IR::Type_Dontcare>()) return sourceExpression;
    const IR::Type *initType = getType(sourceExpression);
    if (initType == nullptr) return sourceExpression;

    auto tvs = unifyCast(errorPosition, destType, initType,
                         "Source expression '%1%' produces a result of type '%2%' which cannot be "
                         "assigned to a left-value with type '%3%'",
                         {sourceExpression, initType, destType});
    if (tvs == nullptr)
        // error already signalled
        return sourceExpression;
    if (!tvs->isIdentity()) {
        ConstantTypeSubstitution cts(tvs, typeMap, this);
        sourceExpression = cts.convert(sourceExpression, getChildContext());  // sets type
    }
    if (destType->is<IR::Type_SerEnum>() && !typeMap->equivalent(destType, initType) &&
        !initType->is<IR::Type_Any>()) {
        typeError("'%1%': values of type '%2%' cannot be implicitly cast to type '%3%'",
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
    if (auto tsc = destType->to<IR::Type_SpecializedCanonical>()) concreteType = tsc->substituted;
    bool cst = isCompileTimeConstant(sourceExpression);
    if (auto ts = concreteType->to<IR::Type_StructLike>()) {
        auto si = sourceExpression->to<IR::StructExpression>();
        auto type = destType->getP4Type();
        setType(type, new IR::Type_Type(destType));
        if (initType->is<IR::Type_UnknownStruct>() ||
            (si != nullptr && initType->is<IR::Type_Struct>())) {
            // Even if the structure is a struct expression with the right type,
            // we still need to recurse over its fields; they many not have
            // the right type.
            CHECK_NULL(si);
            bool hasDots = si->containsDots();
            if (ts->fields.size() != si->components.size() && !hasDots) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, ts->fields.size(), si->components.size());
                return sourceExpression;
            }
            IR::IndexedVector<IR::NamedExpression> vec;
            bool changes = false;
            for (const IR::StructField *fieldI : ts->fields) {
                auto compI = si->components.getDeclaration<IR::NamedExpression>(fieldI->name);
                if (hasDots && compI == nullptr) {
                    continue;
                }
                auto src = assignment(sourceExpression, fieldI->type, compI->expression);
                if (src != compI->expression) changes = true;
                vec.push_back(new IR::NamedExpression(fieldI->name, src));
            }
            if (hasDots) vec.push_back(si->getField("..."_cs));
            if (!changes) vec = si->components;
            if (initType->is<IR::Type_UnknownStruct>() || changes) {
                sourceExpression = new IR::StructExpression(type, type, vec);
                setType(sourceExpression, destType);
            }
        } else if (auto li = sourceExpression->to<IR::ListExpression>()) {
            bool hasDots = li->containsDots();
            if (ts->fields.size() != li->components.size() && !hasDots) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, ts->fields.size(), li->components.size());
                return sourceExpression;
            }
            IR::IndexedVector<IR::NamedExpression> vec;
            size_t listSize = li->size();
            for (size_t i = 0; i < ts->fields.size(); i++) {
                auto fieldI = ts->fields.at(i);
                auto compI = li->components.at(i);
                if (hasDots && i == listSize - 1) {
                    auto nd = new IR::NamedDots(fieldI->srcInfo, compI->to<IR::Dots>());
                    vec.push_back(nd);
                    break;
                }
                auto src = assignment(sourceExpression, fieldI->type, compI);
                vec.push_back(new IR::NamedExpression(fieldI->name, src));
            }
            sourceExpression = new IR::StructExpression(type, type, vec);
            setType(sourceExpression, destType);
            assignment(errorPosition, destType, sourceExpression);
        }
        // else this is some other expression that evaluates to a struct
        if (cst) setCompileTimeConstant(sourceExpression);
    } else if (auto tt = concreteType->to<IR::Type_Tuple>()) {
        if (auto li = sourceExpression->to<IR::ListExpression>()) {
            bool hasDots = li->containsDots();
            if (tt->getSize() != li->components.size() && !hasDots) {
                typeError("%1%: destination type expects %2% fields, but source only has %3%",
                          errorPosition, tt->getSize(), li->components.size());
                return sourceExpression;
            }
            bool changed = false;
            IR::Vector<IR::Expression> vec;
            size_t sourceSize = li->size();
            for (size_t i = 0; i < tt->getSize(); i++) {
                auto typeI = tt->at(i);
                auto compI = li->components.at(i);
                if (hasDots && (i == sourceSize - 1)) {
                    vec.push_back(compI);  // this is '...'
                    break;
                }
                auto src = assignment(sourceExpression, typeI, compI);
                if (src != compI) changed = true;
                vec.push_back(src);
            }
            if (changed) {
                sourceExpression = new IR::ListExpression(vec);
                setType(sourceExpression, destType);
                if (cst) setCompileTimeConstant(sourceExpression);
            }
        }
        // else this is some other expression that evaluates to a tuple
    } else if (auto tstack = concreteType->to<IR::Type_Stack>()) {
        if (auto li = sourceExpression->to<IR::ListExpression>()) {
            bool hasDots = li->containsDots();
            if (tstack->getSize() != li->components.size() && !hasDots) {
                typeError("%1%: destination type expects %2% elements, but source only has %3%",
                          errorPosition, tstack->getSize(), li->components.size());
                return sourceExpression;
            }
            IR::Vector<IR::Expression> vec;
            size_t sourceSize = li->size();
            auto elementType = tstack->elementType;
            for (size_t i = 0; i < tstack->getSize(); i++) {
                auto compI = li->components.at(i);
                if (hasDots && (i == sourceSize - 1)) {
                    vec.push_back(compI);  // this is '...'
                    break;
                }
                auto src = assignment(sourceExpression, elementType, compI);
                vec.push_back(src);
            }
            auto p4stack = tstack->getP4Type();
            sourceExpression = new IR::HeaderStackExpression(p4stack, vec, p4stack);
            setType(sourceExpression, destType);
            if (cst) setCompileTimeConstant(sourceExpression);
        }
    }

    return sourceExpression;
}

const IR::Node *TypeInference::postorder(IR::Annotation *annotation) {
    auto checkAnnotation = [this](const IR::Expression *e) {
        if (!isCompileTimeConstant(e))
            typeError("%1%: structured annotation must be compile-time constant values", e);
        auto t = getType(e);
        if (!t)
            // type error during typechecking
            return;
        if (!t->is<IR::Type_InfInt>() && !t->is<IR::Type_String>() && !t->is<IR::Type_Boolean>())
            typeError("%1%: illegal type for structured annotation; must be int, string or bool",
                      e);
    };

    if (annotation->structured) {
        // If it happens here it was created in the compiler, so it's a bug, not an error.
        BUG_CHECK(annotation->expr.empty() || annotation->kv.empty(),
                  "%1%: structured annotations cannot contain expressions and kv-pairs",
                  annotation);
        for (auto e : annotation->expr) checkAnnotation(e);
        for (auto e : annotation->kv) checkAnnotation(e->expression);
    }
    return annotation;
}

// Returns the type of the constructed object and
// new arguments for constructor, which may have inserted casts.
std::pair<const IR::Type *, const IR::Vector<IR::Argument> *> TypeInference::checkExternConstructor(
    const IR::Node *errorPosition, const IR::Type_Extern *ext,
    const IR::Vector<IR::Argument> *arguments) {
    auto none = std::pair<const IR::Type *, const IR::Vector<IR::Argument> *>(nullptr, nullptr);
    auto freshExtern = cloneWithFreshTypeVariables(ext)->to<IR::Type_Extern>();
    auto constructor = freshExtern->lookupConstructor(arguments);
    if (constructor == nullptr) {
        typeError("%1%: type %2% has no matching constructor", errorPosition, ext);
        return none;
    }
    auto mt = getType(constructor);
    if (mt == nullptr) return none;
    auto methodType = mt->to<IR::Type_Method>();
    BUG_CHECK(methodType != nullptr, "Constructor does not have a method type, but %1%", mt);

    if (errorPosition->is<IR::ConstructorCallExpression>()) {
        for (auto m : ext->methods) {
            if (m->isAbstract) {
                typeError(
                    "%1%: extern type %2% with abstract methods cannot be instantiated"
                    " using a constructor call; consider using a declaration",
                    errorPosition, ext);
                return none;
            }
        }
    }

    auto args = new IR::Vector<IR::ArgumentInfo>();
    size_t i = 0;
    for (auto pi : *methodType->parameters->getEnumerator()) {
        if (i >= arguments->size()) {
            BUG_CHECK(pi->isOptional() || pi->defaultValue != nullptr, "Missing nonoptional arg %s",
                      pi);
            break;
        }
        auto arg = arguments->at(i++);
        if (!isCompileTimeConstant(arg->expression))
            typeError("%1%: cannot evaluate to a compile-time constant", arg);
        auto argType = getType(arg->expression);
        auto paramType = getType(pi);
        if (argType == nullptr || paramType == nullptr) return none;

        if (paramType->is<IR::Type_Control>() || paramType->is<IR::Type_Parser>() ||
            paramType->is<IR::P4Control>() || paramType->is<IR::P4Parser>() ||
            paramType->is<IR::Type_Package>())
            typeError("%1%: parameter cannot have type %2%", pi, paramType);

        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg->expression, true, argType, arg);
        args->push_back(argInfo);
    }

    // will always be bound to Type_Void.
    auto rettype = new IR::Type_Var(IR::ID(nameGen->newName("R"), "<returned type>"_cs));
    auto callType =
        new IR::Type_MethodCall(errorPosition->srcInfo, new IR::Vector<IR::Type>(), rettype, args);
    auto tvs = unify(errorPosition, methodType, callType,
                     "Constructor invocation %1% does not match constructor declaration %2%",
                     {callType, constructor});
    BUG_CHECK(tvs != nullptr || ::errorCount(), "Unification failed with no error");
    if (tvs == nullptr) return none;

    LOG2("Constructor type before specialization " << methodType << " with " << tvs);
    TypeVariableSubstitutionVisitor substVisitor(tvs);
    substVisitor.setCalledBy(this);
    auto specMethodType = methodType->apply(substVisitor, getChildContext());
    LOG2("Constructor type after specialization " << specMethodType);
    learn(specMethodType, this, getChildContext());
    auto canon = getType(specMethodType);
    if (canon == nullptr) return none;

    auto functionType = specMethodType->to<IR::Type_MethodBase>();
    BUG_CHECK(functionType != nullptr, "Method type is %1%", specMethodType);
    if (!functionType->is<IR::Type_Method>()) BUG("Unexpected type for function %1%", functionType);

    ConstantTypeSubstitution cts(tvs, typeMap, this);
    // Arguments may need to be cast, e.g., list expression to a
    // header type.
    auto paramIt = functionType->parameters->begin();
    auto newArgs = new IR::Vector<IR::Argument>();
    bool changed = false;
    for (auto arg : *arguments) {
        cstring argName = arg->name.name;
        bool named = !argName.isNullOrEmpty();
        const IR::Parameter *param;

        if (named) {
            param = functionType->parameters->getParameter(argName);
        } else {
            BUG_CHECK(paramIt != functionType->parameters->end(), "Not enough parameters %1%",
                      errorPosition);
            param = *paramIt;
        }

        auto newExpr = arg->expression;
        if (param->direction == IR::Direction::In || param->direction == IR::Direction::None) {
            // This is like an assignment; may make additional conversions.
            newExpr = assignment(arg, param->type, arg->expression);
        } else {
            // Insert casts for 'int' values.
            newExpr = cts.convert(newExpr, getChildContext())->to<IR::Expression>();
        }
        if (::errorCount() > 0) return none;
        if (newExpr != arg->expression) {
            LOG2("Changing method argument to " << newExpr);
            changed = true;
            newArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, newExpr));
        } else {
            newArgs->push_back(arg);
        }
        if (!named) ++paramIt;
    }
    if (changed) arguments = newArgs;
    auto objectType = new IR::Type_Extern(ext->srcInfo, ext->name, methodType->typeParameters,
                                          freshExtern->methods);
    learn(objectType, this, getChildContext());
    return {objectType, arguments};
}

/// @returns: A pair containing the type returned by the constructor and the new arguments
///           (which may change due to insertion of casts).
std::pair<const IR::Type *, const IR::Vector<IR::Argument> *> TypeInference::containerInstantiation(
    const IR::Node *node,  // can be Declaration_Instance or ConstructorCallExpression
    const IR::Vector<IR::Argument> *constructorArguments, const IR::IContainer *container) {
    CHECK_NULL(node);
    CHECK_NULL(constructorArguments);
    CHECK_NULL(container);
    auto constructor = container->getConstructorMethodType();
    constructor = cloneWithFreshTypeVariables(constructor->to<IR::IMayBeGenericType>())
                      ->to<IR::Type_Method>();
    CHECK_NULL(constructor);

    {
        // Constructor parameters that are infint are also treated as type variables
        TypeVariableSubstitution tvs;
        auto params = constructor->parameters;
        for (auto param : params->parameters) {
            if (auto v = param->type->to<IR::Type_InfInt>()) {
                auto tv = IR::Type_InfInt::get(param->type->srcInfo);
                bool b = tvs.setBinding(v, tv);
                BUG_CHECK(b, "failed replacing %2% with %3%", v, tv);
            }
        }
        TypeVariableSubstitutionVisitor sv(&tvs, true);
        sv.setCalledBy(this);
        constructor = constructor->apply(sv, getChildContext())->to<IR::Type_Method>();
        CHECK_NULL(constructor);
        learn(constructor, this, getChildContext());
        LOG3("Cloned constructor arguments " << constructor);
    }

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
            return std::pair<const IR::Type *, const IR::Vector<IR::Argument> *>(nullptr, nullptr);
        auto argInfo = new IR::ArgumentInfo(arg->srcInfo, arg, true, argType, aarg);
        args->push_back(argInfo);
    }
    auto rettype = new IR::Type_Var(IR::ID(nameGen->newName("<any>")));
    // There are never type arguments at this point; if they exist, they have been folded
    // into the constructor by type specialization.
    auto callType =
        new IR::Type_MethodCall(node->srcInfo, new IR::Vector<IR::Type>(), rettype, args);

    auto tvs = unify(container->getNode(), constructor, callType,
                     "Constructor invocation '%1%' does not match declaration '%2%'",
                     {callType, constructor});
    BUG_CHECK(tvs != nullptr || ::errorCount(), "Null substitution");
    if (tvs == nullptr)
        return std::pair<const IR::Type *, const IR::Vector<IR::Argument> *>(nullptr, nullptr);

    // Infer Dont_Care for type vars used only in not-present optional params
    auto dontCares = new TypeVariableSubstitution();
    auto typeParams = constructor->typeParameters;
    for (auto p : params->parameters) {
        if (!p->isOptional()) continue;
        forAllMatching<IR::Type_Var>(p, [tvs, dontCares, typeParams](const IR::Type_Var *tv) {
            if (tvs->lookup(tv)) return;                            // already bound
            if (typeParams->getDeclByName(tv->name) != tv) return;  // not a tv of this call
            dontCares->setBinding(tv, IR::Type_Dontcare::get());
        });
    }
    addSubstitutions(dontCares);

    ConstantTypeSubstitution cts(tvs, typeMap, this);
    auto newArgs = cts.convert(constructorArguments, getChildContext());

    auto returnType = tvs->lookup(rettype);
    BUG_CHECK(returnType != nullptr, "Cannot infer constructor result type %1%", node);
    return std::pair<const IR::Type *, const IR::Vector<IR::Argument> *>(returnType, newArgs);
}

const IR::Node *TypeInference::postorder(IR::Argument *arg) {
    if (done()) return arg;
    auto type = getType(arg->expression);
    if (type == nullptr) return arg;
    setType(getOriginal(), type);
    setType(arg, type);
    return arg;
}

}  // namespace P4
