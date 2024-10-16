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

#ifndef BF_P4C_PHV_V2_TABLE_REPLAY_FRIENDLY_CONSTRAINTS_H_
#define BF_P4C_PHV_V2_TABLE_REPLAY_FRIENDLY_CONSTRAINTS_H_

#include <algorithm>
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"

struct AllocInfo {
    int length;
    size_t container_size;
    ordered_set<cstring> pack_with;
    bool perfectly_aligned;
};

std::ostream& operator<<(std::ostream &out, const AllocInfo &ai);

namespace PHV {
namespace v2 {

// This pass adds a few pa_container_size constraints based on the feedback from table summary after
// table replay failure. These pa_container_size constraints are trying to replay the trivial
// allocation result for the problematic table provided by table summary. And this can potentially
// fix the problematic's fitting issue and advance the table replay.
class TableReplayFriendlyPhvConstraints : public Transform {
    MauBacktracker &mau_backtracker;
    PhvInfo &phv;
    const ordered_map<cstring, ordered_map<int, AllocInfo>> &trivial_allocation_info;
    const ordered_map<cstring, ordered_map<int, AllocInfo>> &real_allocation_info;
    // extra pa_container_size pragmas to fix table replay
    ordered_map<cstring, std::vector<PHV::Size>> add_pa_container_size;
    // extra pa_no_pack pragmas to fix table replay
    ordered_map<cstring, ordered_set<cstring>> add_pa_no_pack;
    // problematic_table during table replay found by table_summary
    const IR::MAU::Table* problematic_table = nullptr;
    // field candidates to fix in the problematic table
    ordered_set<const PHV::Field *> field_candidates;
    // a map from action to a set of fields in this action
    ordered_map<const IR::MAU::Action *, ordered_set<cstring>> action_to_fields;
    // a set of field that has the same phv size allocation in both trivial and real phv allocation
    ordered_set<const PHV::Field *> container_size_ok;

 public:
    const ordered_map<cstring, std::vector<PHV::Size>> &get_container_size_constr() {
        return add_pa_container_size;
    }
    const ordered_map<cstring, ordered_set<cstring>> &get_no_pack_constr() {
        return add_pa_no_pack;
    }
    TableReplayFriendlyPhvConstraints(
        MauBacktracker &mau_backtracker, PhvInfo &phv,
        const ordered_map<cstring, ordered_map<int, AllocInfo>> &trivial_allocation_info,
        const ordered_map<cstring, ordered_map<int, AllocInfo>> &real_allocation_info):
        mau_backtracker(mau_backtracker), phv(phv),
        trivial_allocation_info(trivial_allocation_info),
        real_allocation_info(real_allocation_info) {}

    const IR::Node *preorder(IR::BFN::Pipe * pipe) override;
    const IR::Node *preorder(IR::Expression *expr) override;

    const IR::Node *postorder(IR::BFN::Pipe *pipe) override {
        LOG5("print all pa container size");
        LOG5("pipe is " + pipe->canon_name());
        for (auto &it : add_pa_container_size) {
            LOG5(it.first);
            for (auto size : it.second) {
                LOG5(size);
            }
        }
        return pipe;
    }
    void end_apply(const IR::Node *) override;
};

// This table collects PHV allocation results after phv analysis. It provides information for
// TableReplayFriendlyPhvConstraints, so that it can replay trivial phv allocation result for some
// table
class CollectPHVAllocationResult : public Inspector {
    // This data structure maps the name of a field to a map that maps alignment to AllocInfo.
    // For example, if a field's name is f and it has two AllocSlice, one is allocating [7:0] to a
    // 8-bit container, another is allocation [8:15] to another 8-bit container. Then one entry of
    // allocation info is f -> { { 0 -> { length = 8, container_size = 8 },
    // { 8 -> { length = 8, container_size = 8 } } }}
    ordered_map<cstring, ordered_map<int, AllocInfo>> trivial_allocation_info;
    ordered_map<cstring, ordered_map<int, AllocInfo>> real_allocation_info;
    PhvInfo &phv;
    MauBacktracker &mau_backtracker;
 public:
    CollectPHVAllocationResult(PhvInfo &phv, MauBacktracker &mau_backtracker) :
        phv(phv), mau_backtracker(mau_backtracker) {}
    void end_apply(const IR::Node *) override;
    const ordered_map<cstring, ordered_map<int, AllocInfo>> &get_trivial_allocation_info()
        { return trivial_allocation_info; }
    const ordered_map<cstring, ordered_map<int, AllocInfo>> &get_real_allocation_info()
        { return real_allocation_info; }
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_TABLE_REPLAY_FRIENDLY_CONSTRAINTS_H_ */
