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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_TABLE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_TABLE_STEPPER_H_

#include <map>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/tofino/shared_expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/tofino/test_spec.h"

namespace P4::P4Tools::P4Testgen::Tofino {

class TofinoTableStepper : public TableStepper {
 private:
    /// Specifies the type of the table implementation:
    /// standard: standard implementation - use normal control plane entries.
    /// selector: ActionSelector implementation - also uses an action profile.
    /// profile:  ACtionProfile implementation - normal entries are not valid
    /// constant: The table is constant - no control entries are possible.
    /// skip: Skip the implementation and just use the default entry (no entry at all).
    enum class TableImplementation { standard, selector, profile, constant, skip };

    /// Tofino specific table properties.
    struct TofinoProperties {
        /// The table has an action profile associated with it.
        const TofinoActionProfile *actionProfile = nullptr;

        /// The table has an action selector associated with it.
        TofinoActionSelector *actionSelector = nullptr;

        /// The selector keys that are part of the selector hash that is calculated.
        std::vector<const IR::Expression *> actionSelectorKeys;

        /// The current execution state does not have this profile added to it yet.
        bool addProfileToState = false;

        /// The type of the table implementation.
        TableImplementation implementaton;
    } tofinoProperties;

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
    explicit TofinoTableStepper(SharedTofinoExprStepper *stepper, const IR::P4Table *table);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_SHARED_TABLE_STEPPER_H_ */
