#include "validateSerEnums.h"

namespace P4 {

std::pair<int, bool> resolveTypeBitWidth(const IR::Type *type, P4::ReferenceMap *refMap,
                                         const IR::Type_SerEnum *enumDecl) {
    if (const auto *bit = type->to<IR::Type_Bits>()) {
        return {bit->size, bit->isSigned};
    }
    if (const auto *typeName = type->to<IR::Type_Name>()) {
        const auto *typeDef = refMap->getDeclaration(typeName->path)->to<IR::Type_Typedef>();
        if (typeDef) return resolveTypeBitWidth(typeDef->type, refMap, enumDecl);
        // otherwise fall to the error below
    }
    ::error(ErrorType::ERR_UNEXPECTED, "%1%: Cannot resolve serialized enum underlying type %2%",
            enumDecl, type);
    return {0, false};
}

void ValidateSerEnums::postorder(const IR::SerEnumMember *item) {
    const auto *enumDecl = findContext<IR::Type_SerEnum>();
    BUG_CHECK(enumDecl, "%1%: Serialized enum member appears outside enum declaration (invalid IR)",
              item);
    BUG_CHECK(enumDecl->type, "Enum missing a type: %1%", enumDecl);
    const auto [bitWidth, isSigned] = resolveTypeBitWidth(enumDecl->type, refMap, enumDecl);
    if (const auto *constant = item->value->to<IR::Constant>()) {
        // signed values are two's complement, so [-2^(n-1)..2^(n-1)-1]
        big_int low = isSigned ? -(big_int(1) << bitWidth - 1) : big_int(0);
        big_int high = (big_int(1) << (isSigned ? bitWidth - 1 : bitWidth)) - 1;

        if (constant->value < low || constant->value > high) {
            ::error(ErrorType::ERR_TYPE_ERROR,
                    "%1%: Serialized enum constant value %2% is out of bounds of the underlying "
                    "type %3%.",
                    item, constant->value, enumDecl->type);
        }
    }
}

}  // namespace P4
