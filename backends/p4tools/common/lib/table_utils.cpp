#include "backends/p4tools/common/lib/table_utils.h"

#include "ir/irutils.h"

namespace P4Tools::TableUtils {

void checkTableImmutability(const IR::P4Table &table, TableProperties &properties) {
    bool isConstant = false;
    const auto *entriesAnnotation = table.properties->getProperty("entries");
    if (entriesAnnotation != nullptr) {
        isConstant = entriesAnnotation->isConstant;
    }
    // Also check if the table is invisible to the control plane.
    // This also implies that it cannot be modified.
    properties.tableIsImmutable = isConstant || table.getAnnotation("hidden") != nullptr;
    const auto *defaultAction = table.properties->getProperty("default_action");
    CHECK_NULL(defaultAction);
    properties.defaultIsImmutable = defaultAction->isConstant;
}

std::vector<const IR::ActionListElement *> buildTableActionList(const IR::P4Table &table) {
    std::vector<const IR::ActionListElement *> tableActionList;
    const auto *actionList = table.getActionList();
    if (actionList == nullptr) {
        return tableActionList;
    }
    for (size_t idx = 0; idx < actionList->size(); idx++) {
        const auto *action = actionList->actionList.at(idx);
        if (action->getAnnotation("defaultonly") != nullptr) {
            continue;
        }
        tableActionList.emplace_back(action);
    }
    return tableActionList;
}

bool compareLPMEntries(const IR::Entry *leftIn, const IR::Entry *rightIn, size_t lpmIndex) {
    // Get the entry key that matches with provided lpm index.
    // There should only be one and we only need to compare this precision.
    BUG_CHECK(
        lpmIndex < leftIn->keys->components.size() && lpmIndex < rightIn->keys->components.size(),
        "LPM index out of range.");
    const auto *left = leftIn->keys->components.at(lpmIndex);
    const auto *right = rightIn->keys->components.at(lpmIndex);

    // The expressions are equivalent, so no need to compare.
    if (left->equiv(*right)) {
        return true;
    }

    // DefaultExpression are least precise.
    if (left->is<IR::DefaultExpression>()) {
        return true;
    }
    if (right->is<IR::DefaultExpression>()) {
        return false;
    }
    // Constants are implicitly more precise than masks.
    if (left->is<IR::Constant>() && right->is<IR::Mask>()) {
        return true;
    }
    if (left->is<IR::Mask>() && right->is<IR::Constant>()) {
        return false;
    }

    const IR::Constant *leftMaskVal = nullptr;
    const IR::Constant *rightMaskVal = nullptr;
    if (const auto *leftMask = left->to<IR::Mask>()) {
        leftMaskVal = leftMask->right->checkedTo<IR::Constant>();
        // The other value must be a mask at this point.
        if (const auto *rightMask = right->checkedTo<IR::Mask>()) {
            rightMaskVal = rightMask->right->checkedTo<IR::Constant>();
        }
        return (leftMaskVal->value) >= (rightMaskVal->value);
    }

    BUG("Unhandled sort elements type: left: %1% - %2% \n right: %3% - %4% ", left,
        left->node_type_name(), right, right->node_type_name());
}

const IR::PathExpression *getDefaultActionName(const IR::P4Table &table) {
    const auto *defaultAction = table.getDefaultAction();
    const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
    return tableAction->method->checkedTo<IR::PathExpression>();
}

const IR::Expression *computeEntryMatch(const IR::P4Table &table, const IR::Entry &entry,
                                        const IR::Key &key) {
    auto numKeys = key.keyElements.size();
    // If there are no entries or keys, there is nothing we can match against.
    if (numKeys == 0) {
        return IR::getBoolLiteral(false);
    }
    BUG_CHECK(key.keyElements.size() == entry.keys->size(),
              "The entry key list and key match list must be equal in size.");
    const IR::Expression *entryMatchCondition = IR::getBoolLiteral(true);
    for (size_t idx = 0; idx < numKeys; ++idx) {
        const auto *keyElement = key.keyElements.at(idx);
        const auto *keyExpr = keyElement->expression;
        BUG_CHECK(keyExpr != nullptr, "Entry %1% in table %2% is null", entry, table);
        const auto *entryKey = entry.keys->components.at(idx);
        // DefaultExpressions always match, so do not even consider them in the equation.
        if (entryKey->is<IR::DefaultExpression>()) {
            continue;
        }
        if (const auto *rangeExpr = entryKey->to<IR::Range>()) {
            const auto *minKey = rangeExpr->left;
            const auto *maxKey = rangeExpr->right;
            entryMatchCondition = new IR::LAnd(
                entryMatchCondition,
                new IR::LAnd(new IR::Leq(minKey, keyExpr), new IR::Leq(keyExpr, maxKey)));
        } else if (const auto *maskExpr = entryKey->to<IR::Mask>()) {
            entryMatchCondition = new IR::LAnd(
                entryMatchCondition, new IR::Equ(new IR::BAnd(keyExpr, maskExpr->right),
                                                 new IR::BAnd(maskExpr->left, maskExpr->right)));
        } else {
            entryMatchCondition = new IR::LAnd(entryMatchCondition, new IR::Equ(keyExpr, entryKey));
        }
    }
    return entryMatchCondition;
}

}  // namespace P4Tools::TableUtils
