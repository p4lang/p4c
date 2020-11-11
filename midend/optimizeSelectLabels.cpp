#include "optimizeSelectLabels.h"

namespace P4 {

big_int DoOptimizeSelectLabels::getValidRange(std::pair<big_int, big_int> range) {
    // This function checks validity of range.
    // If std::pair range have both same values then given range represents constant value,
    // if first value is biger than second, range is not valid,
    // In other cases range is valid
    // Returns -1 if second argument is lower than first, because then range is not valid
    // Returns 0 if range is valid
    // If first and second range arguments are equal then this range represents
    // constant and first value will be return
    if (range.first == range.second)
        return range.first;
    else if (range.first > range.second)
        return -1;
    else
        return 0;
}

IR::Range* DoOptimizeSelectLabels::getRange(SelectLabel* range) {
    const IR::Expression* left = new IR::Constant(range->rangeConstantTypeInfo,
                                                        range->rangeValue.first, range->base);
    const IR::Expression* right = new IR::Constant(range->rangeConstantTypeInfo,
                                                        range->rangeValue.second, range->base);
    return new IR::Range(range->typeInfo, left, right);
}

SelectLabel* DoOptimizeSelectLabels::insertValues(SelectLabel* root, big_int value) {
    SelectLabel* mainRoot = root;
    SelectLabel* leafNode = nullptr;

    // Search leaf node where value will be inserted
    while (mainRoot != nullptr) {
        leafNode = mainRoot;
        if (mainRoot->type == SelectLabel::RangeLabel) {
            if ((value >= mainRoot->rangeValue.first) && (value <= mainRoot->rangeValue.second)) {
                if (mainRoot->left != nullptr) {
                    if ((mainRoot->left->type == SelectLabel::RangeLabel)) {
                        if ((value >= mainRoot->left->rangeValue.first) &&
                                (value <= mainRoot->left->rangeValue.second)) {
                             mainRoot = mainRoot->left;
                        } else {
                             mainRoot = mainRoot->right;
                        }
                    } else {
                        return nullptr;
                    }
                } else {
                    mainRoot = mainRoot->left;
                }
            } else {
                return nullptr;
            }
        }
    }

    // Insert new value in tree
    // Const value is the same as left side of range
    if (value == leafNode->rangeValue.first) {
        std::pair<big_int, big_int> newSubRange;
        newSubRange.first = value + 1;
        newSubRange.second = leafNode->rangeValue.second;

        big_int validRange = getValidRange(newSubRange);
        if (validRange != -1) {
            if (validRange != 0)
                leafNode->left = new SelectLabel(validRange, root->base,
                                                 root->rangeConstantTypeInfo);
            else
                leafNode->left = new SelectLabel(newSubRange, root->base, root->typeInfo,
                                                 root->rangeConstantTypeInfo);
        }
      // Const value is the same as right side of range
    } else if (value == leafNode->rangeValue.second) {
        std::pair<big_int, big_int> newSubRange;
        newSubRange.first = leafNode->rangeValue.first;
        newSubRange.second = value - 1;

        big_int validRange = getValidRange(newSubRange);
        if (validRange != -1) {
            if (validRange != 0)
                leafNode->left = new SelectLabel(validRange, root->base,
                                                 root->rangeConstantTypeInfo);
            else
                leafNode->left = new SelectLabel(newSubRange, root->base, root->typeInfo,
                                                 root->rangeConstantTypeInfo);
        }
      // Const value is inside range
    } else if ((value > leafNode->rangeValue.first) && (value < leafNode->rangeValue.second)) {
        std::pair<big_int, big_int> leftSubRange, rightSubRange;
        leftSubRange.first = leafNode->rangeValue.first;
        leftSubRange.second = value - 1;

        rightSubRange.first = value + 1;
        rightSubRange.second = leafNode->rangeValue.second;

        big_int leftValidRange = getValidRange(leftSubRange);
        big_int rightValidRange = getValidRange(rightSubRange);

        if (leftValidRange != -1) {
            if (leftValidRange != 0)
                leafNode->left = new SelectLabel(leftValidRange, root->base,
                                                 root->rangeConstantTypeInfo);
            else
                leafNode->left = new SelectLabel(leftSubRange, root->base,
                                                 root->typeInfo, root->rangeConstantTypeInfo);
        }

        if (rightValidRange != -1) {
            if (rightValidRange != 0)
                leafNode->right = new SelectLabel(rightValidRange, root->base,
                                                 root->rangeConstantTypeInfo);
            else
                leafNode->right = new SelectLabel(rightSubRange, root->base,
                                                  root->typeInfo, root->rangeConstantTypeInfo);
        }
    }
    return leafNode;
}

SelectLabel* DoOptimizeSelectLabels::insertValues(SelectLabel* root,
                                                            std::pair<big_int, big_int> value) {
    SelectLabel* mainRoot = root;
    SelectLabel* leafNode = nullptr;

    // Search leaf node where value will be inserted
    while (mainRoot != nullptr) {
        leafNode = mainRoot;
        if (mainRoot->type == SelectLabel::RangeLabel) {
            // Overlaping two ranges on left side (eg. 1..6 and 4..9 -> 5..9)
            if ((value.first < mainRoot->rangeValue.first) &&
                    ((value.second >= mainRoot->rangeValue.first) &&
                     (value.second < mainRoot->rangeValue.second))) {
                mainRoot = mainRoot->left;
            // Overlaping two ranges on right side (eg. 4..9 and 1..6 -> 1..3)
            } else if ((value.second > mainRoot->rangeValue.second) &&
                            ((value.first > mainRoot->rangeValue.first) &&
                                (value.first <= mainRoot->rangeValue.second))) {
                if (mainRoot->left != nullptr) {
                    if (mainRoot->left->type == SelectLabel::RangeLabel) {
                        if (((value.first >= mainRoot->left->rangeValue.first) &&
                                    (value.first <= mainRoot->left->rangeValue.second)) ||
                                        ((value.second >= mainRoot->left->rangeValue.first) &&
                                            (value.second <= mainRoot->left->rangeValue.second)))
                            mainRoot = mainRoot->left;
                        else
                            mainRoot = mainRoot->right;
                    } else {
                        return nullptr;
                    }
                } else {
                    mainRoot = mainRoot->right;
                }
            // All others overlapings of two ranges
            } else if ((value.first >= mainRoot->rangeValue.first) &&
                      (value.second <= mainRoot->rangeValue.second)) {
                if (mainRoot->left != nullptr) {
                    if (mainRoot->left->type == SelectLabel::RangeLabel) {
                        if (mainRoot->left->rangeValue.second < value.first)
                            mainRoot = mainRoot->right;
                        else
                            mainRoot = mainRoot->left;
                    } else {
                        return nullptr;
                    }
                } else {
                    mainRoot = mainRoot->left;
                }
            } else {
                return nullptr;
            }
        }
    }

    // Insert new value in tree
    if (leafNode == nullptr) {
        leafNode = new SelectLabel(value, root->base, root->typeInfo, root->rangeConstantTypeInfo);
    } else if ((value.first < leafNode->rangeValue.first) &&
                        ((value.second >= leafNode->rangeValue.first) &&
                                (value.second < leafNode->rangeValue.second))) {
        std::pair<big_int, big_int> newSubRange;
        newSubRange.first = value.second + 1;
        newSubRange.second = leafNode->rangeValue.second;

        if (newSubRange.second == newSubRange.first)
            leafNode->left = new SelectLabel(newSubRange.first, root->base,
                                             root->rangeConstantTypeInfo);
        else
            leafNode->left = new SelectLabel(newSubRange, root->base,
                                             root->typeInfo, root->rangeConstantTypeInfo);
    } else if ((value.second > leafNode->rangeValue.second) &&
                    ((value.first > leafNode->rangeValue.first) &&
                     (value.first <= leafNode->rangeValue.second))) {
        std::pair<big_int, big_int> newSubRange;
        newSubRange.first = leafNode->rangeValue.first;
        newSubRange.second = value.first - 1;
        if (newSubRange.second == newSubRange.first) {
            leafNode->left = new SelectLabel(newSubRange.first, root->base,
                                             root->rangeConstantTypeInfo);
        } else {
            leafNode->left = new SelectLabel(newSubRange, root->base,
                                             root->typeInfo, root->rangeConstantTypeInfo);
        }
    } else if ((value.first >= leafNode->rangeValue.first) &&
              (value.second <= leafNode->rangeValue.second)) {
        std::pair<big_int, big_int> leftSubRange(0, 0), rightSubRange(0, 0);

        leftSubRange.first = leafNode->rangeValue.first;
        leftSubRange.second = value.first - 1;

        rightSubRange.first = value.second + 1;
        rightSubRange.second = leafNode->rangeValue.second;

        big_int validLeft = getValidRange(leftSubRange);
        big_int validRight = getValidRange(rightSubRange);

        if (validLeft != -1) {
            if (validLeft == 0)
                leafNode->left = new SelectLabel(leftSubRange, root->base,
                                                 root->typeInfo, root->rangeConstantTypeInfo);
            else
                leafNode->left = new SelectLabel(validLeft, root->base,
                                                 root->rangeConstantTypeInfo);
        }

        if (validRight != -1) {
            if (validRight == 0)
                leafNode->right = new SelectLabel(rightSubRange, root->base,
                                                  root->typeInfo, root->rangeConstantTypeInfo);
            else
                leafNode->right = new SelectLabel(validRight, root->base,
                                                  root->rangeConstantTypeInfo);
        }
    }

    return leafNode;
}

void DoOptimizeSelectLabels::getNewSelectCases(std::vector<SelectLabel*> &elementsOfRange,
                                              IR::Vector<IR::SelectCase> &newVecCasses,
                                              std::vector<SelectLabel*> &vecOfLabels,
                                              const IR::SelectCase* currentExpression) {
    if (elementsOfRange.size() < 1) {
        return;
    }
    bool elementAlreadyAssigned;

    for (auto element : elementsOfRange) {
        elementAlreadyAssigned = false;
        // Check if this element is already assigned among previous labels
        for (SelectLabel* previousElement : vecOfLabels) {
            if ((previousElement->type == SelectLabel::RangeLabel) &&
                        element->type == SelectLabel::RangeLabel) {
                if (previousElement->rangeValue.first == element->rangeValue.first &&
                    previousElement->rangeValue.second == element->rangeValue.second) {
                    elementAlreadyAssigned = true;
                    break;
                }
            } else {
                if (previousElement->constValue == element->constValue) {
                    elementAlreadyAssigned = true;
                    break;
                }
            }
        }

        if (!elementAlreadyAssigned) {
            vecOfLabels.push_back(element);
            // Tree node contains new constant label
            if (element->type == SelectLabel::ConstantLabel) {
                IR::Constant* value = new IR::Constant(element->typeInfo,
                                                        element->constValue, element->base);
                IR::SelectCase* newSelectCase =
                                            new IR::SelectCase(value, currentExpression->state);

                 // Put new element in vector of all optimized select cases
                newVecCasses.push_back(newSelectCase);

            // Tree node contains new range label
            } else {
                // Create new select case
                IR::Range* newLabel = getRange(element);
                IR::SelectCase* newSelectCase =
                                    new IR::SelectCase(newLabel, currentExpression->state);

                // Put new element in vector of all optimized select cases
                newVecCasses.push_back(newSelectCase);
            }
        }
    }
}

void DoOptimizeSelectLabels::getElementsOfParsedRange(SelectLabel* root,
                                                      std::vector<SelectLabel*> &elementsOfRange) {
    if (!root)
        return;

    // If node is leaf node, put in vector of leafs
    if (!root->left && !root->right) {
        elementsOfRange.push_back(root);
        return;
    }
    // If exists, go recursively through left child
    if (root->left)
       getElementsOfParsedRange(root->left, elementsOfRange);
    // If exists, go recursively through right child
    if (root->right)
       getElementsOfParsedRange(root->right, elementsOfRange);
}

const IR::Node* DoOptimizeSelectLabels::postorder(IR::SelectExpression* selectExpression) {
    auto vecCases = selectExpression->selectCases;
    std::vector<SelectLabel*> vecOfLabels;
    // Vector that will contain optimized select casses
    IR::Vector<IR::SelectCase> newVecCasses;

    for (auto currentExpression : vecCases) {
        auto expression = currentExpression->keyset;
        // Current label is constant
        if (expression->is<IR::Constant>()) {
            auto constant = expression->to<IR::Constant>();
            auto value = constant->value;
            bool alreadyAssigned = false;

            SelectLabel* label = new SelectLabel(value, constant->base, constant->type);
            for (SelectLabel* previousLabel : vecOfLabels) {
                if (previousLabel->type == SelectLabel::ConstantLabel) {
                    big_int constPrevious = previousLabel->constValue;
                    // Check if same label already exists
                    if (value == constPrevious) {
                        alreadyAssigned = true;
                        ::warning(ErrorType::WARN_OVERLAPING, "Label %1% is already assigned."
                            " Optimized code will not contain this label.", constant);
                        break;
                    }
                // Check if label is assigned inside some previous range
                } else {
                    if ((value >= previousLabel->rangeValue.first) &&
                        (value <= previousLabel->rangeValue.second)) {
                        alreadyAssigned = true;
                        IR::Range* previousRange = getRange(previousLabel);
                        ::warning(ErrorType::WARN_OVERLAPING,
                                "Label value %1% is already assigned inside range:"
                                " %2% .. %3%. Optimized code will not contain this label.",
                                constant, previousRange->left, previousRange->right);
                        break;
                    }
                }
            }
            if (!alreadyAssigned) {
                vecOfLabels.push_back(label);
                newVecCasses.push_back(currentExpression);
            }
        // Current label is range
        } else if (expression->is<IR::Range>()) {
            std::vector<SelectLabel*> elementsOfRange;
            auto range = expression->to<IR::Range>();
            auto leftConstant = range->left->to<IR::Constant>();
            auto rightConstant = range->right->to<IR::Constant>();

            if (leftConstant && rightConstant) {  // If labels of range are constants
                auto left = leftConstant->value;
                auto right = rightConstant->value;

                std::pair<big_int, big_int> rangeValues(left, right);
                SelectLabel* treeRoot = new SelectLabel(rangeValues, leftConstant->base,
                                                        range->type, leftConstant->type);
                bool insideRange = false;  // Define if range overlaping with previous label

                // Go through all previous labels and check
                // if some of them are overlaping with current range
                for (SelectLabel* previousLabel : vecOfLabels) {
                    // Constant value label
                    if (previousLabel->type == SelectLabel::ConstantLabel) {
                        if ((previousLabel->constValue >= rangeValues.first) &&
                           (previousLabel->constValue <= rangeValues.second)) {
                            IR::Constant* previousConstant =
                               new IR::Constant(previousLabel->constValue, previousLabel->base);
                            ::warning(ErrorType::WARN_OVERLAPING,
                                "Element of range label %1% .. %2% is already assigned "
                                "by constant label %3%. Optimization will parse this range.",
                                range->left, range->right, previousConstant);
                            insertValues(treeRoot, previousLabel->constValue);
                            insideRange = true;
                        }
                    } else {
                        if ((rangeValues.first >= previousLabel->rangeValue.first &&
                                        rangeValues.first <= previousLabel->rangeValue.second) ||
                                  (rangeValues.second >= previousLabel->rangeValue.first &&
                                        rangeValues.second <= previousLabel->rangeValue.second) ||
                                  (rangeValues.first <= previousLabel->rangeValue.first &&
                                        rangeValues.second >= previousLabel->rangeValue.second)) {
                            insideRange = true;
                            IR::Range* previousRange = getRange(previousLabel);
                            ::warning(ErrorType::WARN_OVERLAPING,
                             "Range label %1% .. %2% is overlaping with range %3% .. %4%."
                             " Optimization will parse this range.",
                             range->left, range->right, previousRange->left, previousRange->right);
                            insertValues(treeRoot, previousLabel->rangeValue);
                        }
                    }
                }

                if (insideRange) {
                    getElementsOfParsedRange(treeRoot, elementsOfRange);
                    getNewSelectCases(elementsOfRange, newVecCasses, vecOfLabels,
                                                                        currentExpression);
                } else {
                    vecOfLabels.push_back(treeRoot);
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
