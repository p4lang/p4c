#include "backends/p4tools/common/lib/table_utils.h"

namespace P4Tools::TableUtils {

void checkTableImmutability(const IR::P4Table *table, TableProperties &properties) {
    bool isConstant = false;
    const auto *entriesAnnotation = table->properties->getProperty("entries");
    if (entriesAnnotation != nullptr) {
        isConstant = entriesAnnotation->isConstant;
    }
    // Also check if the table is invisible to the control plane.
    // This also implies that it cannot be modified.
    properties.tableIsImmutable = isConstant || table->getAnnotation("hidden") != nullptr;
    const auto *defaultAction = table->properties->getProperty("default_action");
    CHECK_NULL(defaultAction);
    properties.defaultIsImmutable = defaultAction->isConstant;
}

std::vector<const IR::ActionListElement *> buildTableActionList(const IR::P4Table *table) {
    std::vector<const IR::ActionListElement *> tableActionList;
    const auto *actionList = table->getActionList();
    if (actionList == nullptr) {
        return tableActionList;
    }
    for (size_t idx = 0; idx < actionList->size(); idx++) {
        const auto *action = actionList->actionList.at(idx);
        if (action->getAnnotation("defaultonly") != nullptr) {
            continue;
        }
        // Check some properties of the list.
        CHECK_NULL(action->expression);
        action->expression->checkedTo<IR::MethodCallExpression>();
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

}  // namespace P4Tools::TableUtils
