#include "removeDuplicatedSelectLabels.h"

namespace P4 {

const IR::Node* DoRemoveDuplicatedSelectLabels::postorder(IR::SelectExpression* selectExpression) {
    auto vecCases = selectExpression->selectCases;
    std::vector<const IR::Constant*> vecOfConstLabels;
    std::vector<const IR::Range*> vecOfRangeLabels;
    // Vector that will contain optimized select casses
    IR::Vector<IR::SelectCase> newVecCasses;

    for (auto currentExpression : vecCases) {
        auto expression = currentExpression->keyset;
        // Current label is constant
        if (expression->is<IR::Constant>()) {
            const IR::Constant* constant = expression->to<IR::Constant>();
            auto value = constant->value;
            bool alreadyAssigned = false;

            for (const IR::Constant* previousLabel : vecOfConstLabels) {
                auto previousConstantValue = previousLabel->value;
                    // Check if same label already exists
                    if (value == previousConstantValue) {
                        alreadyAssigned = true;
                        ::warning(ErrorType::WARN_DUPLICATED, "Label %1% is already assigned."
                            " Optimized code will not contain this label.", constant);
                        break;
                }
            }
            // If same value is defined inside some previous range then this value is unreachable
            for (const IR::Range* previousLabel : vecOfRangeLabels) {
                auto previousLeftConstant = previousLabel->left->to<IR::Constant>();
                auto previousRightConstant = previousLabel->right->to<IR::Constant>();
                if ((value >= previousLeftConstant->value) &&
                                                (value <= previousRightConstant->value)) {
                    alreadyAssigned = true;
                    ::warning(ErrorType::WARN_DUPLICATED, "Label %1% is already assigned inside "
                            " range %2% .. %3%. Optimized code will not contain this label.",
                                 constant, previousLabel->left, previousLabel->right);
                        break;
                }
            }
            if (!alreadyAssigned) {
                vecOfConstLabels.push_back(constant);
                newVecCasses.push_back(currentExpression);
            }
        // Current label is range
        } else if (expression->is<IR::Range>()) {
            const IR::Range* range = expression->to<IR::Range>();
            auto leftConstant = range->left->to<IR::Constant>();
            auto rightConstant = range->right->to<IR::Constant>();

            if (leftConstant && rightConstant) {  // If labels of range are constants
                auto left = leftConstant->value;
                auto right = rightConstant->value;

                bool repeatedRange = false;  // Define if range overlaping with previous range

                // Go through all previous range labels and check
                // if some of them are overlaping with current range
                for (const IR::Range* previousLabel : vecOfRangeLabels) {
                    auto leftPreviousConstant = previousLabel->left->to<IR::Constant>();
                    auto rightPreviousConstant = previousLabel->right->to<IR::Constant>();

                    if ((left == leftPreviousConstant->value) &&
                        (right == rightPreviousConstant->value)) {
                        repeatedRange = true;
                        ::warning(ErrorType::WARN_DUPLICATED,
                                "Range label %1% .. %2% is the same as previous range %3% .. %4%."
                                " Optimized code will not contain this label.",
                                range->left, range->right, previousLabel->left,
                                previousLabel->right);
                    } else if (leftPreviousConstant->value <= left &&
                                             rightPreviousConstant->value >= right) {
                        repeatedRange = true;
                        ::warning(ErrorType::WARN_DUPLICATED,
                         "Range label %1% .. %2% is already assigned inside previous"
                         " range %3% .. %4%. Optimized code will not contain this label.",
                         range->left, range->right, previousLabel->left, previousLabel->right);
                        break;
                    }
                }
                if (!repeatedRange) {
                    vecOfRangeLabels.push_back(range);
                    newVecCasses.push_back(currentExpression);
                }
            } else {
                newVecCasses.push_back(currentExpression);
            }
        } else {
            newVecCasses.push_back(currentExpression);
        }
    }
    // Create new SelectExpression with all new SelectCasses
    IR::SelectExpression* optimizedSelectExpression =
                        new IR::SelectExpression(selectExpression->select, newVecCasses);

    return optimizedSelectExpression;
}

}  // namespace P4
