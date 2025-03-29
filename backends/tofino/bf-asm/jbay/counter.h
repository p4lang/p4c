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

#ifndef BACKENDS_TOFINO_BF_ASM_JBAY_COUNTER_H_
#define BACKENDS_TOFINO_BF_ASM_JBAY_COUNTER_H_

template <typename REGS>
void CounterTable::setup_teop_regs_2(REGS &regs, int stats_group_index) {
    BUG_CHECK(teop >= 0 && teop < 4, "Invalid teop: %d", teop);
    BUG_CHECK(gress == EGRESS, "Invalid gress: %d", gress);

    auto &adrdist = regs.rams.match.adrdist;

    if (!teop_initialized) {
        // assume this stage driving teop
        auto delay = stage->pipelength(gress) - stage->pred_cycle(gress) - 7;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_delay = delay;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_delay_en = 1;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_stats_en = 1;

        adrdist.stats_to_teop_adr_oxbar_ctl[teop].enabled_2bit_muxctl_select = stats_group_index;
        adrdist.stats_to_teop_adr_oxbar_ctl[teop].enabled_2bit_muxctl_enable = 1;
        teop_initialized = true;
    }

    adrdist.teop_to_stats_adr_oxbar_ctl[stats_group_index].enabled_2bit_muxctl_select = teop;
    adrdist.teop_to_stats_adr_oxbar_ctl[stats_group_index].enabled_2bit_muxctl_enable = 1;

    // count all tEOP events
    adrdist.dp_teop_stats_ctl[stats_group_index].dp_teop_stats_ctl_err = 0;
    // XXX is this always 2?
    adrdist.dp_teop_stats_ctl[stats_group_index].dp_teop_stats_ctl_rx_shift = 2;
    adrdist.dp_teop_stats_ctl[stats_group_index].dp_teop_stats_ctl_rx_en = 1;

    auto &stats = regs.rams.map_alu.stats_wrap[stats_group_index].stats;
    stats.statistics_ctl_teop_en = 1;
}

template <typename REGS>
void CounterTable::write_alu_vpn_range_2(REGS &regs) {
    auto &adrdist = regs.rams.match.adrdist;
    int minvpn, sparevpn;

    // Used to validate the BFA VPN configuration
    std::set<int> vpn_processed;
    bitvec vpn_range;

    // Get Spare VPN
    layout_vpn_bounds(minvpn, sparevpn, false);

    for (int home_row : home_rows) {
        bool block_start = false;
        bool block_end = false;
        int min = 1000000;
        int max = -1;
        for (Layout &logical_row : layout) {
            // Block Start with the home row and End with the Spare VPN
            if (logical_row.row == home_row) block_start = true;

            if (block_start) {
                for (auto v : logical_row.vpns) {
                    if (v == sparevpn) {
                        block_end = true;
                        break;
                    }
                    if (vpn_processed.count(v))
                        error(home_lineno, "Multiple instance of the VPN %d detected", v);
                    else
                        vpn_processed.insert(v);

                    if (v < min) min = v;
                    if (v > max) max = v;
                }
            }
            if (block_end) {
                BUG_CHECK(min != 1000000 && max != -1, " Invalid VPN range");

                bitvec block_range(min, max - min + 1);
                if (vpn_range.intersects(block_range))
                    error(home_lineno, "Overlapping of VPN range detected");
                else
                    vpn_range |= block_range;

                adrdist.mau_stats_alu_vpn_range[home_row / 4].stats_vpn_base = min;
                adrdist.mau_stats_alu_vpn_range[home_row / 4].stats_vpn_limit = max;
                adrdist.mau_stats_alu_vpn_range[home_row / 4].stats_vpn_range_check_enable = 1;
                break;
            }
        }
        BUG_CHECK(block_start && block_end, "Invalid VPN range");
    }

    if (vpn_range != bitvec(minvpn, sparevpn - minvpn))
        error(home_lineno, "VPN range not entirely covered");
}

template <>
void CounterTable::setup_teop_regs(Target::JBay::mau_regs &regs, int stats_group_index) {
    setup_teop_regs_2(regs, stats_group_index);
}

template <>
void CounterTable::write_alu_vpn_range(Target::JBay::mau_regs &regs) {
    write_alu_vpn_range_2(regs);
}

#endif /* BACKENDS_TOFINO_BF_ASM_JBAY_COUNTER_H_ */
