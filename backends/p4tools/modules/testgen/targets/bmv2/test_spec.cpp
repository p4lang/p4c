#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

#include "backends/p4tools/common/lib/model.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/* =========================================================================================
 *  Bmv2Register
 * ========================================================================================= */

Bmv2RegisterValue::Bmv2RegisterValue(const IR::Expression *initialValue)
    : initialValue(initialValue) {}

void Bmv2RegisterValue::addRegisterCondition(Bmv2RegisterCondition cond) {
    registerConditions.push_back(cond);
}

const IR::Expression *Bmv2RegisterValue::getInitialValue() const { return initialValue; }

cstring Bmv2RegisterValue::getObjectName() const { return "Bmv2RegisterValue"; }

const IR::Expression *Bmv2RegisterValue::getCurrentValue(const IR::Expression *index) const {
    const IR::Expression *baseExpr = initialValue;
    for (const auto &bmv2registerValue : registerConditions) {
        const auto *storedIndex = bmv2registerValue.index;
        const auto *storedVal = bmv2registerValue.value;
        baseExpr =
            new IR::Mux(baseExpr->type, new IR::Equ(storedIndex, index), storedVal, baseExpr);
    }
    return baseExpr;
}

const IR::Constant *Bmv2RegisterValue::getEvaluatedValue() const {
    const auto *constant = initialValue->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const Bmv2RegisterValue *Bmv2RegisterValue::evaluate(const Model &model) const {
    const auto *evaluatedValue = model.evaluate(initialValue);
    auto *evaluatedRegisterValue = new Bmv2RegisterValue(evaluatedValue);
    const std::vector<ActionArg> evaluatedConditions;
    for (const auto &cond : registerConditions) {
        evaluatedRegisterValue->addRegisterCondition(*cond.evaluate(model));
    }
    return evaluatedRegisterValue;
}

Bmv2RegisterCondition::Bmv2RegisterCondition(const IR::Expression *index,
                                             const IR::Expression *value)
    : index(index), value(value) {}

const IR::Constant *Bmv2RegisterCondition::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *Bmv2RegisterCondition::getEvaluatedIndex() const {
    const auto *constant = index->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const Bmv2RegisterCondition *Bmv2RegisterCondition::evaluate(const Model &model) const {
    const auto *evaluatedIndex = model.evaluate(index);
    const auto *evaluatedValue = model.evaluate(value);
    return new Bmv2RegisterCondition(evaluatedIndex, evaluatedValue);
}

cstring Bmv2RegisterCondition::getObjectName() const { return "Bmv2RegisterCondition"; }

/* =========================================================================================
 *  Bmv2Counter
 * ========================================================================================= */

Bmv2CounterValue::Bmv2CounterValue(const IR::Expression *initialValue, const IR::Expression *size,
                                   CounterType type)
    : initialValue(initialValue), size(size), type(type) {}

void Bmv2CounterValue::addCounterCondition(Bmv2CounterCondition cond) {
    counterConditions.push_back(cond);
}

const IR::Expression *Bmv2CounterValue::getInitialValue() const { return initialValue; }

const IR::Constant *Bmv2CounterValue::getEvaluatedSize() const {
    const auto *constant = size->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const std::vector<Bmv2CounterCondition> Bmv2CounterValue::getCounterConditions() const {
    return counterConditions;
}

cstring Bmv2CounterValue::getObjectName() const { return "Bmv2CounterValue"; }

const IR::Expression *Bmv2CounterValue::getCurrentValue(const IR::Expression *index) const {
    const IR::Expression *baseExpr = initialValue;
    for (const auto &bmv2counterValue : counterConditions) {
        const auto *storedIndex = bmv2counterValue.index;
        const auto *storedVal = bmv2counterValue.value;
        baseExpr =
            new IR::Mux(baseExpr->type, new IR::Equ(storedIndex, index), storedVal, baseExpr);
    }
    return baseExpr;
}

const IR::Constant *Bmv2CounterValue::getEvaluatedValue() const {
    const auto *constant = initialValue->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

Bmv2CounterValue::CounterType Bmv2CounterValue::getCounterTypeByIndex(big_int index) {
    if (index == 0) return CounterType::PACKETS;
    if (index == 1) return CounterType::BYTES;
    if (index == 2) return CounterType::PACKETS_AND_BYTES;
    BUG("Unknown counter type %1%", type);
}

Bmv2CounterValue::CounterType Bmv2CounterValue::getType() const { return type; }

const Bmv2CounterValue *Bmv2CounterValue::evaluate(const Model &model) const {
    const auto *evaluatedValue = model.evaluate(initialValue);
    auto *evaluatedCounterValue = new Bmv2CounterValue(evaluatedValue, size, type);
    const std::vector<ActionArg> evaluatedConditions;
    for (const auto &cond : counterConditions) {
        evaluatedCounterValue->addCounterCondition(*cond.evaluate(model));
    }
    return evaluatedCounterValue;
}

Bmv2CounterCondition::Bmv2CounterCondition(const IR::Expression *index, const IR::Expression *value)
    : index(index), value(value) {}

const IR::Constant *Bmv2CounterCondition::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *Bmv2CounterCondition::getEvaluatedIndex() const {
    const auto *constant = index->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const Bmv2CounterCondition *Bmv2CounterCondition::evaluate(const Model &model) const {
    const auto *evaluatedIndex = model.evaluate(index);
    const auto *evaluatedValue = model.evaluate(value);
    return new Bmv2CounterCondition(evaluatedIndex, evaluatedValue);
}

cstring Bmv2CounterCondition::getObjectName() const { return "Bmv2CounterCondition"; }

/* =========================================================================================
 *  Bmv2_V1ModelActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>>
    *Bmv2_V1ModelActionProfile::getActions() const {
    return &actions;
}

Bmv2_V1ModelActionProfile::Bmv2_V1ModelActionProfile(const IR::IDeclaration *profileDecl)
    : profileDecl(profileDecl) {}

cstring Bmv2_V1ModelActionProfile::getObjectName() const { return profileDecl->controlPlaneName(); }

const IR::IDeclaration *Bmv2_V1ModelActionProfile::getProfileDecl() const { return profileDecl; }

void Bmv2_V1ModelActionProfile::addToActionMap(cstring actionName,
                                               std::vector<ActionArg> actionArgs) {
    actions.emplace_back(actionName, actionArgs);
}
size_t Bmv2_V1ModelActionProfile::getActionMapSize() const { return actions.size(); }

const Bmv2_V1ModelActionProfile *Bmv2_V1ModelActionProfile::evaluate(const Model &model) const {
    auto *profile = new Bmv2_V1ModelActionProfile(profileDecl);
    for (const auto &actionTuple : actions) {
        auto actionArgs = actionTuple.second;
        std::vector<ActionArg> evaluatedArgs;
        evaluatedArgs.reserve(actionArgs.size());
        for (const auto &actionArg : actionArgs) {
            evaluatedArgs.emplace_back(*actionArg.evaluate(model));
        }
        profile->addToActionMap(actionTuple.first, evaluatedArgs);
    }
    return profile;
}

/* =========================================================================================
 *  Bmv2_V1ModelActionSelector
 * ========================================================================================= */

Bmv2_V1ModelActionSelector::Bmv2_V1ModelActionSelector(
    const IR::IDeclaration *selectorDecl, const Bmv2_V1ModelActionProfile *actionProfile)
    : selectorDecl(selectorDecl), actionProfile(actionProfile) {}

cstring Bmv2_V1ModelActionSelector::getObjectName() const { return "Bmv2_V1ModelActionSelector"; }

const IR::IDeclaration *Bmv2_V1ModelActionSelector::getSelectorDecl() const { return selectorDecl; }

const Bmv2_V1ModelActionProfile *Bmv2_V1ModelActionSelector::getActionProfile() const {
    return actionProfile;
}

const Bmv2_V1ModelActionSelector *Bmv2_V1ModelActionSelector::evaluate(const Model &model) const {
    const auto *evaluatedProfile = actionProfile->evaluate(model);
    return new Bmv2_V1ModelActionSelector(selectorDecl, evaluatedProfile);
}

/* =========================================================================================
 *  Bmv2_CloneInfo
 * ========================================================================================= */

Bmv2_CloneInfo::Bmv2_CloneInfo(const IR::Expression *sessionId, const IR::Expression *clonePort,
                               bool isClone)
    : sessionId(sessionId), clonePort(clonePort), isClone(isClone) {}

cstring Bmv2_CloneInfo::getObjectName() const { return "Bmv2_CloneInfo"; }

const IR::Expression *Bmv2_CloneInfo::getClonePort() const { return clonePort; }

const IR::Expression *Bmv2_CloneInfo::getSessionId() const { return sessionId; }

const IR::Constant *Bmv2_CloneInfo::getEvaluatedClonePort() const {
    const auto *constant = clonePort->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *Bmv2_CloneInfo::getEvaluatedSessionId() const {
    const auto *constant = sessionId->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const Bmv2_CloneInfo *Bmv2_CloneInfo::evaluate(const Model &model) const {
    const auto *evaluatedClonePort = model.evaluate(clonePort);
    const auto *evaluatedSessionId = model.evaluate(sessionId);
    return new Bmv2_CloneInfo(evaluatedSessionId, evaluatedClonePort, isClone);
}

bool Bmv2_CloneInfo::isClonedPacket() const { return isClone; }

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
