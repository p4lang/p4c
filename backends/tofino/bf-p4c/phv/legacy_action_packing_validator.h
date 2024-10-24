/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_PHV_LEGACY_ACTION_PACKING_VALIDATOR_H_
#define BF_P4C_PHV_LEGACY_ACTION_PACKING_VALIDATOR_H_

#include <optional>

#include "bf-p4c/phv/action_packing_validator_interface.h"
#include "lib/ordered_set.h"

namespace PHV {
namespace legacy {

/// ActionPackingValidator checks action PHV constraints for packing of fieldslices, by using
/// ActionConstraintSolver. It will try to compute container sizes and corresponding alignments
/// of all slice lists, and then it will build and call the solver to check if it is possible
/// to synthesize actions.
class ActionPackingValidator : public ActionPackingValidatorInterface {
 private:
    const ActionSourceTracker &sources_i;
    const PhvUse &uses_i;

 public:
    explicit ActionPackingValidator(const ActionSourceTracker &sources, const PhvUse &uses)
        : sources_i(sources), uses_i(uses) {}

    /// can_pack checks action phv constraints if fieldslices packed in @p slice_lists will be
    /// allocated without further split. Currently, we only support validation of move-based
    /// instructions.
    Result can_pack(const ordered_set<const SuperCluster::SliceList *> &slice_lists,
                    const ordered_set<const SuperCluster::SliceList *> &can_be_further_split = {},
                    const bool loose_mode = false) const override;
};

}  // namespace legacy
}  // namespace PHV

#endif /* BF_P4C_PHV_LEGACY_ACTION_PACKING_VALIDATOR_H_ */
