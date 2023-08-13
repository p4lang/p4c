#include "midend/convertErrors.h"

#include <cstddef>
#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/exceptions.h"
#include "lib/map.h"

namespace P4 {

const IR::Node *DoConvertErrors::preorder(IR::Type_Error *type) {
    bool convert = policy->convert(type);
    if (!convert) {
        return type;
    }
    size_t count = type->members.size();
    size_t width = policy->errorSize(count);
    BUG_CHECK(count <= (1ULL << width), "%1%: not enough bits to represent %2%", width, type);
    // using the same data structure as for enum elimination.
    auto *r = new P4::EnumRepresentation(type->srcInfo, width);
    const auto *canontype = typeMap->getTypeType(getOriginal(), true);
    BUG_CHECK(canontype->is<IR::Type_Error>(), "canon type of error %s is non error %s?", type,
              canontype);
    repr.emplace(canontype->to<IR::Type_Error>()->name, r);
    for (const auto *d : type->members) {
        r->add(d->name.name);
    }
    return type;
}

const IR::Node *DoConvertErrors::postorder(IR::Type_Name *type) {
    const auto *canontype = typeMap->getTypeType(getOriginal(), true);
    if (!canontype->is<IR::Type_Error>()) {
        return type;
    }
    if (findContext<IR::TypeNameExpression>() != nullptr) {
        // This will be resolved by the caller.
        return type;
    }
    auto errorType = canontype->to<IR::Type_Error>()->name;
    auto *r = ::get(repr, errorType);
    if (r == nullptr) {
        return type;
    }
    return r->type;
}

const IR::Node *DoConvertErrors::postorder(IR::Member *member) {
    if (!member->type->is<IR::Type_Error>()) {
        return member;
    }
    if (!member->type->is<IR::Type_Error>()) {
        return member;
    }
    auto *r = ::get(repr, member->type->to<IR::Type_Error>()->name);
    if (!member->expr->is<IR::TypeNameExpression>()) {
        // variable
        auto *newMember = member->clone();
        newMember->type = r->type;
        return newMember;
    }
    unsigned value = r->get(member->member.name);
    auto *cst = new IR::Constant(member->srcInfo, r->type, value);
    return cst;
}

IR::IndexedVector<IR::SerEnumMember> *ChooseErrorRepresentation::assignValues(
    IR::Type_Error *type, unsigned width) const {
    auto *members = new IR::IndexedVector<IR::SerEnumMember>;
    unsigned idx = 0;
    for (const auto *d : type->members) {
        members->push_back(new IR::SerEnumMember(
            d->name.name, new IR::Constant(IR::Type_Bits::get(width), idx++)));
    }
    return members;
}

}  // namespace P4
