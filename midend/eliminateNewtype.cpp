#include "eliminateNewtype.h"

namespace P4 {

const IR::Node* DoReplaceNewtype::postorder(IR::Cast* expression) {
    auto orig = getOriginal<IR::Cast>();
    auto type = typeMap->getTypeType(orig->destType, true);
    if (type == nullptr)
        return expression;
    if (type->is<IR::Type_Newtype>()) {
        LOG2("Removing cast" << expression);
        // Discard the cast altogether, it should no longer be necessary
        // We can't just keep the cast in, because if the new type is a struct
        // casts to struct types are not actually legal.
        return expression->expr;
    }
    return expression;
}

}  // namespace P4
