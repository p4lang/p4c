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

#include "stateful.h"

int StatefulTable::parse_counter_mode(Target::Tofino target, const value_t &v) {
    if (v != "counter") return -1;
    if (v.type == tSTR) return 4;
    if (v.type != tCMD || v.vec.size != 2) return -1;
    static const std::map<std::string, int> modes = {{"hit", 2}, {"miss", 1}, {"gateway", 3}};
    if (!modes.count(v[1].s)) return -1;
    return modes.at(v[1].s);
}

void StatefulTable::set_counter_mode(Target::Tofino target, int mode) {
    stateful_counter_mode |= mode;
}

template <>
void StatefulTable::write_logging_regs(Target::Tofino::mau_regs &regs) {
    auto &merge = regs.rams.match.merge;
    unsigned meter_group = layout.at(0).row / 4U;
    auto &salu = regs.rams.map_alu.meter_group[meter_group].stateful;
    for (MatchTable *m : match_tables) {
        auto *call = m->get_call(this);
        if (!call || call->args.at(0).type != Call::Arg::Counter) continue;
        if (auto mode = call->args.at(0).count_mode()) {
            merge.mau_stateful_log_counter_ctl[m->logical_id / 8U].set_subfield(
                mode, 3 * (m->logical_id % 8U), 3);
            for (auto &rep : merge.mau_stateful_log_ctl_ixbar_map[m->logical_id / 8U])
                rep.set_subfield(meter_group | 0x4, 3 * (m->logical_id % 8U), 3);
        }
    }
    if (stateful_counter_mode) {
        merge.mau_stateful_log_instruction_width.set_subfield(format->log2size - 3, 2 * meter_group,
                                                              2);
        merge.mau_stateful_log_vpn_offset[meter_group / 2].set_subfield(logvpn_min,
                                                                        6 * (meter_group % 2), 6);
        merge.mau_stateful_log_vpn_limit[meter_group / 2].set_subfield(logvpn_max,
                                                                       6 * (meter_group % 2), 6);
    }

    for (size_t i = 0; i < const_vals.size(); ++i) {
        if (const_vals[i].value > INT_MAX || const_vals[i].value < INT_MIN)
            error(const_vals[i].lineno, "constant value %" PRId64 " too large for stateful alu",
                  const_vals[i].value);
        salu.salu_const_regfile[i] = const_vals[i].value & 0xffffffffU;
    }
}

/// Compute the proper value for the register
///    map_alu.meter_alu_group_data_delay_ctl[].meter_alu_right_group_delay
/// which controls the two halves of the ixbar->meter_alu fifo, based on a bytemask of which
/// bytes are needed in the meter_alu.  On Tofino, the fifo is 64 bits wide, so each enable
/// bit controls 32 bits
int AttachedTable::meter_alu_fifo_enable_from_mask(Target::Tofino::mau_regs &, unsigned bytemask) {
    int rv = 0;
    if (bytemask & 0xf) rv |= 1;
    if (bytemask & 0xf0) rv |= 2;
    return rv;
}

void StatefulTable::gen_tbl_cfg(Target::Tofino, json::map &tbl, json::map &stage_tbl) const {}
