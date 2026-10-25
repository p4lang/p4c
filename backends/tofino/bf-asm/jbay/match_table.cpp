/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* mau table template specializations for jbay -- #included directly in match_tables.cpp */

/**  Write next table setup, which is JBay-specific.  `tbl` here is the ternary indirect
 * table if there is one, or the match table otherwise */
template <>
void MatchTable::write_next_table_regs(Target::JBay::mau_regs &regs, Table *tbl) {
    auto &merge = regs.rams.match.merge;
    if (!hit_next.empty() || !extra_next_lut.empty()) {
        merge.next_table_map_en |= (1U << logical_id);
        int i = 0;
        for (auto &n : hit_next) {
            merge.pred_map_loca[logical_id][i].pred_map_loca_next_table = n.next_table_id();
            merge.pred_map_loca[logical_id][i].pred_map_loca_exec =
                n.next_in_stage(stage->stageno) >> 1;
            merge.pred_map_glob[logical_id][i].pred_map_glob_exec =
                n.next_in_stage(stage->stageno + 1);
            merge.pred_map_glob[logical_id][i].pred_map_long_brch |= n.long_branch_tags();
            ++i;
        }
        for (auto &n : extra_next_lut) {
            merge.pred_map_loca[logical_id][i].pred_map_loca_next_table = n.next_table_id();
            merge.pred_map_loca[logical_id][i].pred_map_loca_exec =
                n.next_in_stage(stage->stageno) >> 1;
            merge.pred_map_glob[logical_id][i].pred_map_glob_exec =
                n.next_in_stage(stage->stageno + 1);
            merge.pred_map_glob[logical_id][i].pred_map_long_brch |= n.long_branch_tags();
            ++i;
        }
        // is this needed?  The model complains if we leave the unused slots as 0
        while (i < Target::NEXT_TABLE_SUCCESSOR_TABLE_DEPTH())
            merge.pred_map_loca[logical_id][i++].pred_map_loca_next_table = 0x1ff;
    }

    merge.next_table_format_data[logical_id].match_next_table_adr_mask = next_table_adr_mask;
    merge.next_table_format_data[logical_id].match_next_table_adr_miss_value =
        miss_next.next_table_id();
    merge.pred_miss_exec[logical_id].pred_miss_loca_exec =
        miss_next.next_in_stage(stage->stageno) >> 1;
    merge.pred_miss_exec[logical_id].pred_miss_glob_exec =
        miss_next.next_in_stage(stage->stageno + 1);
    merge.pred_miss_long_brch[logical_id] = miss_next.long_branch_tags();
}

template <>
void MatchTable::write_regs(Target::JBay::mau_regs &regs, int type, Table *result) {
    write_common_regs<Target::JBay>(regs, type, result);
    // FIXME -- factor this with JBay GatewayTable::standalone_write_regs
    auto &merge = regs.rams.match.merge;
    if (gress == GHOST) merge.pred_ghost_thread |= 1 << logical_id;
    merge.pred_glob_exec_thread[gress] |= 1 << logical_id;
    if (always_run || pred.empty()) merge.pred_always_run[gress] |= 1 << logical_id;

    if (long_branch_input >= 0)
        setup_muxctl(merge.pred_long_brch_lt_src[logical_id], long_branch_input);

    if (result == nullptr) result = this;

    bool is_branch = (miss_next.next_table() != nullptr);
    if (!is_branch && gateway && gateway->is_branch()) is_branch = true;
    if (!is_branch)
        for (auto &n : hit_next)
            if (n.next_table() != nullptr) {
                is_branch = true;
                break;
            }
    if (!is_branch)
        for (auto &n : extra_next_lut)
            if (n.next_table() != nullptr) {
                is_branch = true;
                break;
            }

    if (!is_branch && result->get_format_field_size("next") > 3) is_branch = true;

    // Check if any table actions have a next table miss set up
    // if yes, the pred_is_a_brch register must be set on the table to override the next table
    // configuration with this value.
    //
    // E.g.
    //   switch (mc_filter.apply().action_run) {
    //     NoAction : { // Has @defaultonly
    //       ttl_thr_check.apply();
    //     }
    //   }
    //
    // Generated bfa
    //   ...
    //   hit: [  END  ]
    //   miss:  END
    //   ...
    //   NoAction(-1, 1):
    //     - hit_allowed: { allowed: false, reason: user_indicated_default_only  }
    //     - default_only_action: { allowed: true, is_constant: true  }
    //     - handle: 0x20000015
    //     - next_table_miss:  ttl_thr_check_0
    //
    // If merge.pred_is_a_brch is not set in this usecase, the default miss configuration of 'END'
    // or 'End of Pipe' is executed and next table ttl_thr_check_0 will not be executed.
    if (!is_branch) {
        for (auto &act : *result->actions) {
            if (act.next_table_miss_ref.next_table()) {
                is_branch = true;
                break;
            }
        }
    }

    if (is_branch) merge.pred_is_a_brch |= 1 << logical_id;

    merge.mpr_glob_exec_thread |= merge.logical_table_thread[0].logical_table_thread_egress &
                                  ~merge.logical_table_thread[0].logical_table_thread_ingress &
                                  ~merge.pred_ghost_thread;
}
