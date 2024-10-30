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

#ifndef BF_P4C_PHV_ACTION_PACKING_VALIDATOR_INTERFACE_H_
#define BF_P4C_PHV_ACTION_PACKING_VALIDATOR_INTERFACE_H_

#include <optional>

#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/phv/utils/utils.h"
#include "lib/cstring.h"
#include "lib/ordered_set.h"

namespace PHV {

/// PackingValidator is an interface type of classes that have a can_pack function to verify
/// whether the packing will violate any constraint.
class ActionPackingValidatorInterface {
 public:
    struct Result {
        enum class Code { OK, BAD, UNKNOWN };
        Code code = Code::UNKNOWN;
        cstring err = cstring::empty;
        /// When the instruction to the destination container of an action cannot be found,
        /// all slice lists involved are included in this set: including all sources
        /// and the destination. If it is an intrinsic conflict of constraint, then either
        /// (1)the destination needs to be split more, or (2) sources needs to be split
        /// less to be packed together.
        ordered_set<const SuperCluster::SliceList *> *invalid_packing =
            new ordered_set<const SuperCluster::SliceList *>();
        std::optional<const IR::MAU::Action *> invalid_action = std::nullopt;
        Result() = default;
        explicit Result(Code code, cstring err = cstring::empty) : code(code), err(err) {}
    };

    /// can_pack returns Result with code::OK if @p slice_lists can be allocated to containers
    /// without being further split, and allocation will not violate action PHV constraints.
    /// If @p can_be_split_further is not empty, this function will further check that
    /// when @p slice_lists are allocated to containers without further split, can
    /// @p can_be_split_further be split and allocated successfully, by checking
    /// whether slices in the same byte (must be packed in one container) will violate any
    /// action phv constraint.
    /// contract: both @p slice_lists and @p can_be_split_further must contain fine-sliced
    /// PHV::Fieldslices, that any operation will only read or write one PHV::FieldSlice.
    /// All Slicelists in a super cluster satisfy this constraint.
    /// contract: no overlapping slice between @p slice_lists and @p can_be_further_split.
    /// @p when loose_mode is true, instructions are allowed to overwrite bytes of destination
    /// container that has unwritten fields slices. Although it usually means that the action
    /// cannot be be synthesized because *live* bits will be corrupted. However, because dest
    /// can be split further later to avoid corruption, it is sometimes useful to use loose_mode
    /// so that caller can solve the problem in a divide-and-conquer way.
    virtual Result can_pack(
        const ordered_set<const SuperCluster::SliceList *> &slice_lists,
        const ordered_set<const SuperCluster::SliceList *> &can_be_further_split = {},
        const bool loose_mode = false) const = 0;
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ACTION_PACKING_VALIDATOR_INTERFACE_H_ */
