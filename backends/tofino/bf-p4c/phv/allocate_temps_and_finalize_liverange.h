/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_PHV_ALLOCATE_TEMPS_AND_FINALIZE_LIVERANGE_H_
#define BF_P4C_PHV_ALLOCATE_TEMPS_AND_FINALIZE_LIVERANGE_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "ir/ir.h"

namespace PHV {

// TODO (Yumin): Enhance logs during PHV::AllocateTempsAndFinalizeLiverange to indentify
// and log these conditions in phv logs where a field mutex is not consistent with a table mutex
//
// Two non-mutex fields, f_a and f_b, are overlaid in B0.
// f_a's live range: [-1w, 4r]
// f_b's live range: [3w, 7r]
// It's not a BUG, because when the table t_a that writes f_a are mutex
// with table t_b that reads f_b, hence they will not cause read / write violations
//
//             stage 3         stage 4
//    |---- t_a writes B0
// ---|
//    |--------------------- t_b reads B0
//

class AllocateTempsAndFinalizeLiverange : public PassManager {
    PhvInfo& phv_i;
    const ClotInfo& clot_i;
    const FieldDefUse &defuse_i;
    const TableSummary &ts_i;

 public:
    AllocateTempsAndFinalizeLiverange(PhvInfo& phv, const ClotInfo& clot, const FieldDefUse& defuse,
            const ::TableSummary &ts);
};

// For programs exceeding available stages for Tofino HW i.e. (12 for Tofino1 & 20 for Tofino2), it
// is necessary to have a bfa and whatever logs / json can be generated for a better debug
// experience.
//
// For such compilation, max stage is greater than allowed stages hence the deparser stages is the
// new max stage + 1. Currently live range checks are performed on each field slice, hence we add a
// Pass here UpdateDeparserStage to indicate that the deparser stage has exceeded. Live range checks
// are also updated to check the presence of this condition and validate live range.
//
// NOTES:
// - This pass is only for Alt Phv Alloc compilation as bfa is always generated in
// such conditions for a standard compilation, hence this also brings parity with the standard
// compilation. Also the parent pass AllocateTempsAndFinalizeLiverange is only applicable to Alt
// - This pass is only triggered when compilation has failed.
// - When we reach this stage of compilation (AllocateTempsAndFinalizeLiverange) it is guaranteed to
// have exceeded stages if failed as any other Table Placement error would cause a fatal error in
// TableSummary earlier.
//
// Workflow:
//
// Alt-Phv-Alloc Enabled
//   |
// Table Placement exceeds stages
//   |
// Table Summary (issues warning)
//   |
// AllocateTempsAndFinalizeLiverange triggered
//   |
// Has TP Failed? -> Yes
//   |                 |
//   NO          UpdateDeparserStage
//   |    (set isPhysicalDeparserStageExceeded)
//   v                 |
// FinalizePhysicalLiverange
// (check isPhysicalDeparserStageExceeded and allow larger deparser stage)
//
class UpdateDeparserStage: public Inspector {
    PhvInfo& phv_i;

    void end_apply() override;
 public:
    explicit UpdateDeparserStage(PhvInfo &phv) : phv_i(phv) {}
};

}  // namespace PHV

#endif /* BF_P4C_PHV_ALLOCATE_TEMPS_AND_FINALIZE_LIVERANGE_H_ */
