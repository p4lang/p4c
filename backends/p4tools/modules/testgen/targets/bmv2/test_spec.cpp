#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

#include "backends/p4tools/common/lib/model.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

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

cstring IndexExpression::getObjectName() const { return "IndexExpression"; }

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
 *  Bmv2Register
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

const IR::Constant *IndexMap::getEvaluatedInitialValue() const {
    const auto *constant = initialValue->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

/* =========================================================================================
 *  Bmv2V1ModelRegisterValue
 * ========================================================================================= */

Bmv2V1ModelRegisterValue::Bmv2V1ModelRegisterValue(const IR::Expression *initialValue)
    : IndexMap(initialValue) {}

cstring Bmv2V1ModelRegisterValue::getObjectName() const { return "Bmv2V1ModelRegisterValue"; }

const Bmv2V1ModelRegisterValue *Bmv2V1ModelRegisterValue::evaluate(const Model &model,
                                                                   bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(getInitialValue(), doComplete);
    auto *evaluatedRegisterValue = new Bmv2V1ModelRegisterValue(evaluatedValue);
    for (const auto &cond : indexConditions) {
        const auto *evaluatedCond = cond.evaluate(model, doComplete);
        evaluatedRegisterValue->writeToIndex(evaluatedCond->getEvaluatedIndex(),
                                             evaluatedCond->getEvaluatedValue());
    }
    return evaluatedRegisterValue;
}

/* =========================================================================================
 *  Bmv2V1ModelMeterValue
 * ========================================================================================= */

Bmv2V1ModelMeterValue::Bmv2V1ModelMeterValue(const IR::Expression *initialValue, bool isDirect)
    : IndexMap(initialValue), isDirect(isDirect) {}

cstring Bmv2V1ModelMeterValue::getObjectName() const { return "Bmv2V1ModelMeterValue"; }

const Bmv2V1ModelMeterValue *Bmv2V1ModelMeterValue::evaluate(const Model &model,
                                                             bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(getInitialValue(), doComplete);
    auto *evaluatedMeterValue = new Bmv2V1ModelMeterValue(evaluatedValue, isDirect);
    for (const auto &cond : indexConditions) {
        const auto *evaluatedCond = cond.evaluate(model, doComplete);
        evaluatedMeterValue->writeToIndex(evaluatedCond->getEvaluatedIndex(),
                                          evaluatedCond->getEvaluatedValue());
    }
    return evaluatedMeterValue;
}

bool Bmv2V1ModelMeterValue::isDirectMeter() const { return isDirect; }

/* =========================================================================================
 *  Bmv2V1ModelActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>> *
Bmv2V1ModelActionProfile::getActions() const {
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

const Bmv2V1ModelActionProfile *Bmv2V1ModelActionProfile::evaluate(const Model &model,
                                                                   bool doComplete) const {
    auto *profile = new Bmv2V1ModelActionProfile(profileDecl);
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

const Bmv2V1ModelActionSelector *Bmv2V1ModelActionSelector::evaluate(const Model &model,
                                                                     bool doComplete) const {
    const auto *evaluatedProfile = actionProfile->evaluate(model, doComplete);
    return new Bmv2V1ModelActionSelector(selectorDecl, evaluatedProfile);
}

/* =========================================================================================
 *  Bmv2V1ModelCloneInfo
 * ========================================================================================= */

Bmv2V1ModelCloneInfo::Bmv2V1ModelCloneInfo(const IR::Expression *sessionId,
                                           BMv2Constants::CloneType cloneType,
                                           const ExecutionState &clonedState,
                                           std::optional<int> preserveIndex)
    : sessionId(sessionId),
      cloneType(cloneType),
      clonedState(clonedState),
      preserveIndex(preserveIndex) {}

cstring Bmv2V1ModelCloneInfo::getObjectName() const { return "Bmv2V1ModelCloneInfo"; }

BMv2Constants::CloneType Bmv2V1ModelCloneInfo::getCloneType() const { return cloneType; }

const IR::Expression *Bmv2V1ModelCloneInfo::getSessionId() const { return sessionId; }

const ExecutionState &Bmv2V1ModelCloneInfo::getClonedState() const { return clonedState; }

std::optional<int> Bmv2V1ModelCloneInfo::getPreserveIndex() const { return preserveIndex; }

const Bmv2V1ModelCloneInfo *Bmv2V1ModelCloneInfo::evaluate(const Model & /*model*/,
                                                           bool /*doComplete*/) const {
    P4C_UNIMPLEMENTED("Evaluate is not implemented for this test object.");
}

/* =========================================================================================
 *  Bmv2V1ModelCloneSpec
 * ========================================================================================= */

Bmv2V1ModelCloneSpec::Bmv2V1ModelCloneSpec(const IR::Expression *sessionId,
                                           const IR::Expression *clonePort, bool isClone)
    : sessionId(sessionId), clonePort(clonePort), isClone(isClone) {}

cstring Bmv2V1ModelCloneSpec::getObjectName() const { return "Bmv2V1ModelCloneSpec"; }

const IR::Expression *Bmv2V1ModelCloneSpec::getClonePort() const { return clonePort; }

const IR::Expression *Bmv2V1ModelCloneSpec::getSessionId() const { return sessionId; }

const IR::Constant *Bmv2V1ModelCloneSpec::getEvaluatedClonePort() const {
    const auto *constant = clonePort->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *Bmv2V1ModelCloneSpec::getEvaluatedSessionId() const {
    const auto *constant = sessionId->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const Bmv2V1ModelCloneSpec *Bmv2V1ModelCloneSpec::evaluate(const Model &model,
                                                           bool doComplete) const {
    return new Bmv2V1ModelCloneSpec(model.evaluate(sessionId, doComplete),
                                    model.evaluate(clonePort, doComplete), isClone);
}

bool Bmv2V1ModelCloneSpec::isClonedPacket() const { return isClone; }

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

const Optional *Optional::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(value, doComplete);
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

const Range *Range::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedLow = model.evaluate(low, doComplete);
    const auto *evaluatedHigh = model.evaluate(high, doComplete);
    return new Range(getKey(), evaluatedLow, evaluatedHigh);
}

/* =========================================================================================
 *  MetadataCollection
 * ========================================================================================= */

MetadataCollection::MetadataCollection() = default;

cstring MetadataCollection::getObjectName() const { return "MetadataCollection"; }

const MetadataCollection *MetadataCollection::evaluate(const Model & /*model*/,
                                                       bool /*finalModel*/) const {
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
