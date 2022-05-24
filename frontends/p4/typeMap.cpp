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

#include "typeMap.h"
#include "lib/map.h"

namespace P4 {

bool TypeMap::typeIsEmpty(const IR::Type* type) const {
    if (auto bt = type->to<IR::Type_Bits>()) {
        return bt->size == 0;
    } else if (auto vt = type->to<IR::Type_Varbits>()) {
        return vt->size == 0;
    } else if (auto tt = type->to<IR::Type_Tuple>()) {
        if (tt->getSize() == 0)
            return true;
        for (auto ft : tt->components) {
            auto t = getTypeType(ft, true);
            if (!typeIsEmpty(t))
                return false;
        }
        return true;
    } else if (auto ts = type->to<IR::Type_Stack>()) {
        if (!ts->sizeKnown())
            return false;
        return ts->getSize() == 0;
    } else if (auto tst = type->to<IR::Type_Struct>()) {
        if (tst->fields.size() == 0)
            return true;
        for (auto f : tst->fields) {
            auto t = getTypeType(f->type, true);
            if (!typeIsEmpty(t))
                return false;
        }
        return true;
    }
    return false;
}

void TypeMap::dbprint(std::ostream& out) const {
    out << "TypeMap for " << dbp(program) << std::endl;
    for (auto it : typeMap)
        out << "\t" << dbp(it.first) << "->" << dbp(it.second) << std::endl;
    out << "Left values" << std::endl;
    for (auto it : leftValues)
        out << "\t" << dbp(it) << std::endl;
    out << "Constants" << std::endl;
    for (auto it : constants)
        out << "\t" << dbp(it) << std::endl;
    out << "Type variables" << std::endl;
    out << allTypeVariables << std::endl;
    out << "--------------" << std::endl;
}

void TypeMap::setLeftValue(const IR::Expression* expression) {
    leftValues.insert(expression);
    LOG3("Left value " << dbp(expression));
}

void TypeMap::setCompileTimeConstant(const IR::Expression* expression) {
    constants.insert(expression);
    LOG3("Constant value " << dbp(expression));
}

bool TypeMap::isCompileTimeConstant(const IR::Expression* expression) const {
    bool result = constants.find(expression) != constants.end();
    LOG3(dbp(expression) << (result ? " constant" : " not constant"));
    return result;
}

// Method copies properties from expression to "to" expression.
void TypeMap::cloneExpressionProperties(const IR::Expression* to,
                                        const IR::Expression* from) {
    auto type = getType(from, true);
    setType(to, type);
    if (isLeftValue(from))
        setLeftValue(to);
    if (isCompileTimeConstant(from))
        setCompileTimeConstant(to);
}

void TypeMap::clear() {
    LOG3("Clearing typeMap");
    typeMap.clear(); leftValues.clear(); constants.clear(); allTypeVariables.clear();
    program = nullptr;
}

void TypeMap::checkPrecondition(const IR::Node* element, const IR::Type* type) const {
    CHECK_NULL(element); CHECK_NULL(type);
    if (type->is<IR::Type_Name>())
        BUG("Element %1% maps to a Type_Name %2%", dbp(element), dbp(type));
}

void TypeMap::setType(const IR::Node* element, const IR::Type* type) {
    checkPrecondition(element, type);
    auto it = typeMap.find(element);
    if (it != typeMap.end()) {
        const IR::Type* existingType = it->second;
        if (!implicitlyConvertibleTo(type, existingType))
            BUG("Changing type of %1% in type map from %2% to %3%",
                dbp(element), dbp(existingType), dbp(type));
        return;
    }
    LOG3("setType " << dbp(element) << " => " << dbp(type));
    typeMap.emplace(element, type);
}

const IR::Type* TypeMap::getType(const IR::Node* element, bool notNull) const {
    CHECK_NULL(element);
    auto result = get(typeMap, element);
    LOG4("Looking up type for " << dbp(element) << " => " << dbp(result));
    if (notNull && result == nullptr)
        BUG_CHECK(errorCount() > 0, "Could not find type for %1%", dbp(element));
    if (result != nullptr && result->is<IR::Type_Name>())
        BUG("%1% in map", dbp(result));
    return result;
}

const IR::Type* TypeMap::getTypeType(const IR::Node* element, bool notNull) const {
    CHECK_NULL(element);
    auto result = getType(element, notNull);
    BUG_CHECK(result->is<IR::Type_Type>(), "%1%: expected a TypeType", result);
    return result->to<IR::Type_Type>()->type;
}

void TypeMap::addSubstitutions(const TypeVariableSubstitution* tvs) {
    if (tvs == nullptr || tvs->isIdentity())
        return;
    LOG3("New type variables " << tvs);
    allTypeVariables.simpleCompose(tvs);
}

// Deep structural equivalence between canonical types.
// Does not do unification of type variables - a type variable is only
// equivalent to itself.  nullptr is only equivalent to nullptr.
bool TypeMap::equivalent(const IR::Type* left, const IR::Type* right, bool strict) const {
    if (!strict)
        strict = strictStruct;
    LOG3("Checking equivalence of " << left << " and " << right);
    if (left == nullptr)
        return right == nullptr;
    if (right == nullptr)
        return false;
    if (left->node_type_name() != right->node_type_name())
        return false;

    // Below we are sure that it's the same Node class
    if (left->is<IR::Type_Base>() || left->is<IR::Type_Newtype>() ||
        left->is<IR::Type_Var>() || left->is<IR::Type_Name>())
        // The last case can happen when checking generic functions
        return *left == *right;
    if (auto tt = left->to<IR::Type_Type>())
        return equivalent(tt->type, right->to<IR::Type_Type>()->type, strict);
    if (left->is<IR::Type_Error>())
        return true;
    if (auto lv = left->to<IR::ITypeVar>()) {
        auto rv = right->to<IR::ITypeVar>();
        return lv->getVarName() == rv->getVarName() && lv->getDeclId() == rv->getDeclId();
    }
    if (auto ls = left->to<IR::Type_Stack>()) {
        auto rs = right->to<IR::Type_Stack>();
        if (!ls->sizeKnown()) {
            ::error(ErrorType::ERR_TYPE_ERROR,
                    "%1%: Size of header stack type should be a constant", left);
            return false;
        }
        if (!rs->sizeKnown()) {
            ::error(ErrorType::ERR_TYPE_ERROR,
                    "%1%: Size of header stack type should be a constant", right);
            return false;
        }
        return equivalent(ls->elementType, rs->elementType, strict) &&
                ls->getSize() == rs->getSize();
    }
    if (auto le = left->to<IR::Type_Enum>()) {
        auto re = right->to<IR::Type_Enum>();
        return le->name == re->name;
    }
    if (auto le = left->to<IR::Type_SerEnum>()) {
        auto re = right->to<IR::Type_SerEnum>();
        return le->name == re->name;
    }
    if (auto sl = left->to<IR::Type_StructLike>()) {
        auto sr = right->to<IR::Type_StructLike>();
        if (strict &&  // non-strict equivalence does not check names
            sl->name != sr->name &&
            !sl->is<IR::Type_UnknownStruct>() &&
            !sr->is<IR::Type_UnknownStruct>())
            return false;
        if (sl->fields.size() != sr->fields.size())
            return false;
        for (size_t i = 0; i < sl->fields.size(); i++) {
            auto fl = sl->fields.at(i);
            auto fr = sr->fields.at(i);
            if (fl->name != fr->name)
                return false;
            if (!equivalent(fl->type, fr->type, strict))
                return false;
        }
        return true;
    }
    if (auto lt = left->to<IR::Type_Tuple>()) {
        auto rt = right->to<IR::Type_Tuple>();
        if (lt->components.size() != rt->components.size())
            return false;
        for (size_t i = 0; i < lt->components.size(); i++) {
            auto l = lt->components.at(i);
            auto r = rt->components.at(i);
            if (!equivalent(l, r, strict))
                return false;
        }
        return true;
    }
    if (auto lt = left->to<IR::Type_List>()) {
        auto rt = right->to<IR::Type_List>();
        if (lt->components.size() != rt->components.size())
            return false;
        for (size_t i = 0; i < lt->components.size(); i++) {
            auto l = lt->components.at(i);
            auto r = rt->components.at(i);
            if (!equivalent(l, r, strict))
                return false;
        }
        return true;
    }
    if (auto lt = left->to<IR::Type_Set>()) {
        auto rt = right->to<IR::Type_Set>();
        return equivalent(lt->elementType, rt->elementType, strict);
    }
    if (auto lp = left->to<IR::Type_Package>()) {
        auto rp = right->to<IR::Type_Package>();
        // The following gets into an infinite loop, since the return type of the
        // constructor is the Type_Package itself.
        // return equivalent(lp->getConstructorMethodType(), rp->getConstructorMethodType());
        // The following code is equivalent.
        auto lm = lp->getConstructorMethodType();
        auto rm = rp->getConstructorMethodType();
        if (lm->typeParameters->size() != rm->typeParameters->size())
            return false;
        for (size_t i = 0; i < lm->typeParameters->size(); i++) {
            auto lp = lm->typeParameters->parameters.at(i);
            auto rp = rm->typeParameters->parameters.at(i);
            if (!equivalent(lp, rp, strict))
                return false;
        }
        // Don't check the return type.
        if (lm->parameters->size() != rm->parameters->size())
            return false;
        for (size_t i = 0; i < lm->parameters->size(); i++) {
            auto lp = lm->parameters->parameters.at(i);
            auto rp = rm->parameters->parameters.at(i);
            if (lp->direction != rp->direction)
                return false;
            if (!equivalent(lp->type, rp->type, strict))
                return false;
        }
        return true;
    }
    if (auto a = left->to<IR::IApply>()) {
        return equivalent(a->getApplyMethodType(),
                          right->to<IR::IApply>()->getApplyMethodType(), strict);
    }
    if (auto ls = left->to<IR::Type_SpecializedCanonical>()) {
        auto rs = right->to<IR::Type_SpecializedCanonical>();
        if (!equivalent(ls->baseType, rs->baseType, strict))
            return false;
        if (ls->arguments->size() != rs->arguments->size())
            return false;
        for (size_t i = 0; i < ls->arguments->size(); i++) {
            auto lp = ls->arguments->at(i);
            auto rp = rs->arguments->at(i);
            if (!equivalent(lp, rp, strict))
                return false;
        }
        return true;
    }
    if (auto la = left->to<IR::Type_ActionEnum>()) {
        auto ra = right->to<IR::Type_ActionEnum>();
        return la->actionList == ra->actionList;  // pointer comparison
    }
    if (left->is<IR::Type_Method>() || left->is<IR::Type_Action>()) {
        auto lm = left->to<IR::Type_MethodBase>();
        auto rm = right->to<IR::Type_MethodBase>();
        if (lm->typeParameters->size() != rm->typeParameters->size())
            return false;
        for (size_t i = 0; i < lm->typeParameters->size(); i++) {
            auto lp = lm->typeParameters->parameters.at(i);
            auto rp = rm->typeParameters->parameters.at(i);
            if (!equivalent(lp, rp, strict))
                return false;
        }
        if (!equivalent(lm->returnType, rm->returnType, strict))
            return false;
        if (lm->parameters->size() != rm->parameters->size())
            return false;
        for (size_t i = 0; i < lm->parameters->size(); i++) {
            auto lp = lm->parameters->parameters.at(i);
            auto rp = rm->parameters->parameters.at(i);
            if (lp->direction != rp->direction)
                return false;
            if (!equivalent(lp->type, rp->type, strict))
                return false;
        }
        return true;
    }
    if (auto le = left->to<IR::Type_Extern>()) {
        auto re = right->to<IR::Type_Extern>();
        return le->name == re->name;
    }

    BUG_CHECK(::errorCount(), "%1%: Unexpected type check for equivalence", dbp(left));
    // The following are not expected to be compared for equivalence:
    // Type_Dontcare, Type_Unknown, Type_Name, Type_Specialized, Type_Typedef
    return false;
}

bool TypeMap::implicitlyConvertibleTo(const IR::Type* from, const IR::Type* to) const {
    if (equivalent(from, to))
        return true;
    if (from->is<IR::Type_InfInt>() && to->is<IR::Type_InfInt>())
        // this case is not caught by the equivalence check
        return true;
    if (auto rt = to->to<IR::Type_BaseList>()) {
        if (auto sl = from->to<IR::Type_StructLike>()) {
            // We allow implicit casts from list types to structs
            if (sl->fields.size() != rt->components.size())
                return false;
            for (size_t i = 0; i < rt->components.size(); i++) {
                auto fl = sl->fields.at(i);
                auto r = rt->components.at(i);
                if (!implicitlyConvertibleTo(fl->type, r))
                    return false;
            }
            return true;
        } else if (auto sl = from->to<IR::Type_Tuple>()) {
            // We allow implicit casts from list types to tuples
            if (sl->components.size() != rt->components.size())
                return false;
            for (size_t i = 0; i < rt->components.size(); i++) {
                auto f = sl->components.at(i);
                auto r = rt->components.at(i);
                if (!implicitlyConvertibleTo(f, r))
                    return false;
            }
            return true;
        }
    }
    return false;
}

// Used for tuples, stacks and lists only
const IR::Type* TypeMap::getCanonical(const IR::Type* type) {
    // Currently a linear search; hopefully this won't be too expensive in practice
    std::vector<const IR::Type*>* searchIn;
    if (type->is<IR::Type_Stack>())
        searchIn = &canonicalStacks;
    else if (type->is<IR::Type_Tuple>())
        searchIn = &canonicalTuples;
    else if (type->is<IR::Type_List>())
        searchIn = &canonicalLists;
    else
        BUG("%1%: unexpected type", type);

    for (auto t : *searchIn) {
        if (equivalent(type, t, true))
            return t;
    }
    searchIn->push_back(type);
    return type;
}

int TypeMap::widthBits(const IR::Type* type, const IR::Node* errorPosition, bool max) {
    CHECK_NULL(type);
    const IR::Type* t;
    if (auto tt = type->to<IR::Type_Type>())
        t = tt->type;
    else
        t = getTypeType(type, true);
    if (auto tb = t->to<IR::Type_Bits>()) {
        return tb->width_bits();
    } else if (auto ts = t->to<IR::Type_StructLike>()) {
        int result = 0;
        bool isUnion = t->is<IR::Type_HeaderUnion>();
        for (auto f : ts->fields) {
            int w = widthBits(f->type, errorPosition, max);
            if (w < 0)
                return w;
            if (isUnion)
                result = std::max(w, result);
            else
                result = result + w;
        }
        return result;
    } else if (auto tt = t->to<IR::Type_Tuple>()) {
        int result = 0;
        for (auto f : tt->components) {
            int w = widthBits(f, errorPosition, max);
            if (w < 0)
                return w;
            result = result + w;
        }
        return result;
    } else if (auto te = t->to<IR::Type_SerEnum>()) {
        return widthBits(te->type, errorPosition, max);
    } else if (t->is<IR::Type_Boolean>()) {
        return 1;
    } else if (auto tnt = t->to<IR::Type_Newtype>()) {
        return widthBits(tnt->type, errorPosition, max);
    } else if (auto vb = type->to<IR::Type_Varbits>()) {
        if (max)
            return vb->size;
        return 0;
    } else if (auto ths = t->to<IR::Type_Stack>()) {
        auto w = widthBits(ths->elementType, errorPosition, max);
        return w * ths->getSize();
    }
    ::error(ErrorType::ERR_UNSUPPORTED, "%1%: width not well-defined for values of type %2%",
            errorPosition, t);
    return -1;
}

}  // namespace P4
