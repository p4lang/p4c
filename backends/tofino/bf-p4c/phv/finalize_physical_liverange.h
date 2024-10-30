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

#ifndef BF_P4C_PHV_FINALIZE_PHYSICAL_LIVERANGE_H_
#define BF_P4C_PHV_FINALIZE_PHYSICAL_LIVERANGE_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"
#include "ir/ir.h"

namespace PHV {

/// FinalizePhysicalLiverange will update live ranges of allocated slices based on the
/// table placement result stored in IR::MAU::Table. It will also update
/// PhvInfo::table_to_physical_stages so the field->foreach_alloc can still work correctly.
/// The algorithm is simple, by pre-ordering on all IR::Expression, we can get the
/// corresponding overlapped AllocSlice for the expression, and update its live range by
/// the unit of the expression.
class FinalizePhysicalLiverange : public Inspector, TofinoWriteContext {
 private:
    PhvInfo &phv_i;
    const ClotInfo &clot_i;
    // used to verify overlapped live ranges for overlaying but non-mutex fields.
    const TablesMutuallyExclusive &tb_mutex_i;
    const FieldDefUse &defuse_i;
    /// Keeps metadata about header stacks for the sake of @ref preorder(const IR::BFN::Extract*)
    /// It is set in @ref preorder(const IR::BFN::Pipe*)
    /// @see HeaderPushPop for more discussion on layout of $stkvalid.
    const BFN::HeaderStackInfo *headerStacks = nullptr;

    /// AllocSlice to updated live ranges.
    ordered_map<AllocSlice, LiveRange> live_ranges_i;
    /// table pointers of the new IR.
    ordered_set<const IR::MAU::Table *> tables_i;
    /// table name to stages.
    ordered_map<cstring, std::set<int>> table_stages_i;
    /// temp vars that do not have allocation.
    ordered_map<const PHV::Field *, LiveRange> temp_var_live_ranges_i;

    const PragmaNoInit &pa_no_init;

    /// mark or extends live range of AllocSlices of (@p f, @p bits) to @p unit.
    /// When @p allow_unallocated is false, it will also run a BUG_CHECK to make sure that
    /// we can find a AllocSlice of (@p f, @p bits).
    void mark_access(const PHV::Field *f, le_bitrange bits, const IR::BFN::Unit *unit,
                     bool is_write, bool allow_unallocated = false);

    /// helper functions to update live_ranges_i.
    void update_liverange(const safe_vector<AllocSlice> &slices, const StageAndAccess &op);

 protected:
    /// init_apply clears internal states.
    profile_t init_apply(const IR::Node *root) override {
        live_ranges_i.clear();
        tables_i.clear();
        table_stages_i.clear();
        temp_var_live_ranges_i.clear();

        return Inspector::init_apply(root);
    }

    /// Sets @ref headerStacks
    bool preorder(const IR::BFN::Pipe *pipe) override;

    /// optimization for visitDagOnce = false: we only need to visit parser state once.
    bool preorder(const IR::BFN::ParserState *) override {
        visitOnce();
        return true;
    }

    bool preorder(const IR::BFN::Extract *extract) override;

    /// collect table to stages.
    bool preorder(const IR::MAU::Table *t) override;

    /// re-map AllocSlices to physical liverange.
    bool preorder(const IR::Expression *e) override;

    /// updates PhvInfo:
    /// (1) AllocSlice live range.
    /// (2) PhvInfo::table_to_physical_stages.
    void end_apply() override;

 public:
    /// TODO: we need to set visitDagOnce to be false because there are some IR nodes
    /// that are copied instead of cloned. For example, we notice that the
    /// the *_partition_index:alpm expression is copied but they should be cloned.
    explicit FinalizePhysicalLiverange(PhvInfo &phv, const ClotInfo &clot,
                                       const TablesMutuallyExclusive &tb_mutex,
                                       const FieldDefUse &defuse, const PragmaNoInit &pa_no_init)
        : phv_i(phv), clot_i(clot), tb_mutex_i(tb_mutex), defuse_i(defuse), pa_no_init(pa_no_init) {
        visitDagOnce = false;
    }

    /// @returns unallocated temp vars and their finalized physical live ranges.
    const ordered_map<const PHV::Field *, LiveRange> &unallocated_temp_var_live_ranges() const {
        return temp_var_live_ranges_i;
    }
};

}  // namespace PHV

#endif /* BF_P4C_PHV_FINALIZE_PHYSICAL_LIVERANGE_H_ */
