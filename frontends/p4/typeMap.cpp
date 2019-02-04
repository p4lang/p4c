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
    LOG1("Left value " << dbp(expression));
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
        if (!TypeMap::implicitlyConvertibleTo(type, existingType))
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
    if (notNull && result == nullptr) {
        BUG("Could not find type for %1%", dbp(element));
    }
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
bool TypeMap::equivalent(const IR::Type* left, const IR::Type* right) {
    if (left == nullptr)
        return right == nullptr;
    if (right == nullptr)
        return false;
    if (left->node_type_name() != right->node_type_name())
        return false;

    // Below we are sure that it's the same Node class
    if (left->is<IR::Type_Base>() || left->is<IR::Type_Newtype>())
        return *left == *right;
    if (left->is<IR::Type_Type>())
        return equivalent(left->to<IR::Type_Type>()->type, right->to<IR::Type_Type>()->type);
    if (left->is<IR::Type_Error>())
        return true;
    if (left->is<IR::ITypeVar>()) {
        auto lv = left->to<IR::ITypeVar>();
        auto rv = right->to<IR::ITypeVar>();
        return lv->getVarName() == rv->getVarName() && lv->getDeclId() == rv->getDeclId();
    }
    if (left->is<IR::Type_Stack>()) {
        auto ls = left->to<IR::Type_Stack>();
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
        return equivalent(ls->elementType, rs->elementType) &&
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
    if (left->is<IR::Type_StructLike>()) {
        auto sl = left->to<IR::Type_StructLike>();
        auto sr = right->to<IR::Type_StructLike>();
        if (sl->fields.size() != sr->fields.size())
            return false;
        for (size_t i = 0; i < sl->fields.size(); i++) {
            auto fl = sl->fields.at(i);
            auto fr = sr->fields.at(i);
            if (fl->name != fr->name)
                return false;
            if (!equivalent(fl->type, fr->type))
                return false;
        }
        return true;
    }
    if (left->is<IR::Type_Tuple>()) {
        auto lt = left->to<IR::Type_Tuple>();
        auto rt = right->to<IR::Type_Tuple>();
        if (lt->components.size() != rt->components.size())
            return false;
        for (size_t i = 0; i < lt->components.size(); i++) {
            auto l = lt->components.at(i);
            auto r = rt->components.at(i);
            if (!equivalent(l, r))
                return false;
        }
        return true;
    }
    if (left->is<IR::Type_Set>()) {
        auto lt = left->to<IR::Type_Set>();
        auto rt = right->to<IR::Type_Set>();
        return equivalent(lt->elementType, rt->elementType);
    }
    if (left->is<IR::Type_Package>()) {
        auto lp = left->to<IR::Type_Package>();
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
            if (!equivalent(lp, rp))
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
            if (!equivalent(lp->type, rp->type))
                return false;
        }
        return true;
    }
    if (left->is<IR::IApply>()) {
        return equivalent(left->to<IR::IApply>()->getApplyMethodType(),
                          right->to<IR::IApply>()->getApplyMethodType());
    }
    if (left->is<IR::Type_SpecializedCanonical>()) {
        auto ls = left->to<IR::Type_SpecializedCanonical>();
        auto rs = right->to<IR::Type_SpecializedCanonical>();
        return equivalent(ls->substituted, rs->substituted);
    }
    if (left->is<IR::Type_ActionEnum>()) {
        auto la = left->to<IR::Type_ActionEnum>();
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
            if (!equivalent(lp, rp))
                return false;
        }
        if (!equivalent(lm->returnType, rm->returnType))
            return false;
        if (lm->parameters->size() != rm->parameters->size())
            return false;
        for (size_t i = 0; i < lm->parameters->size(); i++) {
            auto lp = lm->parameters->parameters.at(i);
            auto rp = rm->parameters->parameters.at(i);
            if (lp->direction != rp->direction)
                return false;
            if (!equivalent(lp->type, rp->type))
                return false;
        }
        return true;
    }
    if (left->is<IR::Type_Extern>()) {
        auto le = left->to<IR::Type_Extern>();
        auto re = right->to<IR::Type_Extern>();
        return le->name == re->name;
    }

    BUG("%1%: Unexpected type check for equivalence", dbp(left));
    // The following are not expected to be compared for equivalence:
    // Type_Dontcare, Type_Unknown, Type_Name, Type_Specialized, Type_Typedef
}

bool TypeMap::implicitlyConvertibleTo(const IR::Type* from, const IR::Type* to) {
    if (TypeMap::equivalent(from, to))
        return true;
    // We allow implicit casts from tuples to structs
    if (from->is<IR::Type_StructLike>()) {
        if (to->is<IR::Type_Tuple>()) {
            auto sl = from->to<IR::Type_StructLike>();
            auto rt = to->to<IR::Type_Tuple>();
            if (sl->fields.size() != rt->components.size())
                return false;
            for (size_t i = 0; i < rt->components.size(); i++) {
                auto fl = sl->fields.at(i);
                auto r = rt->components.at(i);
                if (!TypeMap::implicitlyConvertibleTo(fl->type, r))
                    return false;
            }
            return true;
        }
    }
    return false;
}


// Used for tuples and stacks only
const IR::Type* TypeMap::getCanonical(const IR::Type* type) {
    // Currently a linear search; hopefully this won't be too expensive in practice
    std::vector<const IR::Type*>* searchIn;
    if (type->is<IR::Type_Stack>())
        searchIn = &canonicalStacks;
    else if (type->is<IR::Type_Tuple>())
        searchIn = &canonicalTuples;
    else
        BUG("%1%: unexpected type", type);

    for (auto t : *searchIn) {
        if (TypeMap::equivalent(type, t))
            return t;
    }
    searchIn->push_back(type);
    return type;
}


}  // namespace P4
