#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TABLE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TABLE_STEPPER_H_

#include <vector>

#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2V1ModelTableStepper : public TableStepper {
 private:
    /// Specifies the type of the table implementation:
    /// standard: standard implementation - use normal control plane entries.
    /// selector: ActionSelector implementation - also uses an action profile.
    /// profile:  ACtionProfile implementation - normal entries are not valid
    /// constant: The table is constant - no control entries are possible.
    /// skip: Skip the implementation and just use the default entry (no entry at all).
    enum class TableImplementation { standard, selector, profile, constant, skip };

    /// BMv2 specific table properties.
    struct Bmv2V1ModelProperties {
        /// The table has an action profile associated with it.
        const Bmv2V1ModelActionProfile *actionProfile = nullptr;

        /// The table has an action selector associated with it.
        const Bmv2V1ModelActionSelector *actionSelector = nullptr;

        /// The selector keys that are part of the selector hash that is calculated.
        std::vector<const IR::Expression *> actionSelectorKeys;

        /// The current execution state does not have this profile added to it yet.
        bool addProfileToState = false;

        /// The type of the table implementation.
        TableImplementation implementaton = TableImplementation::standard;
    } bmv2V1ModelProperties;

    /// Check whether the table has an action profile implementation.
    bool checkForActionProfile();

    /// Check whether the table has an action selector implementation.
    bool checkForActionSelector();

    /// If the table has an action profile implementation, evaluate the match-action list
    /// accordingly. Entries will use indices to refer to actions instead of their labels.
    void evalTableActionProfile(const std::vector<const IR::ActionListElement *> &tableActionList);

    /// If the table has an action selector implementation, evaluate the match-action list
    /// accordingly. Entries will use indices to refer to actions instead of their labels.
    void evalTableActionSelector(const std::vector<const IR::ActionListElement *> &tableActionList);

 protected:
    const IR::Expression *computeTargetMatchType(const TableUtils::KeyProperties &keyProperties,
                                                 TableMatchMap *matches,
                                                 const IR::Expression *hitCondition) override;

    void checkTargetProperties(
        const std::vector<const IR::ActionListElement *> &tableActionList) override;

    void evalTargetTable(
        const std::vector<const IR::ActionListElement *> &tableActionList) override;

 public:
    explicit Bmv2V1ModelTableStepper(Bmv2V1ModelExprStepper *stepper, const IR::P4Table *table);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TABLE_STEPPER_H_ */
