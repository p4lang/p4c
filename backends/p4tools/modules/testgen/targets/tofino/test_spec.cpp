/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/* =========================================================================================
 *  IndexExpression
 * ========================================================================================= */

IndexExpression::IndexExpression(const IR::Expression *index, const IR::Expression *value)
    : index(index), value(value) {}

const IR::Constant *IndexExpression::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *IndexExpression::getEvaluatedIndex() const {
    const auto *constant = index->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Expression *IndexExpression::getIndex() const { return index; }

const IR::Expression *IndexExpression::getValue() const { return value; }

cstring IndexExpression::getObjectName() const { return "IndexExpression"_cs; }

const IndexExpression *IndexExpression::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedIndex = model.evaluate(index, doComplete);
    const auto *evaluatedValue = model.evaluate(value, doComplete);
    return new IndexExpression(evaluatedIndex, evaluatedValue);
}

std::map<big_int, std::pair<int, const IR::Constant *>> IndexMap::unravelMap() const {
    std::map<big_int, std::pair<int, const IR::Constant *>> valueMap;
    for (auto it = indexConditions.rbegin(); it != indexConditions.rend(); ++it) {
        const auto *storedIndex = it->getEvaluatedIndex();
        const auto *storedVal = it->getEvaluatedValue();
        // Important, if the element already exists in the map, ignore it.
        // That index has been overwritten.
        valueMap.insert({storedIndex->value, {storedIndex->type->width_bits(), storedVal}});
    }
    return valueMap;
}

/* =========================================================================================
 *  IndexMap
 * ========================================================================================= */

IndexMap::IndexMap(const IR::Expression *initialValue) : initialValue(initialValue) {}

void IndexMap::writeToIndex(const IR::Expression *index, const IR::Expression *value) {
    indexConditions.emplace_back(index, value);
}

const IR::Expression *IndexMap::getInitialValue() const { return initialValue; }

const IR::Expression *IndexMap::getValueAtIndex(const IR::Expression *index) const {
    const IR::Expression *baseExpr = initialValue;
    for (const auto &indexMap : indexConditions) {
        const auto *storedIndex = indexMap.getIndex();
        const auto *storedVal = indexMap.getValue();
        baseExpr =
            new IR::Mux(baseExpr->type, new IR::Equ(storedIndex, index), storedVal, baseExpr);
    }
    return baseExpr;
}

const IR::Expression *IndexMap::getEvaluatedInitialValue() const {
    if (const auto *constant = initialValue->to<IR::Constant>()) {
        return constant;
    }
    if (const auto *initalListExprValue = initialValue->to<IR::StructExpression>()) {
        // TODO: Validate that this is a list expression of constants.
        return initalListExprValue;
    }
    BUG("Variable is not a constant or a list expression, has the test object %1% been evaluated?",
        getObjectName());
}

/* =========================================================================================
 *  TofinoRegisterValue
 * ========================================================================================= */

TofinoRegisterValue::TofinoRegisterValue(const IR::Declaration_Instance *decl,
                                         const IR::Expression *initialValue,
                                         const IR::Expression *initialIndex)
    : IndexMap(initialValue), decl(decl), initialIndex(initialIndex) {}

cstring TofinoRegisterValue::getObjectName() const { return "TofinoRegisterValue"_cs; }

const IR::Declaration_Instance *TofinoRegisterValue::getRegisterDeclaration() const { return decl; }

const TofinoRegisterValue *TofinoRegisterValue::evaluate(const Model &model,
                                                         bool doComplete) const {
    const IR::Expression *evaluatedValue = nullptr;
    if (const auto *initalListExprValue = initialValue->to<IR::StructExpression>()) {
        evaluatedValue = model.evaluateStructExpr(initalListExprValue, doComplete);
    } else {
        evaluatedValue = model.evaluate(initialValue, doComplete);
    }
    const auto *evaluatedInitialIndex = model.evaluate(initialIndex, doComplete);
    return new TofinoRegisterValue(decl, evaluatedValue, evaluatedInitialIndex);
}

const IR::Constant *TofinoRegisterValue::getEvaluatedInitialIndex() const {
    const auto *constant = initialIndex->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

/* =========================================================================================
 *  TofinoDirectRegisterValue
 * ========================================================================================= */

TofinoDirectRegisterValue::TofinoDirectRegisterValue(const IR::Declaration_Instance *decl,
                                                     const IR::Expression *initialValue,
                                                     const IR::P4Table *table)
    : initialValue(initialValue), decl(decl), table(table) {}

cstring TofinoDirectRegisterValue::getObjectName() const { return "TofinoDirectRegisterValue"_cs; }

const IR::Declaration_Instance *TofinoDirectRegisterValue::getRegisterDeclaration() const {
    return decl;
}

const IR::P4Table *TofinoDirectRegisterValue::getRegisterTable() const { return table; }

const TofinoDirectRegisterValue *TofinoDirectRegisterValue::evaluate(const Model &model,
                                                                     bool doComplete) const {
    const IR::Expression *evaluatedValue = nullptr;
    if (const auto *initalListExprValue = initialValue->to<IR::StructExpression>()) {
        evaluatedValue = model.evaluateStructExpr(initalListExprValue, doComplete);
    } else {
        evaluatedValue = model.evaluate(initialValue, doComplete);
    }
    return new TofinoDirectRegisterValue(decl, evaluatedValue, table);
}

const IR::Expression *TofinoDirectRegisterValue::getInitialValue() const { return initialValue; }

const IR::Expression *TofinoDirectRegisterValue::getEvaluatedInitialValue() const {
    if (const auto *constant = initialValue->to<IR::Constant>()) {
        return constant;
    }
    if (const auto *initalListExprValue = initialValue->to<IR::StructExpression>()) {
        // TODO: Validate that this is a list expression of constants.
        return initalListExprValue;
    }
    BUG("Variable is not a constant or a list expression, has the test object %1% been evaluated?",
        getObjectName());
}

/* =========================================================================================
 *  TofinoRegisterParam
 * ========================================================================================= */

TofinoRegisterParam::TofinoRegisterParam(const IR::Declaration_Instance *decl,
                                         const IR::Expression *initialValue)
    : decl(decl), initialValue(initialValue) {}

cstring TofinoRegisterParam::getObjectName() const { return "TofinoRegisterParam"_cs; }

const TofinoRegisterParam *TofinoRegisterParam::evaluate(const Model &model,
                                                         bool doComplete) const {
    return new TofinoRegisterParam(decl, model.evaluate(initialValue, doComplete));
}

const IR::Declaration_Instance *TofinoRegisterParam::getRegisterParamDeclaration() const {
    return decl;
}

const IR::Expression *TofinoRegisterParam::getInitialValue() const { return initialValue; }

const IR::Constant *TofinoRegisterParam::getEvaluatedInitialValue() const {
    const auto *constant = initialValue->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

/* =========================================================================================
 *  TofinoActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>> *TofinoActionProfile::getActions()
    const {
    return &actions;
}

TofinoActionProfile::TofinoActionProfile(const IR::IDeclaration *profileDecl)
    : profileDecl(profileDecl) {}

cstring TofinoActionProfile::getObjectName() const { return profileDecl->controlPlaneName(); }

const IR::IDeclaration *TofinoActionProfile::getProfileDecl() const { return profileDecl; }

void TofinoActionProfile::addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs) {
    actions.emplace_back(actionName, actionArgs);
}
size_t TofinoActionProfile::getActionMapSize() const { return actions.size(); }

const TofinoActionProfile *TofinoActionProfile::evaluate(const Model &model,
                                                         bool doComplete) const {
    auto *profile = new TofinoActionProfile(profileDecl);
    for (const auto &actionTuple : actions) {
        auto actionArgs = actionTuple.second;
        std::vector<ActionArg> evaluatedArgs;
        evaluatedArgs.reserve(actionArgs.size());
        for (const auto &actionArg : actionArgs) {
            evaluatedArgs.emplace_back(*actionArg.evaluate(model, doComplete));
        }
        profile->addToActionMap(actionTuple.first, evaluatedArgs);
    }
    return profile;
}

/* =========================================================================================
 *  TofinoActionSelector
 * ========================================================================================= */

cstring TofinoActionSelector::getObjectName() const { return "TofinoActionSelector"_cs; }

const IR::IDeclaration *TofinoActionSelector::getSelectorDecl() const { return selectorDecl; }

const TofinoActionProfile *TofinoActionSelector::getActionProfile() const { return actionProfile; }

TofinoActionSelector::TofinoActionSelector(const IR::IDeclaration *selectorDecl,
                                           const TofinoActionProfile *actionProfile)
    : selectorDecl(selectorDecl), actionProfile(actionProfile) {}

const TofinoActionSelector *TofinoActionSelector::evaluate(const Model &model,
                                                           bool doComplete) const {
    const auto *evaluatedProfile = actionProfile->evaluate(model, doComplete);
    return new TofinoActionSelector(selectorDecl, evaluatedProfile);
}

/* =========================================================================================
 * Table Key Match Types
 * ========================================================================================= */

Range::Range(const IR::KeyElement *key, const IR::Expression *low, const IR::Expression *high)
    : TableMatch(key), low(low), high(high) {}

const IR::Constant *Range::getEvaluatedLow() const {
    const auto *constant = low->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              low->type->node_type_name(), getObjectName());
    return constant;
}

const IR::Constant *Range::getEvaluatedHigh() const {
    const auto *constant = high->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              high->type->node_type_name(), getObjectName());
    return constant;
}

const Range *Range::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedLow = model.evaluate(low, doComplete);
    const auto *evaluatedHigh = model.evaluate(high, doComplete);
    return new Range(getKey(), evaluatedLow, evaluatedHigh);
}

cstring Range::getObjectName() const { return "Range"_cs; }

}  // namespace P4::P4Tools::P4Testgen::Tofino
