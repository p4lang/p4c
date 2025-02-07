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

#ifndef BF_ASM_JBAY_METER_H_
#define BF_ASM_JBAY_METER_H_

template <typename REGS>
void MeterTable::setup_teop_regs_2(REGS &regs, int meter_group_index) {
    BUG_CHECK(teop >= 0 && teop < 4);
    BUG_CHECK(gress == EGRESS);

    auto &adrdist = regs.rams.match.adrdist;
    if (!teop_initialized) {
        // assume this stage driving teop
        auto delay = stage->pipelength(gress) - stage->pred_cycle(gress) - 7;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_delay = delay;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_delay_en = 1;
        adrdist.teop_bus_ctl[teop].teop_bus_ctl_meter_en = 1;

        adrdist.meter_to_teop_adr_oxbar_ctl[teop].enabled_2bit_muxctl_select = meter_group_index;
        adrdist.meter_to_teop_adr_oxbar_ctl[teop].enabled_2bit_muxctl_enable = 1;
        teop_initialized = true;
    }

    adrdist.teop_to_meter_adr_oxbar_ctl[meter_group_index].enabled_2bit_muxctl_select = teop;
    adrdist.teop_to_meter_adr_oxbar_ctl[meter_group_index].enabled_2bit_muxctl_enable = 1;

    // count all tEOP events
    adrdist.dp_teop_meter_ctl[meter_group_index].dp_teop_meter_ctl_err = 0;
    // Refer to JBAY uArch Section 6.4.4.10.8
    //
    // The user of the incoming tEOP address needs to consider the original
    // driver. For instance, a meter address driver will be aliged with the LSB
    // of the 18b incoming address, whereas a single-entry stats driver will be
    // already padded with 2 zeros.
    //
    // For example, dp_teop_meter_ctl.dp_teop_meter_ctl_rx_shift must be
    // programmed to 2 to compensate for the single-entry stats address driver:
    //
    // Meter (23b) = {4b CMD+Color, ((dp_teop{6b VPN, 10b addr, 2b subword
    // zeros} >> 2) + 7b zero pad)}
    //
    // As per above, the dp_teop_meter_ctl_rx_shift is set based on the original
    // driver. For a meter address driving there is no need for any shift,
    // however if a stats address is driving then it needs to be shifted by 2.
    // Compiler currently does not use this mechanism where a stats address is
    // driving the meter, this is scope for optimization in future.
    adrdist.dp_teop_meter_ctl[meter_group_index].dp_teop_meter_ctl_rx_shift = 0;
    adrdist.dp_teop_meter_ctl[meter_group_index].dp_teop_meter_ctl_rx_en = 1;

    auto &meter = regs.rams.map_alu.meter_group[meter_group_index].meter;
    meter.meter_ctl_teop_en = 1;
}

template <typename REGS>
void MeterTable::write_alu_vpn_range_2(REGS &regs) {
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
                BUG_CHECK(min != 1000000 && max != -1);

                bitvec block_range(min, max - min + 1);
                if (vpn_range.intersects(block_range))
                    error(home_lineno, "Overlapping of VPN range detected");
                else
                    vpn_range |= block_range;

                adrdist.mau_meter_alu_vpn_range[home_row / 4].meter_vpn_base = min;
                adrdist.mau_meter_alu_vpn_range[home_row / 4].meter_vpn_limit = max;
                adrdist.mau_meter_alu_vpn_range[home_row / 4].meter_vpn_range_check_enable = 1;
                for (MatchTable *m : match_tables)
                    adrdist.meter_alu_adr_range_check_icxbar_map[home_row / 4] |= 1U
                                                                                  << m->logical_id;
                break;
            }
        }
        BUG_CHECK(block_start && block_end);
    }

    if (vpn_range != bitvec(minvpn, sparevpn - minvpn))
        error(home_lineno, "VPN range not entirely covered");
}

template <>
void MeterTable::setup_teop_regs(Target::JBay::mau_regs &regs, int meter_group_index) {
    setup_teop_regs_2(regs, meter_group_index);
}

template <>
void MeterTable::write_alu_vpn_range(Target::JBay::mau_regs &regs) {
    write_alu_vpn_range_2(regs);
}

#endif /* BF_ASM_JBAY_METER_H_ */
