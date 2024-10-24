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

#ifndef BF_P4C_PHV_VALIDATE_ALLOCATION_H_
#define BF_P4C_PHV_VALIDATE_ALLOCATION_H_

#include "bf-p4c/phv/alloc_setting.h"
#include "bf-p4c/phv/fieldslice_live_range.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/symbitmatrix.h"
#include "phv_fields.h"

class PhvInfo;
class ClotInfo;

namespace PHV {

/**
 * Validates that the PHV allocation represented by a PhvInfo object is
 * consistent with the input program and with Tofino hardware constraints.
 *
 * Some of the things this pass verifies:
 *  - Are containers correctly split between threads?
 *  - Are any PHV bits allocated to unused fields?
 *  - Are any field bits allocated more than once?
 *  - Does the allocation allow fields to be deparsed correctly?
 *
 * See the implementation for the full list of checks performed. If any problems
 * are found, they are reported as errors.
 */
class ValidateAllocation final : public Inspector {
 public:
    ValidateAllocation(PhvInfo &phv, const ClotInfo &clot,
                       const PHV::FieldSliceLiveRangeDB &physical_liverange,
                       const PHV::AllocSetting &setting)
        : phv(phv), clot(clot), physical_liverange(physical_liverange), setting(setting) {}

    void set_physical_liverange_overlay(bool enable) { physical_liverange_overlay = enable; }

 private:
    PhvInfo &phv;
    const ClotInfo &clot;
    const PHV::FieldSliceLiveRangeDB &physical_liverange;
    bool physical_liverange_overlay = false;
    const PHV::AllocSetting &setting;

    SymBitMatrix mutually_exclusive_field_ids;
    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Digest *digest) override;
    bool preorder(const IR::BFN::Pipe *pipe) override;
    bool preorder(const IR::BFN::DeparserParameter *dp) override;

    /// @returns total number of container bits used for POV bit allocation in @p gress.
    size_t getPOVContainerBytes(gress_t gress) const;
};

}  // namespace PHV

#endif /* BF_P4C_PHV_VALIDATE_ALLOCATION_H_ */
