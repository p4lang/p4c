#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

#include "backends/p4tools/common/lib/model.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

/* =========================================================================================
 *  PnaDpdkRegister
 * ========================================================================================= */

PnaDpdkRegisterValue::PnaDpdkRegisterValue(const IR::Expression *initialValue)
    : initialValue(initialValue) {}

void PnaDpdkRegisterValue::addRegisterCondition(PnaDpdkRegisterCondition cond) {
    registerConditions.push_back(cond);
}

const IR::Expression *PnaDpdkRegisterValue::getInitialValue() const { return initialValue; }

cstring PnaDpdkRegisterValue::getObjectName() const { return "PnaDpdkRegisterValue"; }

const IR::Expression *PnaDpdkRegisterValue::getCurrentValue(const IR::Expression *index) const {
    const IR::Expression *baseExpr = initialValue;
    for (const auto &pnaDpdkregisterValue : registerConditions) {
        const auto *storedIndex = pnaDpdkregisterValue.index;
        const auto *storedVal = pnaDpdkregisterValue.value;
        baseExpr =
            new IR::Mux(baseExpr->type, new IR::Equ(storedIndex, index), storedVal, baseExpr);
    }
    return baseExpr;
}

const IR::Constant *PnaDpdkRegisterValue::getEvaluatedValue() const {
    const auto *constant = initialValue->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const PnaDpdkRegisterValue *PnaDpdkRegisterValue::evaluate(const Model &model,
                                                           bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(initialValue, doComplete);
    auto *evaluatedRegisterValue = new PnaDpdkRegisterValue(evaluatedValue);
    const std::vector<ActionArg> evaluatedConditions;
    for (const auto &cond : registerConditions) {
        evaluatedRegisterValue->addRegisterCondition(*cond.evaluate(model, doComplete));
    }
    return evaluatedRegisterValue;
}

PnaDpdkRegisterCondition::PnaDpdkRegisterCondition(const IR::Expression *index,
                                                   const IR::Expression *value)
    : index(index), value(value) {}

const IR::Constant *PnaDpdkRegisterCondition::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const IR::Constant *PnaDpdkRegisterCondition::getEvaluatedIndex() const {
    const auto *constant = index->to<IR::Constant>();
    BUG_CHECK(constant, "Variable is not a constant, has the test object %1% been evaluated?",
              getObjectName());
    return constant;
}

const PnaDpdkRegisterCondition *PnaDpdkRegisterCondition::evaluate(const Model &model,
                                                                   bool doComplete) const {
    const auto *evaluatedIndex = model.evaluate(index, doComplete);
    const auto *evaluatedValue = model.evaluate(value, doComplete);
    return new PnaDpdkRegisterCondition(evaluatedIndex, evaluatedValue);
}

cstring PnaDpdkRegisterCondition::getObjectName() const { return "PnaDpdkRegisterCondition"; }

/* =========================================================================================
 *  PnaDpdkActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>> *PnaDpdkActionProfile::getActions()
    const {
    return &actions;
}

PnaDpdkActionProfile::PnaDpdkActionProfile(const IR::IDeclaration *profileDecl)
    : profileDecl(profileDecl) {}

cstring PnaDpdkActionProfile::getObjectName() const { return profileDecl->controlPlaneName(); }

const IR::IDeclaration *PnaDpdkActionProfile::getProfileDecl() const { return profileDecl; }

void PnaDpdkActionProfile::addToActionMap(cstring actionName, std::vector<ActionArg> actionArgs) {
    actions.emplace_back(actionName, actionArgs);
}
size_t PnaDpdkActionProfile::getActionMapSize() const { return actions.size(); }

const PnaDpdkActionProfile *PnaDpdkActionProfile::evaluate(const Model &model,
                                                           bool doComplete) const {
    auto *profile = new PnaDpdkActionProfile(profileDecl);
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
 *  PnaDpdkActionSelector
 * ========================================================================================= */

PnaDpdkActionSelector::PnaDpdkActionSelector(const IR::IDeclaration *selectorDecl,
                                             const PnaDpdkActionProfile *actionProfile)
    : selectorDecl(selectorDecl), actionProfile(actionProfile) {}

cstring PnaDpdkActionSelector::getObjectName() const { return "PnaDpdkActionSelector"; }

const IR::IDeclaration *PnaDpdkActionSelector::getSelectorDecl() const { return selectorDecl; }

const PnaDpdkActionProfile *PnaDpdkActionSelector::getActionProfile() const {
    return actionProfile;
}

const PnaDpdkActionSelector *PnaDpdkActionSelector::evaluate(const Model &model,
                                                             bool doComplete) const {
    const auto *evaluatedProfile = actionProfile->evaluate(model, doComplete);
    return new PnaDpdkActionSelector(selectorDecl, evaluatedProfile);
}

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

}  // namespace P4Tools::P4Testgen::Pna
