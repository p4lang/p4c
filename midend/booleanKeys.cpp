#include "midend/booleanKeys.h"

#include <cstddef>

#include "ir/irutils.h"
#include "ir/vector.h"

namespace P4C::P4 {

const IR::Node *CastBooleanTableKeys::postorder(IR::KeyElement *key) {
    if (key->expression->type->is<IR::Type_Boolean>()) {
        key->expression =
            new IR::Cast(key->expression->srcInfo, IR::Type_Bits::get(1), key->expression);
    }
    return key;
}

const IR::Node *CastBooleanTableKeys::postorder(IR::Entry *entry) {
    auto *keyExprs = entry->keys->clone();
    for (size_t idx = 0; idx < keyExprs->size(); ++idx) {
        const auto *keyExpr = keyExprs->components.at(idx);
        if (const auto *boolLiteral = keyExpr->to<IR::BoolLiteral>()) {
            int v = boolLiteral->value ? 1 : 0;
            keyExprs->components[idx] = IR::Constant::get(IR::Type_Bits::get(1), v);
        }
    }
    entry->keys = keyExprs;
    return entry;
}

}  // namespace P4C::P4
