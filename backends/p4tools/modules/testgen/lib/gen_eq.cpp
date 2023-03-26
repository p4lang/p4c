#include "backends/p4tools/modules/testgen/lib/gen_eq.h"

#include <cstddef>
#include <string>
#include <vector>

#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

const IR::Expression *GenEq::equate(const IR::Expression *target, const IR::Expression *keyset) {
    if (const auto *defaultKey = keyset->to<IR::DefaultExpression>()) {
        return equate(target, defaultKey);
    }

    if (const auto *listKey = keyset->to<IR::ListExpression>()) {
        return equate(target, listKey);
    }

    if (const auto *maskKey = keyset->to<IR::Mask>()) {
        return equate(target, maskKey);
    }

    if (const auto *rangeKey = keyset->to<IR::Range>()) {
        return equate(target, rangeKey);
    }

    // If the target is a list expression, it had better be a singleton. In this case, recurse into
    // the singleton element.
    if (const auto *listTarget = target->to<IR::ListExpression>()) {
        BUG_CHECK(listTarget->size() == 1, "Cannot match %1% with %2%", target, keyset);
        return equate(listTarget->components.at(0), keyset);
    }

    return mkEq(target, keyset);
}

const IR::Expression *GenEq::equate(const IR::Expression * /*target*/,
                                    const IR::DefaultExpression * /*keyset*/) {
    return new IR::BoolLiteral(IR::Type::Boolean::get(), true);
}

const IR::Expression *GenEq::equate(const IR::Expression *target,
                                    const IR::ListExpression *keyset) {
    // If the keyset is a singleton list, recurse into the singleton element.
    if (keyset->size() == 1) {
        return equate(target, keyset->components.at(0));
    }

    const auto *listTarget = target->to<IR::ListExpression>();
    BUG_CHECK(listTarget, "Cannot match %1% with %2%", target, keyset);
    return equate(listTarget, keyset);
}

const IR::Expression *GenEq::equate(const IR::Expression *target, const IR::Mask *keyset) {
    // If the target is a list expression, it had better be a singleton. In this case, recurse into
    // the singleton element.
    if (const auto *listTarget = target->to<IR::ListExpression>()) {
        BUG_CHECK(listTarget->size() == 1, "Cannot match %1% with %2%", target, keyset);
        return equate(listTarget->components.at(0), keyset);
    }

    // Let a &&& b represent the keyset.
    // We return a & b == target & b.
    return mkEq(new IR::BAnd(target->type, keyset->left, keyset->right),
                new IR::BAnd(target->type, target, keyset->right));
}

const IR::Expression *GenEq::equate(const IR::Expression *target, const IR::Range *keyset) {
    // If the target is a list expression, it had better be a singleton. In this case, recurse into
    // the singleton element.
    if (const auto *listTarget = target->to<IR::ListExpression>()) {
        BUG_CHECK(listTarget->size() == 1, "Cannot match %1% with %2%", target, keyset);
        return equate(listTarget->components.at(0), keyset);
    }

    const auto *boolType = IR::Type::Boolean::get();
    return new IR::LAnd(boolType, new IR::Leq(boolType, keyset->left, target),
                        new IR::Leq(boolType, target, keyset->right));
}

const IR::Expression *GenEq::equate(const IR::ListExpression *target,
                                    const IR::ListExpression *keyset) {
    // If the keyset is a singleton list, recurse into the singleton element. Similarly for the
    // target.
    if (keyset->size() == 1) {
        return equate(target, keyset->components.at(0));
    }
    if (target->size() == 1) {
        return equate(target->components.at(0), keyset);
    }

    BUG_CHECK(target->size() == keyset->size(), "Cannot match %1% with %2%", target, keyset);

    const IR::Expression *result = new IR::BoolLiteral(IR::Type::Boolean::get(), true);
    bool firstLoop = true;
    for (size_t i = 0; i < target->size(); i++) {
        const auto *conjunct = equate(target->components.at(i), keyset->components.at(i));
        if (firstLoop) {
            result = conjunct;
            firstLoop = false;
        } else {
            result = new IR::LAnd(IR::Type::Boolean::get(), result, conjunct);
        }
    }

    return result;
}

const IR::Equ *GenEq::mkEq(const IR::Expression *e1, const IR::Expression *e2) {
    return new IR::Equ(IR::Type::Boolean::get(), e1, e2);
}

}  // namespace P4Testgen

}  // namespace P4Tools
