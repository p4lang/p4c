#include "backends/p4tools/testgen/targets/bmv2/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/* =========================================================================================
 *  Uninterpreted Bmv2Register
 * ========================================================================================= */

Bmv2RegisterValue::Bmv2RegisterValue(const IR::Expression* initialValue)
    : initialValue(initialValue) {}

void Bmv2RegisterValue::addRegisterCondition(Bmv2RegisterCondition cond) {
    registerConditions.push_back(cond);
}

const IR::Expression* Bmv2RegisterValue::getInitialValue() const { return initialValue; }

cstring Bmv2RegisterValue::getObjectName() const { return "Bmv2RegisterValue"; }

const IR::Expression* Bmv2RegisterValue::getCurrentValue(const IR::Expression* index) const {
    const IR::Expression* baseExpr = initialValue;
    for (const auto& Bmv2registerValue : registerConditions) {
        const auto* storedIndex = Bmv2registerValue.index;
        const auto* storedVal = Bmv2registerValue.value;
        baseExpr =
            new IR::Mux(baseExpr->type, new IR::Equ(storedIndex, index), storedVal, baseExpr);
    }
    return baseExpr;
}

Bmv2RegisterCondition::Bmv2RegisterCondition(const IR::Expression* index,
                                             const IR::Expression* value)
    : index(index), value(value) {}

/* =========================================================================================
 *  Bmv2_V1ModelActionProfile
 * ========================================================================================= */

const std::vector<std::pair<cstring, std::vector<ActionArg>>>*
Bmv2_V1ModelActionProfile::getActions() const {
    return &actions;
}

Bmv2_V1ModelActionProfile::Bmv2_V1ModelActionProfile(const IR::IDeclaration* profileDecl)
    : profileDecl(profileDecl) {}

cstring Bmv2_V1ModelActionProfile::getObjectName() const { return profileDecl->controlPlaneName(); }

const IR::IDeclaration* Bmv2_V1ModelActionProfile::getProfileDecl() const { return profileDecl; }

void Bmv2_V1ModelActionProfile::addToActionMap(cstring actionName,
                                               std::vector<ActionArg> actionArgs) {
    actions.emplace_back(actionName, actionArgs);
}
size_t Bmv2_V1ModelActionProfile::getActionMapSize() const { return actions.size(); }

/* =========================================================================================
 *  Bmv2_V1ModelActionSelector
 * ========================================================================================= */

cstring Bmv2_V1ModelActionSelector::getObjectName() const { return "Bmv2_V1ModelActionSelector"; }

const IR::IDeclaration* Bmv2_V1ModelActionSelector::getSelectorDecl() const { return selectorDecl; }

const ActionProfile* Bmv2_V1ModelActionSelector::getActionProfile() const { return actionProfile; }

Bmv2_V1ModelActionSelector::Bmv2_V1ModelActionSelector(const IR::IDeclaration* selectorDecl,
                                                       const ActionProfile* actionProfile)
    : selectorDecl(selectorDecl), actionProfile(actionProfile) {}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
