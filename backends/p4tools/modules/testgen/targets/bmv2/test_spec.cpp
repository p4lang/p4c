#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

#include "backends/p4tools/common/lib/model.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

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
 *  Bmv2V1ModelActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>>
    *Bmv2V1ModelActionProfile::getActions() const {
    return &actions;
}

Bmv2V1ModelActionProfile::Bmv2V1ModelActionProfile(const IR::IDeclaration *profileDecl)
    : profileDecl(profileDecl) {}

cstring Bmv2V1ModelActionProfile::getObjectName() const { return profileDecl->controlPlaneName(); }

const IR::IDeclaration *Bmv2V1ModelActionProfile::getProfileDecl() const { return profileDecl; }

void Bmv2V1ModelActionProfile::addToActionMap(cstring actionName,
                                              std::vector<ActionArg> actionArgs) {
    actions.emplace_back(actionName, actionArgs);
}
size_t Bmv2V1ModelActionProfile::getActionMapSize() const { return actions.size(); }

const Bmv2V1ModelActionProfile *Bmv2V1ModelActionProfile::evaluate(const Model &model) const {
    auto *profile = new Bmv2V1ModelActionProfile(profileDecl);
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
 *  Bmv2V1ModelActionSelector
 * ========================================================================================= */

Bmv2V1ModelActionSelector::Bmv2V1ModelActionSelector(const IR::IDeclaration *selectorDecl,
                                                     const Bmv2V1ModelActionProfile *actionProfile)
    : selectorDecl(selectorDecl), actionProfile(actionProfile) {}

cstring Bmv2V1ModelActionSelector::getObjectName() const { return "Bmv2V1ModelActionSelector"; }

const IR::IDeclaration *Bmv2V1ModelActionSelector::getSelectorDecl() const { return selectorDecl; }

const Bmv2V1ModelActionProfile *Bmv2V1ModelActionSelector::getActionProfile() const {
    return actionProfile;
}

const Bmv2V1ModelActionSelector *Bmv2V1ModelActionSelector::evaluate(const Model &model) const {
    const auto *evaluatedProfile = actionProfile->evaluate(model);
    return new Bmv2V1ModelActionSelector(selectorDecl, evaluatedProfile);
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

/* =========================================================================================
 * Table Key Match Types
 * ========================================================================================= */

Optional::Optional(const IR::KeyElement *key, const IR::Expression *val, bool addMatch)
    : TableMatch(key), value(val), addMatch(addMatch) {}

const IR::Constant *Optional::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              value->type->node_type_name(), getObjectName());
    return constant;
}

const Optional *Optional::evaluate(const Model &model) const {
    const auto *evaluatedValue = model.evaluate(value);
    return new Optional(getKey(), evaluatedValue, addMatch);
}

cstring Optional::getObjectName() const { return "Optional"; }

bool Optional::addAsExactMatch() const { return addMatch; }

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

const Range *Range::evaluate(const Model &model) const {
    const auto *evaluatedLow = model.evaluate(low);
    const auto *evaluatedHigh = model.evaluate(high);
    return new Range(getKey(), evaluatedLow, evaluatedHigh);
}

/* =========================================================================================
 *  MetadataCollection
 * ========================================================================================= */

MetadataCollection::MetadataCollection() = default;

cstring MetadataCollection::getObjectName() const { return "MetadataCollection"; }

const MetadataCollection *MetadataCollection::evaluate(const Model & /*model*/) const {
    P4C_UNIMPLEMENTED("%1% has no implementation for \"evaluate\".", getObjectName());
}

const std::map<cstring, const IR::Literal *> &MetadataCollection::getMetadataFields() const {
    return metadataFields;
}

const IR::Literal *MetadataCollection::getMetadataField(cstring name) {
    return metadataFields.at(name);
}

void MetadataCollection::addMetaDataField(cstring name, const IR::Literal *metadataField) {
    metadataFields[name] = metadataField;
}

cstring Range::getObjectName() const { return "Range"; }

}  // namespace P4Tools::P4Testgen::Bmv2
