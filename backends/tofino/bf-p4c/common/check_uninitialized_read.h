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

#ifndef BF_P4C_COMMON_CHECK_UNINITIALIZED_READ_H_
#define BF_P4C_COMMON_CHECK_UNINITIALIZED_READ_H_

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "ir/ir.h"

using namespace P4;

/* FindUninitializedAndOverlayedReads Pass does the following
 * - Checks all field slices for overlays
 * - Throws a warning message if uninitialized fields can cause data corruption due to overlay
 * - Skips field slices which are
 *   - with a pa_no_init pragma
 *   - pov bits
 *   - padding fields
 *   - deparser zero candidates
 *   - overlayable fields
 *   - pov bit protected fields (which are not overlayable)
 */
class FindUninitializedAndOverlayedReads : public Inspector {
    const FieldDefUse &defuse;
    const PhvInfo &phv;
    const PHV::Pragmas &pragmas;
    const DependencyGraph &deps;

    ordered_set<const PHV::Field *> pov_protected_fields;

    struct uninit_read {
        cstring field_slice;
        cstring overlay_slice;
        cstring field_cont_slice;
        cstring overlay_cont_slice;
        cstring loc;

        // Less than function for std::set comparison to skip adding duplicates
        bool operator<(const uninit_read &other) const {
            return std::tie(field_slice, overlay_slice, field_cont_slice, overlay_cont_slice, loc) <
                   std::tie(other.field_slice, other.overlay_slice, other.field_cont_slice,
                            other.overlay_cont_slice, loc);
        }

        uninit_read(cstring fs, cstring os, cstring fcs, cstring ocs, cstring loc)
            : field_slice(fs),
              overlay_slice(os),
              field_cont_slice(fcs),
              overlay_cont_slice(ocs),
              loc(loc) {}
    };

 public:
    FindUninitializedAndOverlayedReads(const FieldDefUse &defuse, const PhvInfo &phv,
                                       const PHV::Pragmas &pragmas, const DependencyGraph &deps)
        : defuse(defuse), phv(phv), pragmas(pragmas), deps(deps) {}

    bool preorder(const IR::BFN::DeparserParameter *param) override;
    bool preorder(const IR::BFN::Digest *digest) override;

    void end_apply() override;
};

class CheckUninitializedAndOverlayedReads : public PassManager {
 public:
    CheckUninitializedAndOverlayedReads(const FieldDefUse &defuse, const PhvInfo &phv,
                                        const PHV::Pragmas &pragmas, const BFN_Options &options);
};
#endif /* BF_P4C_COMMON_CHECK_UNINITIALIZED_READ_H_ */
