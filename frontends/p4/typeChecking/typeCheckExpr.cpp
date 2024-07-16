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

#include "constantTypeSubstitution.h"
#include "frontends/p4/enumInstance.h"
#include "lib/algorithm.h"
#include "typeChecker.h"
#include "typeConstraints.h"

namespace P4 {

using namespace literals;

const IR::Node *TypeInference::postorder(IR::Parameter *param) {
    if (done()) return param;
    const IR::Type *paramType = getTypeType(param->type);
    if (paramType == nullptr) return param;
    BUG_CHECK(!paramType->is<IR::Type_Type>(), "%1%: unexpected type", paramType);

    if (paramType->is<IR::P4Control>() || paramType->is<IR::P4Parser>()) {
        typeError("%1%: parameter cannot have type %2%", param, paramType);
        return param;
    }

    if (!readOnly && paramType->is<IR::Type_InfInt>()) {
        // We only give these errors if we are no in 'readOnly' mode:
        // this prevents giving a confusing error message to the user.
        if (param->direction != IR::Direction::None) {
            typeError("%1%: parameters with type %2% must be directionless", param, paramType);
            return param;
        }
        if (findContext<IR::P4Action>()) {
            typeError("%1%: actions cannot have parameters with type %2%", param, paramType);
            return param;
        }
    }

    // The parameter type cannot have free type variables
    if (auto *gen = paramType->to<IR::IMayBeGenericType>()) {
        auto tp = gen->getTypeParameters();
        if (!tp->empty()) {
            typeError("Type parameters needed for %1%", param->name);
            return param;
        }
    }

    if (param->defaultValue) {
        if (!typeMap->isCompileTimeConstant(param->defaultValue))
            typeError("%1%: expression must be a compile-time constant", param->defaultValue);
    }

    setType(getOriginal(), paramType);
    setType(param, paramType);
    return param;
}

const IR::Node *TypeInference::postorder(IR::Constant *expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->type);
    if (type == nullptr) return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    setCompileTimeConstant(expression);
    return expression;
}

const IR::Node *TypeInference::postorder(IR::StringLiteral *expression) {
    if (done()) return expression;
    setType(getOriginal(), IR::Type_String::get());
    setType(expression, IR::Type_String::get());
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::BoolLiteral *expression) {
    if (done()) return expression;
    setType(getOriginal(), IR::Type_Boolean::get());
    setType(expression, IR::Type_Boolean::get());
    setCompileTimeConstant(getOriginal<IR::Expression>());
    setCompileTimeConstant(expression);
    return expression;
}

bool TypeInference::containsActionEnum(const IR::Type *type) const {
    if (auto st = type->to<IR::Type_Struct>()) {
        if (auto field = st->getField(IR::Type_Table::action_run)) {
            auto ft = getTypeType(field->type);
            if (ft->is<IR::Type_ActionEnum>()) return true;
        }
    }
    return false;
}

// Returns false on error
bool TypeInference::compare(const IR::Node *errorPosition, const IR::Type *ltype,
                            const IR::Type *rtype, Comparison *compare) {
    if (ltype->is<IR::Type_Action>() || rtype->is<IR::Type_Action>()) {
        // Actions return Type_Action instead of void.
        typeError("%1% and %2% cannot be compared", compare->left, compare->right);
        return false;
    }
    if (ltype->is<IR::Type_Table>() || rtype->is<IR::Type_Table>()) {
        typeError("%1% and %2%: tables cannot be compared", compare->left, compare->right);
        return false;
    }
    if (ltype->is<IR::Type_Extern>() || rtype->is<IR::Type_Extern>()) {
        typeError("%1% and %2%: externs cannot be compared", compare->left, compare->right);
        return false;
    }
    if (containsActionEnum(ltype) || containsActionEnum(rtype)) {
        typeError("%1% and %2%: table application results cannot be compared", compare->left,
                  compare->right);
        return false;
    }

    bool defined = false;
    if (typeMap->equivalent(ltype, rtype) &&
        (!ltype->is<IR::Type_Void>() && !ltype->is<IR::Type_Varbits>()) &&
        !ltype->to<IR::Type_UnknownStruct>()) {
        defined = true;
    } else if (ltype->is<IR::Type_Base>() && rtype->is<IR::Type_Base>() &&
               typeMap->equivalent(ltype, rtype)) {
        defined = true;
    } else if (ltype->is<IR::Type_BaseList>() && rtype->is<IR::Type_BaseList>()) {
        auto tvs = unify(errorPosition, ltype, rtype);
        if (tvs == nullptr) return false;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap, this);
            compare->left = cts.convert(compare->left, getChildContext());
            compare->right = cts.convert(compare->right, getChildContext());
        }
        defined = true;
    } else if (auto se = rtype->to<IR::Type_SerEnum>()) {
        // This can only happen in a switch statement, other comparisons
        // eliminate SerEnums before calling here.
        if (typeMap->equivalent(ltype, se->type)) defined = true;
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
            TypeVariableSubstitution *tvs;
            if (ls == nullptr) {
                tvs = unify(errorPosition, ltype, rtype);
            } else {
                tvs = unify(errorPosition, rtype, ltype);
            }
            if (tvs == nullptr) return false;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, typeMap, this);
                compare->left = cts.convert(compare->left, getChildContext());
                compare->right = cts.convert(compare->right, getChildContext());
            }

            if (ls != nullptr) {
                auto l = compare->left->to<IR::StructExpression>();
                CHECK_NULL(l);  // struct initializers are the only expressions that can
                // have StructUnknown types
                BUG_CHECK(rtype->is<IR::Type_StructLike>(), "%1%: expected a struct", rtype);
                auto type = new IR::Type_Name(rtype->to<IR::Type_StructLike>()->name);
                compare->left =
                    new IR::StructExpression(compare->left->srcInfo, type, type, l->components);
                setType(compare->left, rtype);
                if (lcst) setCompileTimeConstant(compare->left);
            } else {
                auto r = compare->right->to<IR::StructExpression>();
                CHECK_NULL(r);  // struct initializers are the only expressions that can
                // have StructUnknown types
                BUG_CHECK(ltype->is<IR::Type_StructLike>(), "%1%: expected a struct", ltype);
                auto type = new IR::Type_Name(ltype->to<IR::Type_StructLike>()->name);
                compare->right =
                    new IR::StructExpression(compare->right->srcInfo, type, type, r->components);
                setType(compare->right, rtype);
                if (rcst) setCompileTimeConstant(compare->right);
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
            if (tvs == nullptr) return false;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, typeMap, this);
                compare->left = cts.convert(compare->left, getChildContext());
                compare->right = cts.convert(compare->right, getChildContext());
            }
            defined = true;
        }
    }

    if (!defined) {
        typeError("'%1%' with type '%2%' cannot be compared to '%3%' with type '%4%'",
                  compare->left, ltype, compare->right, rtype);
        return false;
    }
    return true;
}

const IR::Node *TypeInference::postorder(IR::Operation_Relation *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

    bool equTest = expression->is<IR::Equ>() || expression->is<IR::Neq>();
    if (auto l = ltype->to<IR::Type_SerEnum>()) ltype = getTypeType(l->type);
    if (auto r = rtype->to<IR::Type_SerEnum>()) rtype = getTypeType(r->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_InfInt>()) {
        // This can happen because we are replacing some constant functions with
        // constants during type checking
        auto result = constantFold(expression);
        setType(getOriginal(), IR::Type_Boolean::get());
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    } else if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_Bits>()) {
        auto e = expression->clone();
        e->left = new IR::Cast(e->left->srcInfo, rtype, e->left);
        setType(e->left, rtype);
        ltype = rtype;
        expression = e;
    } else if (rtype->is<IR::Type_InfInt>() && ltype->is<IR::Type_Bits>()) {
        auto e = expression->clone();
        e->right = new IR::Cast(e->right->srcInfo, ltype, e->right);
        setType(e->right, ltype);
        rtype = ltype;
        expression = e;
    }

    if (equTest) {
        Comparison c;
        c.left = expression->left;
        c.right = expression->right;
        auto b = compare(expression, ltype, rtype, &c);
        if (!b) return expression;
        expression->left = c.left;
        expression->right = c.right;
    } else {
        if (!ltype->is<IR::Type_Bits>() || !rtype->is<IR::Type_Bits>() || !(ltype->equiv(*rtype))) {
            typeError("%1%: not defined on %2% and %3%", expression, ltype->toString(),
                      rtype->toString());
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

const IR::Node *TypeInference::postorder(IR::Concat *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

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

    bool castLeft = false;
    bool castRight = false;
    if (auto se = ltype->to<IR::Type_SerEnum>()) {
        ltype = getTypeType(se->type);
        castLeft = true;
    }
    if (auto se = rtype->to<IR::Type_SerEnum>()) {
        rtype = getTypeType(se->type);
        castRight = true;
    }
    if (ltype == nullptr || rtype == nullptr) {
        // getTypeType should have already taken care of the error message
        return expression;
    }
    if (!ltype->is<IR::Type_Bits>() || !rtype->is<IR::Type_Bits>()) {
        typeError("%1%: Concatenation not defined on %2% and %3%", expression, ltype->toString(),
                  rtype->toString());
        return expression;
    }
    auto bl = ltype->to<IR::Type_Bits>();
    auto br = rtype->to<IR::Type_Bits>();
    const IR::Type *resultType = IR::Type_Bits::get(bl->size + br->size, bl->isSigned);

    if (castLeft) {
        auto e = expression->clone();
        e->left = new IR::Cast(e->left->srcInfo, bl, e->left);
        if (isCompileTimeConstant(expression->left)) setCompileTimeConstant(e->left);
        setType(e->left, ltype);
        expression = e;
    }
    if (castRight) {
        auto e = expression->clone();
        e->right = new IR::Cast(e->right->srcInfo, br, e->right);
        if (isCompileTimeConstant(expression->right)) setCompileTimeConstant(e->right);
        setType(e->right, rtype);
        expression = e;
    }

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
const IR::Node *TypeInference::postorder(IR::Key *key) {
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
    setType(getOriginal(), keyTuple);
    return key;
}

/**
 *  typecheck a table initializer entry list
 */
const IR::Node *TypeInference::preorder(IR::EntriesList *el) {
    if (done()) return el;
    auto table = findContext<IR::P4Table>();
    BUG_CHECK(table != nullptr, "%1% entries not within a table", el);
    const IR::Key *key = table->getKey();
    if (key == nullptr) {
        if (el->size() != 0)
            typeError("Entries cannot be specified for a table with no key %1%", table);
        prune();
        return el;
    }
    auto keyTuple = typeMap->getType(key);  // direct typeMap call to skip checks
    if (keyTuple == nullptr) {
        // The keys have to be before the entries list.  If they are not,
        // at this point they have not yet been type-checked.
        if (key->srcInfo.isValid() && el->srcInfo.isValid() && key->srcInfo >= el->srcInfo) {
            typeError("%1%: Entries list must be after table key %2%", el, key);
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
const IR::Node *TypeInference::postorder(IR::Entry *entry) {
    if (done()) return entry;
    auto table = findContext<IR::P4Table>();
    if (table == nullptr) return entry;
    const IR::Key *key = table->getKey();
    if (key == nullptr) return entry;
    auto keyTuple = getType(key);
    if (keyTuple == nullptr) return entry;

    auto entryKeyType = getType(entry->keys);
    if (entryKeyType == nullptr) return entry;
    if (auto ts = entryKeyType->to<IR::Type_Set>()) entryKeyType = ts->elementType;
    if (entry->singleton) {
        if (auto tl = entryKeyType->to<IR::Type_BaseList>()) {
            // An entry of _ does not have type Tuple<Type_Dontcare>, but rather Type_Dontcare
            if (tl->getSize() == 1 && tl->components.at(0)->is<IR::Type_Dontcare>())
                entryKeyType = tl->components.at(0);
        }
    }

    auto keyset = entry->getKeys();
    if (keyset == nullptr || !(keyset->is<IR::ListExpression>())) {
        typeError("%1%: key expression must be tuple", keyset);
        return entry;
    }
    if (keyset->components.size() < key->keyElements.size()) {
        typeError("%1%: Size of entry keyset must match the table key set size", keyset);
        return entry;
    }

    bool nonConstantKeys = false;
    for (auto ke : keyset->components)
        if (!isCompileTimeConstant(ke)) {
            typeError("Key entry must be a compile time constant: %1%", ke);
            nonConstantKeys = true;
        }
    if (nonConstantKeys) return entry;

    if (entry->priority && !isCompileTimeConstant(entry->priority)) {
        typeError("Entry priority must be a compile time constant: %1%", entry->priority);
        return entry;
    }

    TypeVariableSubstitution *tvs =
        unifyCast(entry, keyTuple, entryKeyType,
                  "Table entry has type '%1%' which is not the expected type '%2%'",
                  {keyTuple, entryKeyType});
    if (tvs == nullptr) return entry;
    ConstantTypeSubstitution cts(tvs, typeMap, this);
    auto ks = cts.convert(keyset, getChildContext());
    if (::errorCount() > 0) return entry;

    if (ks != keyset)
        entry = new IR::Entry(entry->srcInfo, entry->annotations, entry->isConst, entry->priority,
                              ks->to<IR::ListExpression>(), entry->action, entry->singleton);

    auto actionRef = entry->getAction();
    auto ale = validateActionInitializer(actionRef);
    if (ale != nullptr) {
        auto anno = ale->getAnnotation(IR::Annotation::defaultOnlyAnnotation);
        if (anno != nullptr) {
            typeError("%1%: Action marked with %2% used in table", entry,
                      IR::Annotation::defaultOnlyAnnotation);
            return entry;
        }
    }
    return entry;
}

const IR::Node *TypeInference::postorder(IR::ListExpression *expression) {
    if (done()) return expression;
    bool constant = true;
    auto components = new IR::Vector<IR::Type>();
    for (auto c : expression->components) {
        if (!isCompileTimeConstant(c)) constant = false;
        auto type = getType(c);
        if (type == nullptr) return expression;
        components->push_back(type);
    }

    auto tupleType = new IR::Type_List(expression->srcInfo, *components);
    auto type = canonicalize(tupleType);
    if (type == nullptr) return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Invalid *expression) {
    if (done()) return expression;
    auto unk = IR::Type_Unknown::get();
    setType(expression, unk);
    setType(getOriginal(), unk);
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::InvalidHeader *expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->headerType);
    auto concreteType = type;
    if (auto ts = concreteType->to<IR::Type_SpecializedCanonical>()) concreteType = ts->substituted;
    if (!concreteType->is<IR::Type_Header>()) {
        typeError("%1%: invalid header expression has a non-header type `%2%`", expression, type);
        return expression;
    }
    setType(expression, type);
    setType(getOriginal(), type);
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::InvalidHeaderUnion *expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->headerUnionType);
    auto concreteType = type;
    if (auto ts = concreteType->to<IR::Type_SpecializedCanonical>()) concreteType = ts->substituted;
    if (!concreteType->is<IR::Type_HeaderUnion>()) {
        typeError("%1%: does not have a header_union type `%2%`", expression, type);
        return expression;
    }
    setType(expression, type);
    setType(getOriginal(), type);
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::P4ListExpression *expression) {
    if (done()) return expression;
    bool constant = true;
    auto elementType = getTypeType(expression->elementType);
    auto vec = new IR::Vector<IR::Expression>();
    bool changed = false;
    for (auto c : expression->components) {
        if (!isCompileTimeConstant(c)) constant = false;
        auto type = getType(c);
        if (type == nullptr) return expression;
        auto tvs = unify(expression, elementType, type,
                         "Vector element type '%1%' does not match expected type '%2%'",
                         {type, elementType});
        if (tvs == nullptr) return expression;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap, this);
            auto converted = cts.convert(c, getChildContext());
            vec->push_back(converted);
            changed = changed || converted != c;
        } else {
            vec->push_back(c);
        }
    }

    if (changed)
        expression = new IR::P4ListExpression(expression->srcInfo, *vec, elementType->getP4Type());
    auto type = new IR::Type_P4List(expression->srcInfo, elementType);
    setType(getOriginal(), type);
    setType(expression, type);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::HeaderStackExpression *expression) {
    if (done()) return expression;
    bool constant = true;
    auto stackType = getTypeType(expression->headerStackType);
    if (auto st = stackType->to<IR::Type_Stack>()) {
        auto elementType = st->elementType;
        auto vec = new IR::Vector<IR::Expression>();
        bool changed = false;
        if (expression->size() != st->getSize()) {
            typeError("%1%: number of initializers %2% has to match stack size %3%", expression,
                      expression->size(), st->getSize());
            return expression;
        }
        for (auto c : expression->components) {
            if (!isCompileTimeConstant(c)) constant = false;
            auto type = getType(c);
            if (type == nullptr) return expression;
            auto tvs = unify(expression, elementType, type,
                             "Stack element type '%1%' does not match expected type '%2%'",
                             {type, elementType});
            if (tvs == nullptr) return expression;
            if (!tvs->isIdentity()) {
                ConstantTypeSubstitution cts(tvs, typeMap, this);
                auto converted = cts.convert(c, getChildContext());
                vec->push_back(converted);
                changed = true;
            } else {
                vec->push_back(c);
            }
            if (changed)
                expression = new IR::HeaderStackExpression(expression->srcInfo, *vec, stackType);
        }
    } else {
        typeError("%1%: header stack expression has an incorrect type `%2%`", expression,
                  stackType);
        return expression;
    }

    setType(getOriginal(), stackType);
    setType(expression, stackType);
    if (constant) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::StructExpression *expression) {
    if (done()) return expression;
    bool constant = true;
    auto components = new IR::IndexedVector<IR::StructField>();
    for (auto c : expression->components) {
        if (!isCompileTimeConstant(c->expression)) constant = false;
        auto type = getType(c->expression);
        if (type == nullptr) return expression;
        components->push_back(new IR::StructField(c->name, type));
    }

    // This is the type inferred by looking at the fields.
    const IR::Type *structType =
        new IR::Type_UnknownStruct(expression->srcInfo, "unknown struct", *components);
    structType = canonicalize(structType);

    const IR::Expression *result = expression;
    if (expression->structType != nullptr) {
        // We know the exact type of the initializer
        auto desired = getTypeType(expression->structType);
        if (desired == nullptr) return expression;
        auto tvs = unify(expression, desired, structType,
                         "Initializer type '%1%' does not match expected type '%2%'",
                         {structType, desired});
        if (tvs == nullptr) return expression;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap, this);
            result = cts.convert(expression, getChildContext());
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

const IR::Node *TypeInference::postorder(IR::ArrayIndex *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;
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

    if ((index < 0) && !rtype->is<IR::Type_Bits>() && !rtype->is<IR::Type_SerEnum>() &&
        !rtype->is<IR::Type_InfInt>()) {
        typeError("Array index %1% must be an integer, but it has type %2%", expression->right,
                  rtype->toString());
        return expression;
    }

    const IR::Type *type = nullptr;
    if (hst) {
        if (checkArrays && hst->sizeKnown()) {
            int size = hst->getSize();
            if (index >= 0 && index >= size) {
                typeError("Array index %1% larger or equal to array size %2%", expression->right,
                          hst->size);
                return expression;
            }
        }
        type = hst->elementType;
    } else if (auto tup = ltype->to<IR::Type_BaseList>()) {
        if (index < 0) {
            typeError("Tuple index %1% must be constant", expression->right);
            return expression;
        }
        if (static_cast<size_t>(index) >= tup->getSize()) {
            typeError("Tuple index %1% larger than tuple size %2%", expression->right,
                      tup->getSize());
            return expression;
        }
        type = tup->components.at(index);
        if (isCompileTimeConstant(expression->left)) {
            setCompileTimeConstant(expression);
            setCompileTimeConstant(getOriginal<IR::Expression>());
        }
    } else {
        typeError("Indexing %1% applied to non-array and non-tuple type %2%", expression,
                  ltype->toString());
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

const IR::Node *TypeInference::binaryBool(const IR::Operation_Binary *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

    if (!ltype->is<IR::Type_Boolean>() || !rtype->is<IR::Type_Boolean>()) {
        typeError("%1%: not defined on %2% and %3%", expression, ltype->toString(),
                  rtype->toString());
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

const IR::Node *TypeInference::binaryArith(const IR::Operation_Binary *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;
    bool castLeft = false;
    bool castRight = false;

    if (auto se = ltype->to<IR::Type_SerEnum>()) {
        ltype = getTypeType(se->type);
        castLeft = true;
    }
    if (auto se = rtype->to<IR::Type_SerEnum>()) {
        rtype = getTypeType(se->type);
        castRight = true;
    }
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    const IR::Type_Bits *bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits *br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    } else if (ltype->is<IR::Type_InfInt>() && rtype->is<IR::Type_InfInt>()) {
        auto t = IR::Type_InfInt::get();
        auto result = constantFold(expression);
        setType(getOriginal(), t);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }

    const IR::Type *resultType = ltype;
    if (bl != nullptr && br != nullptr) {
        if (bl->size != br->size) {
            typeError("%1%: Cannot operate on values with different widths %2% and %3%", expression,
                      bl->size, br->size);
            return expression;
        }
        if (bl->isSigned != br->isSigned) {
            typeError("%1%: Cannot operate on values with different signs", expression);
            return expression;
        }
    }
    if ((bl == nullptr && br != nullptr) || castLeft) {
        // must insert cast on the left
        auto leftResultType = br;
        if (castLeft && !br) leftResultType = bl;
        auto e = expression->clone();
        e->left = new IR::Cast(e->left->srcInfo, leftResultType, e->left);
        setType(e->left, leftResultType);
        if (isCompileTimeConstant(expression->left)) {
            e->left = constantFold(e->left);
            setCompileTimeConstant(e->left);
        }
        expression = e;
        resultType = leftResultType;
    }
    if ((bl != nullptr && br == nullptr) || castRight) {
        auto e = expression->clone();
        auto rightResultType = bl;
        if (castRight && !bl) rightResultType = br;
        e->right = new IR::Cast(e->right->srcInfo, rightResultType, e->right);
        setType(e->right, rightResultType);
        if (isCompileTimeConstant(expression->right)) {
            e->right = constantFold(e->right);
            setCompileTimeConstant(e->right);
        }
        expression = e;
        resultType = rightResultType;
    }

    setType(getOriginal(), resultType);
    setType(expression, resultType);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node *TypeInference::unsBinaryArith(const IR::Operation_Binary *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

    if (auto se = ltype->to<IR::Type_SerEnum>()) ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>()) rtype = getTypeType(se->type);

    const IR::Type_Bits *bl = ltype->to<IR::Type_Bits>();
    if (bl != nullptr && bl->isSigned) {
        typeError("%1%: Cannot operate on signed values", expression);
        return expression;
    }
    const IR::Type_Bits *br = rtype->to<IR::Type_Bits>();
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

const IR::Node *TypeInference::shift(const IR::Operation_Binary *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

    if (auto se = ltype->to<IR::Type_SerEnum>()) ltype = getTypeType(se->type);
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
            warn(ErrorType::WARN_OVERFLOW, "%1%: shifting value with %2% bits by %3%", expression,
                 lt->size, shift);
        // If the amount is signed but positive, make it unsigned
        if (auto bt = rtype->to<IR::Type_Bits>()) {
            if (bt->isSigned) {
                rtype = IR::Type_Bits::get(rtype->srcInfo, bt->width_bits(), false);
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
        typeError("%1%: Shift amount must be an unsigned number", expression->right);
        return expression;
    }

    if (!lt && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1% left operand of shift must be a numeric type, not %2%", expression,
                  ltype->toString());
        return expression;
    }

    if (ltype->is<IR::Type_InfInt>() && !rtype->is<IR::Type_InfInt>() &&
        !isCompileTimeConstant(expression->right)) {
        typeError(
            "%1%: shift result type is arbitrary-precision int, but right operand is not constant; "
            "width of left operand of shift needs to be specified or both operands need to be "
            "constant",
            expression);
        return expression;
    }

    setType(expression, ltype);
    setType(getOriginal(), ltype);
    if (isCompileTimeConstant(expression->left) && isCompileTimeConstant(expression->right)) {
        auto result = constantFold(expression);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

// Handle .. and &&&
const IR::Node *TypeInference::typeSet(const IR::Operation_Binary *expression) {
    if (done()) return expression;
    auto ltype = getType(expression->left);
    auto rtype = getType(expression->right);
    if (ltype == nullptr || rtype == nullptr) return expression;

    auto leftType = ltype;  // save original type
    if (auto se = ltype->to<IR::Type_SerEnum>()) ltype = getTypeType(se->type);
    if (auto se = rtype->to<IR::Type_SerEnum>()) rtype = getTypeType(se->type);
    BUG_CHECK(ltype && rtype, "Invalid Type_SerEnum/getTypeType");

    // The following section is very similar to "binaryArith()" above
    const IR::Type_Bits *bl = ltype->to<IR::Type_Bits>();
    const IR::Type_Bits *br = rtype->to<IR::Type_Bits>();
    if (bl == nullptr && !ltype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->left, ltype->toString());
        return expression;
    } else if (br == nullptr && !rtype->is<IR::Type_InfInt>()) {
        typeError("%1%: cannot be applied to expression '%2%' with type '%3%'",
                  expression->getStringOp(), expression->right, rtype->toString());
        return expression;
    }

    const IR::Type *sameType = leftType;
    if (bl != nullptr && br != nullptr) {
        if (!typeMap->equivalent(bl, br)) {
            typeError("%1%: Cannot operate on values with different types %2% and %3%", expression,
                      bl->toString(), br->toString());
            return expression;
        }
    } else if (bl == nullptr && br != nullptr) {
        auto e = expression->clone();
        e->left = new IR::Cast(e->left->srcInfo, rtype, e->left);
        setCompileTimeConstant(e->left);
        expression = e;
        sameType = rtype;
        setType(e->left, sameType);
    } else if (bl != nullptr && br == nullptr) {
        auto e = expression->clone();
        e->right = new IR::Cast(e->right->srcInfo, ltype, e->right);
        setCompileTimeConstant(e->right);
        expression = e;
        setType(e->right, ltype);
        sameType = leftType;  // Not ltype: SerEnum &&& Bit is Set<SerEnum>
    } else {
        // both are InfInt: use same exact type for both sides, so it is properly
        // set after unification
        // FIXME -- the below is obviously wrong and just serves to tweak when precisely
        // the type will be inferred -- papering over bugs elsewhere in typechecking,
        // avoiding the BUG_CHECK(!readOnly... in end_apply/apply_visitor above.
        // (maybe just need learner->readOnly = false in TypeInference::learn above?)
        auto r = expression->right->clone();
        auto e = expression->clone();
        if (isCompileTimeConstant(expression->right)) setCompileTimeConstant(r);
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

const IR::Node *TypeInference::postorder(IR::LNot *expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr) return expression;
    if (!(*type == *IR::Type_Boolean::get())) {
        typeError("Cannot apply %1% to value %2% of type %3%", expression->getStringOp(),
                  expression->expr, type->toString());
    } else {
        setType(expression, IR::Type_Boolean::get());
        setType(getOriginal(), IR::Type_Boolean::get());
    }
    if (isCompileTimeConstant(expression->expr)) {
        auto result = constantFold(expression);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Neg *expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr) return expression;

    if (auto se = type->to<IR::Type_SerEnum>()) type = getTypeType(se->type);
    BUG_CHECK(type, "Invalid Type_SerEnum/getTypeType");

    if (type->is<IR::Type_InfInt>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else if (type->is<IR::Type_Bits>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else {
        typeError("Cannot apply %1% to value %2% of type %3%", expression->getStringOp(),
                  expression->expr, type->toString());
    }
    if (isCompileTimeConstant(expression->expr)) {
        auto result = constantFold(expression);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::UPlus *expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr) return expression;

    if (auto se = type->to<IR::Type_SerEnum>()) type = getTypeType(se->type);
    BUG_CHECK(type, "Invalid Type_SerEnum/getTypeType");

    if (type->is<IR::Type_InfInt>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else if (type->is<IR::Type_Bits>()) {
        setType(getOriginal(), type);
        setType(expression, type);
    } else {
        typeError("Cannot apply %1% to value %2% of type %3%", expression->getStringOp(),
                  expression->expr, type->toString());
    }
    if (isCompileTimeConstant(expression->expr)) {
        auto result = constantFold(expression);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Cmpl *expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr) return expression;

    if (auto se = type->to<IR::Type_SerEnum>()) type = getTypeType(se->type);
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
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Cast *expression) {
    if (done()) return expression;
    const IR::Type *sourceType = getType(expression->expr);
    const IR::Type *castType = getTypeType(expression->destType);
    if (sourceType == nullptr || castType == nullptr) return expression;

    auto concreteType = castType;
    if (auto tsc = castType->to<IR::Type_SpecializedCanonical>()) concreteType = tsc->substituted;
    if (auto st = concreteType->to<IR::Type_StructLike>()) {
        if (auto se = expression->expr->to<IR::StructExpression>()) {
            // Interpret (S) { kvpairs } as a struct initializer expression
            // instead of a cast to a struct.
            if (se->type == nullptr || se->type->is<IR::Type_Unknown>() ||
                se->type->is<IR::Type_UnknownStruct>()) {
                auto type = castType->getP4Type();
                setType(type, new IR::Type_Type(st));
                auto sie = new IR::StructExpression(se->srcInfo, type, se->components);
                auto result = postorder(sie);  // may insert casts
                setType(result, st);
                if (isCompileTimeConstant(se)) {
                    setCompileTimeConstant(result->to<IR::Expression>());
                    setCompileTimeConstant(getOriginal<IR::Expression>());
                }
                return result;
            } else {
                typeError("%1%: cast not supported", expression->destType);
                return expression;
            }
        } else if (expression->expr->is<IR::ListExpression>()) {
            auto result = assignment(expression, st, expression->expr);
            return result;
        } else if (auto ih = expression->expr->to<IR::Invalid>()) {
            auto type = castType->getP4Type();
            auto concreteCastType = castType;
            if (auto ts = castType->to<IR::Type_SpecializedCanonical>())
                concreteCastType = ts->substituted;
            if (concreteCastType->is<IR::Type_Header>()) {
                setType(type, new IR::Type_Type(castType));
                auto result = new IR::InvalidHeader(ih->srcInfo, type, type);
                setType(result, castType);
                return result;
            } else if (concreteCastType->is<IR::Type_HeaderUnion>()) {
                setType(type, new IR::Type_Type(castType));
                auto result = new IR::InvalidHeaderUnion(ih->srcInfo, type, type);
                setType(result, castType);
                return result;
            } else {
                typeError("%1%: 'invalid' expression type `%2%` must be a header or header union",
                          expression, castType);
                return expression;
            }
        }
    }
    if (auto lt = concreteType->to<IR::Type_P4List>()) {
        auto listElementType = lt->elementType;
        if (auto le = expression->expr->to<IR::ListExpression>()) {
            IR::Vector<IR::Expression> vec;
            bool isConstant = true;
            for (size_t i = 0; i < le->size(); i++) {
                auto compI = le->components.at(i);
                auto src = assignment(expression, listElementType, compI);
                if (!isCompileTimeConstant(src)) isConstant = false;
                vec.push_back(src);
            }
            auto vecType = castType->getP4Type();
            setType(vecType, new IR::Type_Type(lt));
            auto result = new IR::P4ListExpression(le->srcInfo, vec, listElementType->getP4Type());
            setType(result, lt);
            if (isConstant) {
                setCompileTimeConstant(result);
                setCompileTimeConstant(getOriginal<IR::Expression>());
            }
            return result;
        } else {
            typeError("%1%: casts to list not supported", expression);
            return expression;
        }
    }
    if (concreteType->is<IR::Type_Stack>()) {
        if (expression->expr->is<IR::ListExpression>()) {
            auto result = assignment(expression, concreteType, expression->expr);
            return result;
        } else {
            typeError("%1%: casts to header stack not supported", expression);
            return expression;
        }
    }

    if (!castType->is<IR::Type_Bits>() && !castType->is<IR::Type_Boolean>() &&
        !castType->is<IR::Type_Newtype>() && !castType->is<IR::Type_SerEnum>() &&
        !castType->is<IR::Type_InfInt>() && !castType->is<IR::Type_SpecializedCanonical>()) {
        typeError("%1%: cast not supported", expression->destType);
        return expression;
    }

    if (!canCastBetween(castType, sourceType)) {
        // This cast is not legal directly, but let's try to see whether
        // performing a substitution can help.  This will allow the use
        // of constants on the RHS.
        const IR::Type *destType = castType;
        while (destType->is<IR::Type_Newtype>())
            destType = getTypeType(destType->to<IR::Type_Newtype>()->type);

        auto tvs = unify(expression, destType, sourceType, "Cannot cast from '%1%' to '%2%'",
                         {sourceType, castType});
        if (tvs == nullptr) return expression;
        const IR::Expression *rhs = expression->expr;
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap, this);
            rhs = cts.convert(expression->expr, getChildContext());  // sets type
        }
        if (rhs != expression->expr) {
            // if we are here we have performed a substitution on the rhs
            expression = new IR::Cast(expression->srcInfo, expression->destType, rhs);
            sourceType = getTypeType(expression->destType);
        }
        if (!canCastBetween(castType, sourceType))
            typeError("%1%: Illegal cast from %2% to %3%", expression, sourceType->toString(),
                      castType->toString());
    }
    setType(expression, castType);
    setType(getOriginal(), castType);
    if (isCompileTimeConstant(expression->expr)) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::PathExpression *expression) {
    if (done()) return expression;
    auto decl = getDeclaration(expression->path, !errorOnNullDecls);
    if (errorOnNullDecls && decl == nullptr) {
        typeError("%1%: Cannot resolve declaration", expression);
        return expression;
    }
    const IR::Type *type = nullptr;
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
    if (decl->is<IR::Declaration_ID>() && declType &&
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
    } else if (decl->is<IR::Declaration_Constant>() || decl->is<IR::Declaration_Instance>()) {
        setCompileTimeConstant(expression);
        setCompileTimeConstant(getOriginal<IR::Expression>());
    } else if (decl->is<IR::Method>() || decl->is<IR::Function>()) {
        type = getType(decl->getNode());
        // Each method invocation uses fresh type variables
        if (type != nullptr)
            // may be nullptr because typechecking may have failed
            type = cloneWithFreshTypeVariables(type->to<IR::Type_MethodBase>());
    } else if (decl->is<IR::Type_Declaration>()) {
        typeError("%1%: Type cannot be used here, expecting an expression.", expression);
        return expression;
    }

    if (type == nullptr) {
        type = getType(decl->getNode());
        if (type == nullptr) return expression;
    }

    setType(getOriginal(), type);
    setType(expression, type);
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Slice *expression) {
    if (done()) return expression;
    const IR::Type *type = getType(expression->e0);
    if (type == nullptr) return expression;

    if (auto se = type->to<IR::Type_SerEnum>()) type = getTypeType(se->type);

    if (!type->is<IR::Type_Bits>()) {
        typeError("%1%: bit extraction only defined for bit<> types", expression);
        return expression;
    }

    auto e1type = getType(expression->e1);
    if (e1type && e1type->is<IR::Type_SerEnum>()) {
        auto ei = EnumInstance::resolve(expression->e1, typeMap);
        CHECK_NULL(ei);
        auto sei = ei->to<SerEnumInstance>();
        if (sei == nullptr) {
            typeError("%1%: slice bit index values must be constants", expression->e1);
            return expression;
        }
        expression->e1 = sei->value;
    }
    auto e2type = getType(expression->e2);
    if (e2type && e2type->is<IR::Type_SerEnum>()) {
        auto ei = EnumInstance::resolve(expression->e2, typeMap);
        CHECK_NULL(ei);
        auto sei = ei->to<SerEnumInstance>();
        if (sei == nullptr) {
            typeError("%1%: slice bit index values must be constants", expression->e2);
            return expression;
        }
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

    auto msb = expression->e1->checkedTo<IR::Constant>();
    auto lsb = expression->e2->checkedTo<IR::Constant>();
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

    const IR::Type *resultType = IR::Type_Bits::get(bst->srcInfo, m - l + 1, false);
    resultType = canonicalize(resultType);
    if (resultType == nullptr) return expression;
    setType(getOriginal(), resultType);
    setType(expression, resultType);
    if (isLeftValue(expression->e0)) {
        setLeftValue(expression);
        setLeftValue(getOriginal<IR::Expression>());
    }
    if (isCompileTimeConstant(expression->e0)) {
        auto result = constantFold(expression);
        setCompileTimeConstant(result);
        setCompileTimeConstant(getOriginal<IR::Expression>());
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Dots *expression) {
    if (done()) return expression;
    setType(expression, IR::Type_Any::get());
    setType(getOriginal(), IR::Type_Any::get());
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Mux *expression) {
    if (done()) return expression;
    const IR::Type *firstType = getType(expression->e0);
    const IR::Type *secondType = getType(expression->e1);
    const IR::Type *thirdType = getType(expression->e2);
    if (firstType == nullptr || secondType == nullptr || thirdType == nullptr) return expression;

    if (!firstType->is<IR::Type_Boolean>()) {
        typeError("Selector of %1% must be bool, not %2%", expression->getStringOp(),
                  firstType->toString());
        return expression;
    }

    if (secondType->is<IR::Type_InfInt>() && thirdType->is<IR::Type_InfInt>()) {
        typeError("Width must be specified for at least one of %1% or %2%", expression->e1,
                  expression->e2);
        return expression;
    }
    auto tvs = unify(expression, secondType, thirdType,
                     "The expressions in a ?: conditional have different types '%1%' and '%2%'",
                     {secondType, thirdType});
    if (tvs != nullptr) {
        if (!tvs->isIdentity()) {
            ConstantTypeSubstitution cts(tvs, typeMap, this);
            auto e1 = cts.convert(expression->e1, getChildContext());
            auto e2 = cts.convert(expression->e2, getChildContext());
            if (::errorCount() > 0) return expression;
            expression->e1 = e1;
            expression->e2 = e2;
            secondType = typeMap->getType(e1);
        }
        setType(expression, secondType);
        setType(getOriginal(), secondType);
        if (isCompileTimeConstant(expression->e0) && isCompileTimeConstant(expression->e1) &&
            isCompileTimeConstant(expression->e2)) {
            auto result = constantFold(expression);
            setCompileTimeConstant(result);
            setCompileTimeConstant(getOriginal<IR::Expression>());
            return result;
        }
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::TypeNameExpression *expression) {
    if (done()) return expression;
    auto type = getType(expression->typeName);
    if (type == nullptr) return expression;
    setType(getOriginal(), type);
    setType(expression, type);
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::Member *expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr) return expression;

    cstring member = expression->member.name;
    if (auto ts = type->to<IR::Type_SpecializedCanonical>()) type = ts->substituted;

    if (auto *ext = type->to<IR::Type_Extern>()) {
        auto call = findContext<IR::MethodCallExpression>();
        if (call == nullptr) {
            typeError("%1%: Methods can only be called", expression);
            return expression;
        }
        auto method = ext->lookupMethod(expression->member, call->arguments);
        if (method == nullptr) {
            typeError("%1%: extern %2% does not have method matching this call", expression,
                      ext->name);
            return expression;
        }

        const IR::Type *methodType = getType(method);
        if (methodType == nullptr) return expression;
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
    if (inMethod && (member == IR::Type::minSizeInBits || member == IR::Type::minSizeInBytes ||
                     member == IR::Type::maxSizeInBits || member == IR::Type::maxSizeInBytes)) {
        auto type = new IR::Type_Method(IR::Type_InfInt::get(), new IR::ParameterList(), member);
        auto ctype = canonicalize(type);
        if (ctype == nullptr) return expression;
        setType(getOriginal(), ctype);
        setType(expression, ctype);
        return expression;
    }

    if (type->is<IR::Type_StructLike>()) {
        std::string typeStr = "structure ";
        if (type->is<IR::Type_Header>() || type->is<IR::Type_HeaderUnion>()) {
            typeStr = "";
            if (inMethod && (member == IR::Type_Header::isValid)) {
                // Built-in method
                auto type =
                    new IR::Type_Method(IR::Type_Boolean::get(), new IR::ParameterList(), member);
                auto ctype = canonicalize(type);
                if (ctype == nullptr) return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }
        if (type->is<IR::Type_Header>()) {
            if (inMethod &&
                (member == IR::Type_Header::setValid || member == IR::Type_Header::setInvalid)) {
                if (!isLeftValue(expression->expr))
                    typeError("%1%: must be applied to a left-value", expression);
                // Built-in method
                auto type =
                    new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList, member);
                auto ctype = canonicalize(type);
                if (ctype == nullptr) return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                return expression;
            }
        }

        auto stb = type->to<IR::Type_StructLike>();
        auto field = stb->getField(member);
        if (field == nullptr) {
            typeError("Field %1% is not a member of %2%%3%", expression->member, typeStr, stb);
            return expression;
        }

        auto fieldType = getTypeType(field->type);
        if (fieldType == nullptr) return expression;
        if (fieldType->is<IR::Type_ActionEnum>() && !getParent<IR::SwitchStatement>()) {
            typeError("%1%: only allowed in switch statements", expression);
            return expression;
        }
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

    if (auto *apply = type->to<IR::IApply>(); apply && member == IR::IApply::applyMethodName) {
        auto *methodType = apply->getApplyMethodType();
        auto *canon = canonicalize(methodType);
        if (!canon) return expression;
        methodType = canon->to<IR::Type_Method>();
        if (methodType == nullptr) return expression;
        learn(methodType, this, getChildContext());
        setType(getOriginal(), methodType);
        setType(expression, methodType);
        return expression;
    }

    if (auto *stack = type->to<IR::Type_Stack>()) {
        auto parser = findContext<IR::P4Parser>();
        if (member == IR::Type_Stack::next || member == IR::Type_Stack::last) {
            if (parser == nullptr) {
                typeError("%1%: 'last', and 'next' for stacks can only be used in a parser",
                          expression);
                return expression;
            }
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
            if (parser == nullptr) {
                typeError("%1%: 'lastIndex' for stacks can only be used in a parser", expression);
                return expression;
            }
            setType(getOriginal(), IR::Type_Bits::get(32, false));
            setType(expression, IR::Type_Bits::get(32, false));
            return expression;
        } else if (member == IR::Type_Stack::push_front || member == IR::Type_Stack::pop_front) {
            if (parser != nullptr)
                typeError("%1%: '%2%' and '%3%' for stacks cannot be used in a parser", expression,
                          IR::Type_Stack::push_front, IR::Type_Stack::pop_front);
            if (!isLeftValue(expression->expr))
                typeError("%1%: must be applied to a left-value", expression);
            auto params = new IR::IndexedVector<IR::Parameter>();
            auto param = new IR::Parameter(IR::ID("count"_cs, nullptr), IR::Direction::None,
                                           IR::Type_InfInt::get());
            auto tt = new IR::Type_Type(param->type);
            setType(param->type, tt);
            setType(param, param->type);
            params->push_back(param);
            auto type =
                new IR::Type_Method(IR::Type_Void::get(), new IR::ParameterList(*params), member);
            auto canon = canonicalize(type);
            if (canon == nullptr) return expression;
            setType(getOriginal(), canon);
            setType(expression, canon);
            return expression;
        }
    }

    if (auto *tt = type->to<IR::Type_Type>()) {
        auto base = tt->type;
        if (base->is<IR::Type_Error>() || base->is<IR::Type_Enum>() ||
            base->is<IR::Type_SerEnum>()) {
            if (isCompileTimeConstant(expression->expr)) {
                setCompileTimeConstant(expression);
                setCompileTimeConstant(getOriginal<IR::Expression>());
            }
            auto fbase = base->to<IR::ISimpleNamespace>();
            if (auto decl = fbase->getDeclByName(member)) {
                if (auto ftype = getType(decl->getNode())) {
                    setType(getOriginal(), ftype);
                    setType(expression, ftype);
                }
            } else {
                typeError("%1%: Invalid enum tag", expression);
                setType(getOriginal(), type);
                setType(expression, type);
            }
            return expression;
        }
    }

    typeError("Cannot extract field %1% from %2% which has type %3%", expression->member,
              expression->expr, type->toString());
    // unreachable
    return expression;
}

// If inActionList this call is made in the "action" property of a table
const IR::Expression *TypeInference::actionCall(bool inActionList,
                                                const IR::MethodCallExpression *actionCall) {
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
    if (!methodType) return actionCall;  // error emitted in getType
    auto baseType = methodType->to<IR::Type_Action>();
    if (!baseType) {
        typeError("%1%: must be an action", method);
        return actionCall;
    }
    LOG2("Action type " << baseType);
    BUG_CHECK(method->is<IR::PathExpression>(), "%1%: unexpected call", method);
    BUG_CHECK(baseType->returnType == nullptr, "%1%: action with return type?",
              baseType->returnType);
    if (!baseType->typeParameters->empty()) {
        typeError("%1%: Actions cannot be generic", baseType->typeParameters);
        return actionCall;
    }
    if (!actionCall->typeArguments->empty()) {
        typeError("%1%: Cannot supply type parameters for an action invocation",
                  actionCall->typeArguments);
        return actionCall;
    }

    bool inTable = findContext<IR::P4Table>() != nullptr;

    TypeConstraints constraints(typeMap->getSubstitutions(), typeMap);
    auto params = new IR::ParameterList;

    // keep track of parameters that have not been matched yet
    absl::flat_hash_map<cstring, const IR::Parameter *, Util::Hash> left;
    for (auto p : baseType->parameters->parameters) left.emplace(p->name, p);

    auto paramIt = baseType->parameters->parameters.begin();
    auto newArgs = new IR::Vector<IR::Argument>();
    bool changed = false;
    for (auto arg : *actionCall->arguments) {
        cstring argName = arg->name.name;
        bool named = !argName.isNullOrEmpty();
        const IR::Parameter *param;
        auto newExpr = arg->expression;

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
        if (!left.erase(param->name)) {
            // This should have been checked by the CheckNamedArgs pass.
            BUG("%1%: Duplicate argument name?", param->name);
        }

        auto paramType = getType(param);
        auto argType = getType(arg);
        if (paramType == nullptr || argType == nullptr)
            // type checking failed before
            return actionCall;
        constraints.addImplicitCastConstraint(actionCall, paramType, argType);
        if (param->direction == IR::Direction::None) {
            if (inActionList) {
                typeError("%1%: parameter %2% cannot be bound: it is set by the control plane", arg,
                          param);
            } else if (inTable) {
                // For actions None parameters are treated as IN
                // parameters when the action is called directly.  We
                // don't require them to be bound to a compile-time
                // constant.  But if the action is instantiated in a
                // table (as default_action or entries), then the
                // arguments do have to be compile-time constants.
                if (!isCompileTimeConstant(arg->expression))
                    typeError("%1%: action argument must be a compile-time constant",
                              arg->expression);
            }
            // This is like an assignment; may make additional conversions.
            newExpr = assignment(arg, param->type, arg->expression);
            if (readOnly) {
                // FIXME -- if we're in readonly mode, we should not have introduced any mods
                // here, but there's a bug in the DPDK backend where it generates a ListExpression
                // that would be converted to a StructExpression, and other problems where it
                // can't deal with that StructExpressions, so we hack to avoid breaking those tests
                newExpr = arg->expression;
            }
        } else if (param->direction == IR::Direction::Out ||
                   param->direction == IR::Direction::InOut) {
            if (!isLeftValue(arg->expression))
                typeError("%1%: must be a left-value", arg->expression);
        } else {
            // This is like an assignment; may make additional conversions.
            newExpr = assignment(arg, param->type, arg->expression);
        }
        if (::errorCount() > 0) return actionCall;
        if (newExpr != arg->expression) {
            LOG2("Changing action argument to " << newExpr);
            changed = true;
            newArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, newExpr));
        } else {
            newArgs->push_back(arg);
        }
        if (!named) ++paramIt;
    }
    if (changed)
        actionCall =
            new IR::MethodCallExpression(actionCall->srcInfo, actionCall->type, actionCall->method,
                                         actionCall->typeArguments, newArgs);

    // Check remaining parameters: they must be all non-directional
    bool error = false;
    for (auto p : left) {
        if (p.second->direction != IR::Direction::None && p.second->defaultValue == nullptr) {
            typeError("%1%: Parameter %2% must be bound", actionCall, p.second);
            error = true;
        }
    }
    if (error) return actionCall;

    auto resultType = new IR::Type_Action(baseType->srcInfo, baseType->typeParameters, params);

    setType(getOriginal(), resultType);
    setType(actionCall, resultType);
    auto tvs = constraints.solve();
    if (tvs == nullptr || errorCount() > 0) return actionCall;
    addSubstitutions(tvs);

    ConstantTypeSubstitution cts(tvs, typeMap, this);
    actionCall = cts.convert(actionCall, getChildContext())
                     ->to<IR::MethodCallExpression>();  // cast arguments
    if (::errorCount() > 0) return actionCall;

    LOG2("Converted action " << actionCall);
    setType(actionCall, resultType);
    return actionCall;
}

const IR::Node *TypeInference::postorder(IR::MethodCallStatement *mcs) {
    // Remove mcs if child methodCall resolves to a compile-time constant.
    return !mcs->methodCall ? nullptr : mcs;
}

const IR::Node *TypeInference::postorder(IR::MethodCallExpression *expression) {
    if (done()) return expression;
    LOG2("Solving method call " << dbp(expression));
    auto methodType = getType(expression->method);
    if (methodType == nullptr) return expression;
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
                !type->is<IR::Type_Extern>() && expression->typeArguments->size() == 0 &&
                expression->arguments->size() == 0) {
                auto max = mem->member.name.startsWith("max");
                int w = typeMap->widthBits(type, expression, max);
                LOG3("Folding " << mem << " to " << w);
                if (w < 0) return expression;
                if (mem->member.name.endsWith("Bytes")) w = ROUNDUP(w, 8);
                if (getParent<IR::MethodCallStatement>()) return nullptr;
                auto result = new IR::Constant(expression->srcInfo, w);
                auto tt = new IR::Type_Type(result->type);
                setType(result->type, tt);
                setType(result, result->type);
                setCompileTimeConstant(result);
                return result;
            }
            if (mem->member == IR::Type_Header::isValid && type->is<IR::Type_Header>()) {
                const IR::BoolLiteral *lit = nullptr;
                if (mem->expr->is<IR::InvalidHeader>())
                    lit = new IR::BoolLiteral(expression->srcInfo, false);
                if (mem->expr->is<IR::InvalidHeaderUnion>())
                    lit = new IR::BoolLiteral(expression->srcInfo, false);
                if (mem->expr->is<IR::StructExpression>())
                    lit = new IR::BoolLiteral(expression->srcInfo, true);
                if (lit) {
                    LOG3("Folding " << mem << " to " << lit);
                    if (getParent<IR::MethodCallStatement>()) return nullptr;
                    setType(lit, IR::Type_Boolean::get());
                    setCompileTimeConstant(lit);
                    return lit;
                }
            }
        }

        if (getContext()->node->is<IR::ActionListElement>()) {
            typeError("%1% is not invoking an action", expression);
            return expression;
        }

        // We build a type for the callExpression and unify it with the method expression
        // Allocate a fresh variable for the return type; it will be hopefully bound in the process.
        auto rettype = new IR::Type_Var(IR::ID(nameGen->newName("R"), "<returned type>"_cs));
        auto args = new IR::Vector<IR::ArgumentInfo>();
        bool constArgs = true;
        for (auto aarg : *expression->arguments) {
            auto arg = aarg->expression;
            auto argType = getType(arg);
            if (argType == nullptr) return expression;
            auto argInfo = new IR::ArgumentInfo(arg->srcInfo, isLeftValue(arg),
                                                isCompileTimeConstant(arg), argType, aarg);
            args->push_back(argInfo);
            constArgs = constArgs && isCompileTimeConstant(arg);
        }
        auto typeArgs = new IR::Vector<IR::Type>();
        for (auto ta : *expression->typeArguments) {
            auto taType = getTypeType(ta);
            if (taType == nullptr) return expression;
            typeArgs->push_back(taType);
        }
        auto callType = new IR::Type_MethodCall(expression->srcInfo, typeArgs, rettype, args);

        auto tvs = unify(expression, methodBaseType, callType,
                         "Function type '%1%' does not match invocation type '%2%'",
                         {methodBaseType, callType});
        if (tvs == nullptr) return expression;

        // Infer Dont_Care for type vars used only in not-present optional params
        auto dontCares = new TypeVariableSubstitution();
        auto typeParams = methodBaseType->typeParameters;
        for (auto p : *methodBaseType->parameters) {
            if (!p->isOptional()) continue;
            forAllMatching<IR::Type_Var>(
                p, [tvs, dontCares, typeParams, this](const IR::Type_Var *tv) {
                    if (typeMap->getSubstitutions()->lookup(tv) != nullptr)
                        return;                                             // already bound
                    if (tvs->lookup(tv)) return;                            // already bound
                    if (typeParams->getDeclByName(tv->name) != tv) return;  // not a tv of this call
                    dontCares->setBinding(tv, IR::Type_Dontcare::get());
                });
        }
        addSubstitutions(dontCares);

        LOG2("Method type before specialization " << methodType << " with " << tvs);
        TypeVariableSubstitutionVisitor substVisitor(tvs);
        substVisitor.setCalledBy(this);
        auto specMethodType = methodType->apply(substVisitor);
        LOG2("Method type after specialization " << specMethodType);
        learn(specMethodType, this, getChildContext());

        auto canon = getType(specMethodType);
        if (canon == nullptr) return expression;

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
        learn(returnType, this, getChildContext());
        if (returnType->is<IR::Type_Control>() || returnType->is<IR::Type_Parser>() ||
            returnType->is<IR::P4Parser>() || returnType->is<IR::P4Control>() ||
            returnType->is<IR::Type_Package>() ||
            (returnType->is<IR::Type_Extern>() && !constArgs)) {
            // Experimental: methods with all constant arguments can return an extern
            // instance as a factory method evaluated at compile time.
            typeError("%1%: illegal return type %2%", expression, returnType);
            return expression;
        }

        setType(getOriginal(), returnType);
        setType(expression, returnType);

        ConstantTypeSubstitution cts(tvs, typeMap, this);
        auto result = expression;
        // Arguments may need to be cast, e.g., list expression to a
        // header type.
        auto paramIt = functionType->parameters->begin();
        auto newArgs = new IR::Vector<IR::Argument>();
        bool changed = false;
        for (auto arg : *expression->arguments) {
            cstring argName = arg->name.name;
            bool named = !argName.isNullOrEmpty();
            const IR::Parameter *param;

            if (named) {
                param = functionType->parameters->getParameter(argName);
            } else {
                param = *paramIt;
            }

            if (param->type->is<IR::Type_Dontcare>())
                typeError(
                    "%1%: Could not infer a type for parameter %2% "
                    "(inferred type is don't care '_')",
                    arg, param);

            // By calling generic functions with don't care parameters
            // we can force parameters to have illegal types.  Check here for this case.
            // e.g., void f<T>(in T arg); table t { }; f(t);
            if (param->type->is<IR::Type_Table>() || param->type->is<IR::Type_Action>() ||
                param->type->is<IR::Type_Control>() || param->type->is<IR::Type_Package>() ||
                param->type->is<IR::Type_Parser>())
                typeError("%1%: argument cannot have type %2%", arg, param->type);

            auto newExpr = arg->expression;
            if (param->direction == IR::Direction::In) {
                // This is like an assignment; may make additional conversions.
                newExpr = assignment(arg, param->type, arg->expression);
            } else {
                // Insert casts for 'int' values.
                newExpr = cts.convert(newExpr, getChildContext())->to<IR::Expression>();
            }
            if (::errorCount() > 0) return expression;
            if (newExpr != arg->expression) {
                LOG2("Changing method argument to " << newExpr);
                changed = true;
                newArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, newExpr));
            } else {
                newArgs->push_back(arg);
            }
            if (!named) ++paramIt;
        }

        if (changed)
            result = new IR::MethodCallExpression(result->srcInfo, result->type, result->method,
                                                  result->typeArguments, newArgs);
        setType(result, returnType);

        auto mi = MethodInstance::resolve(result, this, typeMap, getChildContext(), true);
        if (mi->isApply() && findContext<IR::P4Action>()) {
            typeError("%1%: apply cannot be called from actions", expression);
            return expression;
        }

        if (const auto *ef = mi->to<ExternFunction>()) {
            const IR::Type *baseReturnType = returnType;
            if (const auto *sc = returnType->to<IR::Type_SpecializedCanonical>())
                baseReturnType = sc->baseType;
            const bool factoryOrStaticAssert =
                baseReturnType->is<IR::Type_Extern>() || ef->method->name == "static_assert";
            if (constArgs && factoryOrStaticAssert) {
                // factory extern function calls (those that return extern objects) with constant
                // args are compile-time constants.
                // The result of a static_assert call is also a compile-time constant.
                setCompileTimeConstant(expression);
                setCompileTimeConstant(getOriginal<IR::Expression>());
            }
        }

        auto bi = mi->to<BuiltInMethod>();
        if ((findContext<IR::SelectCase>()) && (!bi || (bi->name == IR::Type_Stack::pop_front ||
                                                        bi->name == IR::Type_Stack::push_front))) {
            typeError("%1%: no function calls allowed in this context", expression);
            return expression;
        }
        return result;
    }
    return expression;
}

const IR::Node *TypeInference::postorder(IR::ConstructorCallExpression *expression) {
    if (done()) return expression;
    auto type = getTypeType(expression->constructedType);
    if (type == nullptr) return expression;

    auto simpleType = type;
    CHECK_NULL(simpleType);
    if (auto *sc = type->to<IR::Type_SpecializedCanonical>()) simpleType = sc->substituted;

    if (auto *e = simpleType->to<IR::Type_Extern>()) {
        auto [contType, newArgs] = checkExternConstructor(expression, e, expression->arguments);
        if (newArgs == nullptr) return expression;
        expression->arguments = newArgs;
        setType(getOriginal(), contType);
        setType(expression, contType);
    } else if (auto *c = simpleType->to<IR::IContainer>()) {
        auto typeAndArgs = containerInstantiation(expression, expression->arguments, c);
        auto contType = typeAndArgs.first;
        auto args = typeAndArgs.second;
        if (contType == nullptr || args == nullptr) return expression;
        if (auto *st = type->to<IR::Type_SpecializedCanonical>()) {
            contType = new IR::Type_SpecializedCanonical(type->srcInfo, st->baseType, st->arguments,
                                                         contType);
        }
        expression->arguments = args;
        setType(expression, contType);
        setType(getOriginal(), contType);
    } else {
        typeError("%1%: Cannot invoke a constructor on type %2%", expression, type->toString());
    }

    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

static void convertStructToTuple(const IR::Type_StructLike *structType, IR::Type_Tuple *tuple) {
    for (auto field : structType->fields) {
        if (auto ft = field->type->to<IR::Type_Bits>()) {
            tuple->components.push_back(ft);
        } else if (auto ft = field->type->to<IR::Type_StructLike>()) {
            convertStructToTuple(ft, tuple);
        } else if (auto ft = field->type->to<IR::Type_InfInt>()) {
            tuple->components.push_back(ft);
        } else if (auto ft = field->type->to<IR::Type_Boolean>()) {
            tuple->components.push_back(ft);
        } else {
            typeError("Type not supported %1% for struct field %2% in 'select'", field->type,
                      field);
        }
    }
}

const IR::SelectCase *TypeInference::matchCase(const IR::SelectExpression *select,
                                               const IR::Type_BaseList *selectType,
                                               const IR::SelectCase *selectCase,
                                               const IR::Type *caseType) {
    // The selectType is always a tuple
    // If the caseType is a set type, we unify the type of the set elements
    if (auto *set = caseType->to<IR::Type_Set>()) caseType = set->elementType;
    // The caseType may be a simple type, and then we have to unwrap the selectType
    if (caseType->is<IR::Type_Dontcare>()) return selectCase;

    if (auto *sl = caseType->to<IR::Type_StructLike>()) {
        auto tupleType = new IR::Type_Tuple();
        convertStructToTuple(sl, tupleType);
        caseType = tupleType;
    }
    const IR::Type *useSelType = selectType;
    if (!caseType->is<IR::Type_BaseList>()) {
        if (selectType->components.size() != 1) {
            typeError("Type mismatch %1% (%2%) vs %3% (%4%)", select->select,
                      selectType->toString(), selectCase, caseType->toString());
            return nullptr;
        }
        useSelType = selectType->components.at(0);
    }
    auto tvs = unifyCast(
        select, useSelType, caseType,
        "'match' case label '%1%' has type '%2%' which does not match the expected type '%3%'",
        {selectCase->keyset, caseType, useSelType});
    if (tvs == nullptr) return nullptr;
    ConstantTypeSubstitution cts(tvs, typeMap, this);
    auto ks = cts.convert(selectCase->keyset, getChildContext());
    if (::errorCount() > 0) return selectCase;

    if (ks != selectCase->keyset)
        selectCase = new IR::SelectCase(selectCase->srcInfo, ks, selectCase->state);
    return selectCase;
}

const IR::Node *TypeInference::postorder(IR::This *expression) {
    if (done()) return expression;
    auto decl = findContext<IR::Declaration_Instance>();
    if (findContext<IR::Function>() == nullptr || decl == nullptr)
        typeError("%1%: can only be used in the definition of an abstract method", expression);
    auto type = getType(decl);
    setType(expression, type);
    setType(getOriginal(), type);
    return expression;
}

const IR::Node *TypeInference::postorder(IR::DefaultExpression *expression) {
    if (!done()) {
        setType(expression, IR::Type_Dontcare::get());
        setType(getOriginal(), IR::Type_Dontcare::get());
    }
    setCompileTimeConstant(expression);
    setCompileTimeConstant(getOriginal<IR::Expression>());
    return expression;
}

bool TypeInference::containsHeader(const IR::Type *type) {
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Stack>() ||
        type->is<IR::Type_HeaderUnion>())
        return true;
    if (auto *st = type->to<IR::Type_Struct>()) {
        for (auto f : st->fields)
            if (containsHeader(f->type)) return true;
    }
    return false;
}

/// Expressions that appear in a select expression are restricted to a small
/// number of types: bits, enums, serializable enums, and booleans.
static bool validateSelectTypes(const IR::Type *type, const IR::SelectExpression *expression) {
    if (auto tuple = type->to<IR::Type_BaseList>()) {
        for (auto ct : tuple->components) {
            auto check = validateSelectTypes(ct, expression);
            if (!check) return false;
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

const IR::Node *TypeInference::postorder(IR::SelectExpression *expression) {
    if (done()) return expression;
    auto selectType = getType(expression->select);
    if (selectType == nullptr) return expression;

    // Check that the selectType is determined
    auto tuple = selectType->to<IR::Type_BaseList>();
    BUG_CHECK(tuple != nullptr, "%1%: Expected a tuple type for the select expression, got %2%",
              expression, selectType);
    if (!validateSelectTypes(selectType, expression)) return expression;

    bool changes = false;
    IR::Vector<IR::SelectCase> vec;
    for (auto sc : expression->selectCases) {
        auto type = getType(sc->keyset);
        if (type == nullptr) return expression;
        auto newsc = matchCase(expression, tuple, sc, type);
        vec.push_back(newsc);
        if (newsc != sc) changes = true;
    }
    if (changes)
        expression =
            new IR::SelectExpression(expression->srcInfo, expression->select, std::move(vec));
    setType(expression, IR::Type_State::get());
    setType(getOriginal(), IR::Type_State::get());
    return expression;
}

const IR::Node *TypeInference::postorder(IR::AttribLocal *local) {
    setType(local, local->type);
    setType(getOriginal(), local->type);
    return local;
}

}  // namespace P4
