#include "eliminateNewtype.h"

namespace P4 {

const IR::Node* DoReplaceNewtype::postorder(IR::Cast* expression) {
    auto type = typeMap->getTypeType(expression->type, true);
    if (type == nullptr)
        return expression;
    if (type->is<IR::Type_Newtype>()) {
        LOG2("Removing cast" << expression);
        // Discard the cast altogether, it should no longer be necessary
        return expression->expr;
    }
    return expression;
}

}  // namespace P4
