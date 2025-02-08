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

#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "data_switchbox.h"
#include "input_xbar.h"
#include "misc.h"

// target specific template specializations
#include "tofino/meter.h"
#include "jbay/meter.h"

Table::Layout::bus_type_t MeterTable::default_bus_type() const {
    // FIXME -- this is a bit of a hack -- if color_mapram_addr has been set, we want the
    // bus_type for color maprams, not for the meter proper (which should not actually
    // have a bus specified?)
    if (color_mapram_addr == IDLE_MAP_ADDR) return Layout::IDLE_BUS;
    warning(lineno, "meter table should not have bus:, will be ignored");
    return Layout::SEARCH_BUS;
}

void MeterTable::setup(VECTOR(pair_t) & data) {
    common_init_setup(data, false, P4Table::Stateful);
    for (auto &kv : MapIterChecked(data, true)) {
        if (common_setup(kv, data, P4Table::Meter)) {
        } else if (kv.key == "input_xbar") {
            if (CHECKTYPE(kv.value, tMAP))
                input_xbar.emplace_back(InputXbar::create(this, false, kv.key, kv.value.map));
        } else if (kv.key == "color_aware") {
            if (kv.value == "per_flow")
                color_aware = color_aware_per_flow_enable = true;
            else
                color_aware = get_bool(kv.value);
        } else if (kv.key == "color_maprams") {
            if (CHECKTYPE(kv.value, tMAP)) {
                if (auto addr_type = get(kv.value.map, "address")) {
                    if (CHECKTYPE(*addr_type, tSTR)) {
                        if (*addr_type == "idletime")
                            color_mapram_addr = IDLE_MAP_ADDR;
                        else if (*addr_type == "stats")
                            color_mapram_addr = STATS_MAP_ADDR;
                        else
                            error(addr_type->lineno, "Unrecognized color mapram address type %s",
                                  addr_type->s);
                    }
                }
                setup_layout(color_maprams, kv.value.map, " color_maprams");
                if (auto *vpn = get(kv.value.map, "vpns"))
                    if (CHECKTYPE(*vpn, tVEC)) setup_vpns(color_maprams, &vpn->vec, true);
            }
        } else if (kv.key == "pre_color") {
            if (CHECKTYPE(kv.value, tCMD)) {
                if (kv.value != "hash_dist")
                    error(kv.value.lineno, "Pre color must come from hash distribution");
                if (kv.value.vec.size != 3)
                    error(kv.value.lineno,
                          "Pre color hash distribution requires two parameters,"
                          " but has %d",
                          kv.value.vec.size);
                if (CHECKTYPE(kv.value.vec[1], tINT)) pre_color_hash_dist_unit = kv.value.vec[1].i;
                if (CHECKTYPE(kv.value.vec[2], tRANGE)) {
                    auto range = kv.value.vec[2];
                    int diff = range.hi - range.lo + 1;
                    if (diff != 2 || range.lo % 2 != 0)
                        error(kv.value.lineno, "Invalid hash distribution range for precolor");
                    pre_color_bit_lo = range.lo;
                }
            }
        } else if (kv.key == "type") {
            if (kv.value == "standard")
                type = STANDARD;
            else if (kv.value == "lpf")
                type = LPF;
            else if (kv.value == "wred")
                type = RED;
            else
                error(kv.value.lineno, "Unknown meter type %s", value_desc(kv.value));
        } else if (kv.key == "red_output") {
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &v : kv.value.map) {
                    if (CHECKTYPE(v.key, tSTR) && CHECKTYPE(v.value, tINT)) {
                        if (v.key == "drop")
                            red_drop_value = v.value.i;
                        else if (v.key == "nodrop")
                            red_nodrop_value = v.value.i;
                        else
                            error(kv.value.lineno, "Unknown meter red param: %s", v.key.s);
                    }
                }
            }
        } else if (kv.key == "count") {
            if (kv.value == "bytes")
                count = BYTES;
            else if (kv.value == "packets")
                count = PACKETS;
            else
                error(kv.value.lineno, "Unknown meter count %s", value_desc(kv.value));
        } else if (kv.key == "teop") {
            if (gress != EGRESS) error(kv.value.lineno, "tEOP can only be used in EGRESS");
            if (!Target::SUPPORT_TRUE_EOP())
                error(kv.value.lineno, "tEOP is not available on device");
            if (CHECKTYPE(kv.value, tINT)) {
                teop = kv.value.i;
                if (teop < 0 || teop > 3)
                    error(kv.value.lineno, "Invalid tEOP bus %d, valid values are 0-3", teop);
            }
            BUG_CHECK(!stage->teop[teop].first,
                      "previously used tEOP bus %d used again in stage %d", teop, stage->stageno);
            stage->teop[teop] = {true, stage->stageno};
        } else if (kv.key == "green") {
            if (CHECKTYPE(kv.value, tINT)) {
                green_value = kv.value.i;
            }
        } else if (kv.key == "yellow") {
            if (CHECKTYPE(kv.value, tINT)) {
                yellow_value = kv.value.i;
            }
        } else if (kv.key == "red") {
            if (CHECKTYPE(kv.value, tINT)) {
                red_value = kv.value.i;
            }
        } else if (kv.key == "profile") {
            if (CHECKTYPE(kv.value, tINT)) {
                profile = kv.value.i;
            }
        } else if (kv.key == "sweep_interval") {
            if (CHECKTYPE(kv.value, tINT)) {
                // sweep_interval value in assembly if present is from
                // meter_sweep_interval pragma in p4 program. Allowed values for
                // the meter_sweep_interval register are [0:20]. but [5:20] are
                // only to be used with shifting meter time scale. We check and
                // throw an error if value is present and not in range[0:4]
                int intvl = kv.value.i;
                if (intvl >= 0 && intvl <= 4)
                    sweep_interval = intvl;
                else
                    error(
                        lineno,
                        "Invalid meter sweep interval of %d. Allowed values are in the range[0:4]",
                        intvl);
            }
        } else if (kv.key == "bytecount_adjust") {
            if (CHECKTYPE(kv.value, tINT)) {
                bytecount_adjust = kv.value.i;
            }
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (teop >= 0 && count != BYTES) error(lineno, "tEOP bus can only used when counting bytes");
    if (Target::SRAM_GLOBAL_ACCESS())
        alloc_global_srams();
    else
        alloc_rams(true, stage->sram_use);
}

void MeterTable::pass1() {
    LOG1("### Meter table " << name() << " pass1 " << loc());
    if (!p4_table)
        p4_table = P4Table::alloc(P4Table::Meter, this);
    else
        p4_table->check(this);
    alloc_vpns();
    alloc_maprams();
    if (color_maprams.empty() && type != LPF && type != RED)
        error(lineno, "Missing color_maprams in meter table %s", name());
    if (uses_colormaprams() && color_mapram_addr == NO_COLOR_MAP)
        error(lineno, "Missing color mapram address type in table %s", name());
    for (auto &r : color_maprams) {
        for (auto &memunit : r.memunits) {
            BUG_CHECK(memunit.row == r.row, "memunit on wrong row");
            if (Table *old = stage->mapram_use[r.row][memunit.col])
                error(r.lineno,
                      "Table %s trying to use mapram %d,%d for color, which is "
                      "in use by table %s",
                      name(), r.row, memunit.col, old->name());
            stage->mapram_use[r.row][memunit.col] = this;
        }
    }
    if (!no_vpns && !color_maprams.empty() && color_maprams[0].vpns.empty())
        setup_vpns(color_maprams, 0);
    std::sort(layout.begin(), layout.end(),
              [](const Layout &a, const Layout &b) -> bool { return a.row > b.row; });
    stage->table_use[timing_thread(gress)] |= Stage::USE_METER;
    if (type == LPF || type == RED)
        stage->table_use[timing_thread(gress)] |= Stage::USE_METER_LPF_RED;
    for (auto &ixb : input_xbar) ixb->pass1();
    for (auto &hd : hash_dist) hd.pass1(this, HashDistribution::OTHER, false);
    int prev_row = -1;
    for (auto &row : layout) {
        if (home_rows.count(row.row)) prev_row = -1;

        if (prev_row >= 0)
            need_bus(lineno, stage->overflow_bus_use, row.row, "Overflow");
        else
            need_bus(lineno, stage->meter_bus_use, row.row, "Meter data");
        for (int r = (row.row + 1) | 1; r < prev_row; r += 2)
            need_bus(lineno, stage->overflow_bus_use, r, "Overflow");
        prev_row = row.row;
    }
    Synth2Port::pass1();
}

void MeterTable::pass2() {
    LOG1("### Meter table " << name() << " pass2 " << loc());
    for (auto &ixb : input_xbar) ixb->pass2();

    for (auto match_table : get_match_tables()) {
        for (auto &hd : match_table->hash_dist) {
            if (hd.id == pre_color_hash_dist_unit) {
                hd.meter_pre_color = true;
                hd.meter_mask_index = pre_color_bit_lo / 2;
            }
        }
    }
    if (get_match_tables().size() > 1 && color_mapram_addr == IDLE_MAP_ADDR)
        error(lineno, "Shared meter cannot use idletime addressing for color maprams");
    for (auto &hd : hash_dist) hd.pass2(this);
}

void MeterTable::pass3() { LOG1("### Meter table " << name() << " pass3 " << loc()); }

int MeterTable::direct_shiftcount() const {
    return 64 + METER_ADDRESS_ZERO_PAD - 7;  // meters are always 128 bits wide
}

int MeterTable::indirect_shiftcount() const {
    return METER_ADDRESS_ZERO_PAD - 7;  // meters are always 128 bits wide
}

int MeterTable::address_shift() const {
    return 7;  // meters are always 128 bits wide
}

int MeterTable::color_shiftcount(Table::Call &call, int group, int tcam_shift) const {
    int extra_padding = 0;
    int zero_pad = 0;
    if (color_mapram_addr == IDLE_MAP_ADDR) {
        extra_padding = IDLETIME_ADDRESS_ZERO_PAD - IDLETIME_HUFFMAN_BITS;
        zero_pad = IDLETIME_ADDRESS_ZERO_PAD;
    } else if (color_mapram_addr == STATS_MAP_ADDR) {
        extra_padding = STAT_ADDRESS_ZERO_PAD - STAT_METER_COLOR_LOWER_HUFFMAN_BITS;
        zero_pad = STAT_ADDRESS_ZERO_PAD;
    }

    if (call.args[0].name() && strcmp(call.args[0].name(), "$DIRECT") == 0) {
        return 64 + tcam_shift + extra_padding;
    } else if (auto f = call.args[0].field()) {
        return f->by_group[group]->bit(0) % 128U + extra_padding;
    } else if (auto f = call.args[1].field()) {
        return f->bit(0) + zero_pad;
    } else {
        return 0;
    }
}

unsigned MeterTable::determine_shiftcount(Table::Call &call, int group, unsigned word,
                                          int tcam_shift) const {
    return determine_meter_shiftcount(call, group, word, tcam_shift);
}

template <class REGS>
void MeterTable::write_merge_regs_vt(REGS &regs, MatchTable *match, int type, int bus,
                                     const std::vector<Call::Arg> &args) {
    auto &merge = regs.rams.match.merge;
    unsigned adr_mask = 0U;
    unsigned per_entry_en_mux_ctl = 0U;
    unsigned adr_default = 0U;
    unsigned meter_type_position = 0U;
    METER_ACCESS_TYPE default_type = match->default_meter_access_type(false);
    AttachedTable::determine_meter_merge_regs(match, type, bus, args, default_type, adr_mask,
                                              per_entry_en_mux_ctl, adr_default,
                                              meter_type_position);
    merge.mau_meter_adr_default[type][bus] = adr_default;
    merge.mau_meter_adr_mask[type][bus] = adr_mask;
    merge.mau_meter_adr_per_entry_en_mux_ctl[type][bus] = per_entry_en_mux_ctl;
    merge.mau_meter_adr_type_position[type][bus] = meter_type_position;
}

template <class REGS>
void MeterTable::write_color_regs(REGS &regs, MatchTable *match, int type, int bus,
                                  const std::vector<Call::Arg> &args) {
    BUG_CHECK(uses_colormaprams(), "meter %s does not use color maprams, but uses color?", name());
    auto &merge = regs.rams.match.merge;
    unsigned adr_mask = 0U;
    unsigned per_entry_en_mux_ctl = 0U;
    unsigned adr_default = 0U;
    unsigned meter_type_position = 0U;
    AttachedTable::determine_meter_merge_regs(match, type, bus, args, METER_COLOR_ACCESS, adr_mask,
                                              per_entry_en_mux_ctl, adr_default,
                                              meter_type_position);

    // Based on the uArch section 6.2.8.4.9 Map RAM Addressing, color maprams can be
    // addressed by either idletime or stats based addresses.  Which address is used
    // can be specified in the asm file, and is built according to the specification

    if (color_mapram_addr == IDLE_MAP_ADDR) {
        unsigned idle_mask = (1U << IDLETIME_ADDRESS_BITS) - 1;
        unsigned full_idle_mask = (1U << IDLETIME_FULL_ADDRESS_BITS) - 1;
        unsigned shift_diff = METER_LOWER_HUFFMAN_BITS - IDLETIME_HUFFMAN_BITS;
        merge.mau_idletime_adr_mask[type][bus] = (adr_mask >> shift_diff) & idle_mask;
        merge.mau_idletime_adr_default[type][bus] = (adr_default >> shift_diff) & full_idle_mask;
        if (per_entry_en_mux_ctl > shift_diff)
            merge.mau_idletime_adr_per_entry_en_mux_ctl[type][bus] =
                per_entry_en_mux_ctl - shift_diff;
        else
            merge.mau_idletime_adr_per_entry_en_mux_ctl[type][bus] = 0;
    } else if (color_mapram_addr == STATS_MAP_ADDR) {
        unsigned stats_mask = (1U << STAT_ADDRESS_BITS) - 1;
        unsigned full_stats_mask = (1U << STAT_FULL_ADDRESS_BITS) - 1;
        unsigned shift_diff = METER_LOWER_HUFFMAN_BITS - STAT_METER_COLOR_LOWER_HUFFMAN_BITS;
        merge.mau_stats_adr_mask[type][bus] = (adr_mask >> shift_diff) & stats_mask;
        merge.mau_stats_adr_default[type][bus] = (adr_default >> shift_diff) & full_stats_mask;
        if (per_entry_en_mux_ctl > shift_diff)
            merge.mau_stats_adr_per_entry_en_mux_ctl[type][bus] = per_entry_en_mux_ctl - shift_diff;
        else
            merge.mau_stats_adr_per_entry_en_mux_ctl[type][bus] = 0;
    } else {
        BUG();
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void MeterTable::write_color_regs, mau_regs &,
                      MatchTable *, int, int, const std::vector<Call::Arg> &);

template <class REGS>
void MeterTable::setup_exact_shift(REGS &regs, int bus, int group, int word, int word_group,
                                   Call &meter_call, Call &color_call) {
    auto &merge = regs.rams.match.merge;
    int shiftcount = determine_shiftcount(meter_call, group, word, 0);
    merge.mau_meter_adr_exact_shiftcount[bus][word_group] = shiftcount;
    if (uses_colormaprams()) {
        int color_shift = color_shiftcount(color_call, group, 0);
        if (color_mapram_addr == IDLE_MAP_ADDR) {
            merge.mau_idletime_adr_exact_shiftcount[bus][word_group] = color_shift;
            merge.mau_payload_shifter_enable[0][bus].idletime_adr_payload_shifter_en = 1;
        } else if (color_mapram_addr == STATS_MAP_ADDR) {
            merge.mau_stats_adr_exact_shiftcount[bus][word_group] = color_shift;
            merge.mau_payload_shifter_enable[0][bus].stats_adr_payload_shifter_en = 1;
        }
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void MeterTable::setup_exact_shift, mau_regs &,
                      int, int, int, int, Call &, Call &);

template <class REGS>
void MeterTable::setup_tcam_shift(REGS &regs, int bus, int tcam_shift, Call &meter_call,
                                  Call &color_call) {
    auto &merge = regs.rams.match.merge;
    int shiftcount = determine_shiftcount(meter_call, 0, 0, tcam_shift);
    merge.mau_meter_adr_tcam_shiftcount[bus] = shiftcount;
    if (uses_colormaprams()) {
        int color_shift = color_shiftcount(color_call, 0, tcam_shift);
        if (color_mapram_addr == IDLE_MAP_ADDR) {
            merge.mau_idletime_adr_tcam_shiftcount[bus] = color_shift;
            merge.mau_payload_shifter_enable[1][bus].idletime_adr_payload_shifter_en = 1;
        } else if (color_mapram_addr == STATS_MAP_ADDR) {
            merge.mau_stats_adr_tcam_shiftcount[bus] = color_shift;
            merge.mau_payload_shifter_enable[1][bus].stats_adr_payload_shifter_en = 1;
        }
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void MeterTable::setup_tcam_shift, mau_regs &,
                      int, int, Call &, Call &);

template <class REGS>
void MeterTable::write_regs_home_row(REGS &regs, unsigned row) {
    auto &map_alu = regs.rams.map_alu;
    auto &map_alu_row = map_alu.row[row];
    auto &adrdist = regs.rams.match.adrdist;
    unsigned side = 1;  // Meter can only be on right side
    int minvpn, maxvpn;
    layout_vpn_bounds(minvpn, maxvpn, true);

    if (home_rows.size() > 1) {
        int sparevpn;
        layout_vpn_bounds(minvpn, sparevpn, false);
        bool block_start = false;
        bool block_end = false;
        minvpn = INT_MAX;
        maxvpn = INT_MIN;
        for (Layout &logical_row : layout) {
            // Block Start with the home row and End with the Spare VPN
            if (logical_row.row / 2U == row) block_start = true;

            if (block_start) {
                for (auto v : logical_row.vpns) {
                    if (v == sparevpn) {
                        block_end = true;
                        break;
                    }

                    if (v < minvpn) minvpn = v;
                    if (v > maxvpn) maxvpn = v;
                }
            }
            if (block_end) {
                BUG_CHECK(minvpn != INT_MAX && maxvpn != INT_MIN);
                break;
            }
        }
        BUG_CHECK(block_start && block_end);
    }

    int meter_group_index = row / 2U;
    auto &meter = map_alu.meter_group[meter_group_index].meter;
    auto &meter_ctl = meter.meter_ctl;
    auto &red_value_ctl = meter.red_value_ctl;

    int first_home_row = *home_rows.begin();
    if (count == BYTES) {
        auto meter_bytecount_adjust_size = meter_ctl.meter_bytecount_adjust.size();
        auto meter_bytecount_adjust_mask = ((1U << meter_bytecount_adjust_size) - 1);
        int bytecount_adjust_max = (1U << (meter_bytecount_adjust_size - 1)) - 1;
        int bytecount_adjust_min = -1 * (1U << (meter_bytecount_adjust_size - 1));
        if (bytecount_adjust > bytecount_adjust_max || bytecount_adjust < bytecount_adjust_min) {
            error(lineno,
                  "The bytecount adjust value of %d on meter %s "
                  "does not fit within allowed range for %d bits - { %d, %d }",
                  bytecount_adjust, name(), meter_bytecount_adjust_size, bytecount_adjust_min,
                  bytecount_adjust_max);
        }
        meter_ctl.meter_bytecount_adjust = bytecount_adjust & meter_bytecount_adjust_mask;
    }
    auto &delay_ctl = map_alu.meter_alu_group_data_delay_ctl[meter_group_index];
    delay_ctl.meter_alu_right_group_delay =
        Target::METER_ALU_GROUP_DATA_DELAY() + row / 4 + stage->tcam_delay(gress);
    switch (type) {
        case LPF:
            meter_ctl.lpf_enable = 1;
            delay_ctl.meter_alu_right_group_enable = 1;
            break;
        case RED:
            meter_ctl.lpf_enable = 1;
            meter_ctl.red_enable = 1;
            delay_ctl.meter_alu_right_group_enable = 1;
            red_value_ctl.red_nodrop_value = red_nodrop_value;
            red_value_ctl.red_drop_value = red_drop_value;
            break;
        default:
            meter_ctl.meter_enable = 1;
            // RNG:
            // Enables random number generator for meter probabilistic charging
            // when green/yellow burst size exponent > 14.  This should be set
            // when any meter entry in the table has a burstsize exponent > 14
            // RNG is also enabled whenever red_enable config bit is set.

            // this should always be turned on
            // for color-based meters, to handle an issue with large burst
            // sizes.  This applies to both packet-based and byte-based meters.
            // Mike F said, "The hardware adjusts the rate under the hood to
            // match the desired rate. Without enabling the RNG, the hardware
            // will always overcharge the buckets thereby reducing the rate."
            meter_ctl.meter_rng_enable = 1;
            meter_ctl.meter_time_scale = profile;
            break;
    }
    if (count == BYTES) meter_ctl.meter_byte = 1;
    if (gress == EGRESS) meter_ctl.meter_alu_egress = 1;
    auto &error_ctl = map_alu.meter_alu_group_error_ctl[meter_group_index];
    error_ctl.meter_alu_group_ecc_error_enable = 1;
    error_ctl.meter_alu_group_thread = gress;
    auto &meter_sweep_ctl = adrdist.meter_sweep_ctl[meter_group_index];
    // The driver will manage turning on the meter sweep enable,
    // so the compiler should not configure this value (check glass
    // code)
    // meter_sweep_ctl.meter_sweep_en = 1;
    meter_sweep_ctl.meter_sweep_offset = minvpn;
    meter_sweep_ctl.meter_sweep_size = maxvpn;
    meter_sweep_ctl.meter_sweep_remove_hole_pos = 0;  // FIXME -- see CSR?
    meter_sweep_ctl.meter_sweep_remove_hole_en = 0;   // FIXME
    meter_sweep_ctl.meter_sweep_interval = sweep_interval + profile;
    for (auto &ixb : input_xbar) {
        auto &vh_adr_xbar = regs.rams.array.row[row].vh_adr_xbar;
        auto &data_ctl = regs.rams.array.row[row].vh_xbar[side].stateful_meter_alu_data_ctl;
        // FIXME: Currently in the compiler, the data headed to the meter alu/stateful alu
        // can only come from hash or the search bus, but not both, thus it is
        // currenlty safe for them to be mutually exclusive.  If the compiler was to
        // allocate fields to both, this would have to interpret the information
        // correctly
        auto hashdata_bytemask = bitmask2bytemask(ixb->hash_group_bituse());
        if (hashdata_bytemask != 0U) {
            vh_adr_xbar.alu_hashdata_bytemask.alu_hashdata_bytemask_right = hashdata_bytemask;
            setup_muxctl(vh_adr_xbar.exactmatch_row_hashadr_xbar_ctl[2 + side], ixb->hash_group());
        } else {
            // FIXME: Need to be some validation between Tofino and JBay if the input
            // xbar is valid for these meters.
            bitvec bytemask = ixb->bytemask();
            bytemask >>= bytemask.min().index();
            unsigned u_bytemask = bytemask.getrange(0, bytemask.max().index() + 1);
            data_ctl.stateful_meter_alu_data_bytemask = u_bytemask;
            data_ctl.stateful_meter_alu_data_xbar_ctl = 8 | ixb->match_group();
        }
    }
    if (output_used) {
        auto &action_ctl = map_alu.meter_alu_group_action_ctl[meter_group_index];
        action_ctl.right_alu_action_enable = 1;
        action_ctl.right_alu_action_delay = stage->meter_alu_delay(gress, false);
        auto &switch_ctl = regs.rams.array.switchbox.row[row].ctl;
        switch_ctl.r_action_o_mux_select.r_action_o_sel_action_rd_r_i = 1;
        // disable action data address huffman decoding, on the assumtion we're not
        // trying to combine this with an action data table on the same home row.
        // Otherwise, the huffman decoding will think this is an 8-bit value and
        // replicate it.
        regs.rams.array.row[row]
            .action_hv_xbar.action_hv_xbar_disable_ram_adr.action_hv_xbar_disable_ram_adr_right = 1;
    }
    map_alu_row.i2portctl.synth2port_vpn_ctl.synth2port_vpn_base = minvpn;
    map_alu_row.i2portctl.synth2port_vpn_ctl.synth2port_vpn_limit = maxvpn;
    auto &movereg_meter_ctl = adrdist.movereg_meter_ctl[meter_group_index];
    if (run_at_eop()) movereg_meter_ctl.movereg_meter_ctl_deferred = 1;
    movereg_meter_ctl.movereg_ad_meter_shift = 7;
    movereg_meter_ctl.movereg_meter_ctl_lt = logical_id;
    if (direct) movereg_meter_ctl.movereg_meter_ctl_direct = 1;
    movereg_meter_ctl.movereg_meter_ctl_color_en = 1;
    for (MatchTable *m : match_tables) {
        if (direct) adrdist.movereg_ad_direct[1] |= 1U << m->logical_id;
        // The first ALU will drive this xbar register
        if (first_home_row / 4U == meter_group_index) {
            adrdist.movereg_ad_meter_alu_to_logical_xbar_ctl[m->logical_id / 8U].set_subfield(
                4 | meter_group_index, 3 * (m->logical_id % 8U), 3);
        }
    }
}

template <class REGS>
void MeterTable::write_mapram_color_regs(REGS &regs, bool &push_on_overflow) {
    auto &map_alu = regs.rams.map_alu;
    auto &adrdist = regs.rams.match.adrdist;
    auto &merge = regs.rams.match.merge;
    int curr_home_row = -1;

    for (Layout &row : color_maprams) {
        curr_home_row = get_home_row_for_row(row.row * 2);
        // Allocating color maprams above home row is invalid
        // as color writes can only be distributed to maprams
        // via buses going on the home row or below
        BUG_CHECK(curr_home_row / 4U >= row.row / 2U);

        int color_map_color = color_maprams.empty() ? 0 : (curr_home_row / 4U) & 1;
        if (row.row == curr_home_row / 2) { /* on the home row */
            if (color_map_color)
                map_alu.mapram_color_switchbox.row[row.row]
                    .ctl.r_color1_mux_select.r_color1_sel_color_r_i = 1;
            else
                map_alu.mapram_color_switchbox.row[row.row]
                    .ctl.r_color0_mux_select.r_color0_sel_color_r_i = 1;
        } else if (row.row / 4U == curr_home_row / 8U) { /* same half as home */
            if (color_map_color)
                map_alu.mapram_color_switchbox.row[row.row]
                    .ctl.r_color1_mux_select.r_color1_sel_oflo_color_r_i = 1;
            else
                map_alu.mapram_color_switchbox.row[row.row]
                    .ctl.r_color0_mux_select.r_color0_sel_oflo_color_r_i = 1;
        } else { /* other half from home */
            map_alu.mapram_color_switchbox.row[row.row].ctl.t_oflo_color_o_mux_select = 1;
            merge.mau_match_central_mapram_read_color_oflo_ctl |= 1U << color_map_color;
        }

        /*
         * Below diagrams show how select bits are set to
         * route color data from meter alu located on the home
         * row down to the color maprams
         * *********************************************
         * - ROUTE FROM RIGHT TO BOTTOM
         * Bus coming from Meter ALU on current home row
         *   .------------
         *   | r_color_write_i
         *   v
         * .---.
         * |   |<---- (select = 1'b1)
         * |   | b_oflo_color_write_o_sel_r_color_write_i
         * .___.
         *   |
         *   | b_oflo_color_write_o
         *   v
         * Bus going to color map rams below
         *
         * *********************************************
         * - ROUTE FROM TOP TO BOTTOM
         * Bus coming from home row above
         *   |
         *   | t_oflo_color_write_i
         *   v
         * .---.
         * |   |<---- (select = 1'b1)
         * |   | b_oflo_color_write_o_sel_t_oflo_color_write_i
         * .___.
         *   |
         *   | b_oflo_color_write_o
         *   v
         * Bus going to color map rams below
         *
         * *********************************************
         * - ROUTE FROM TOP TO RIGHT
         * Bus coming from home row above
         *   |
         *   | t_oflo_color_write_i
         *   v
         * .---.
         * |   |<---- (select = 1'b1)
         * |   | r_oflo_color_write_o_mux_select
         * .___.
         *   |
         *   | r_oflo_color_write_o
         *   .---------------->
         * Bus going to color map rams on right
         *
         * *********************************************
         *
         * A - Meter 1 Map Rams
         * a - Meter 1 Color Map Rams
         * B - Meter 1 Map Rams
         * b - Meter 1 Color Map Rams
         *
         * Log Phy            Columns            SW   Mtr
         * Row Row   0    1    2    3    4    5  Box  ALU
         *         .---..---..---..---..---..---.
         *  15  7  | A || A || A || A || A || A | 3    3
         *         .___..___..___..___..___..___.
         *         .---..---..---..---..---..---.
         *  13  6  | A || A || A || A || a || a |
         *         .___..___..___..___..___..___.
         *         .---..---..---..---..---..---.
         *  11  5  | B || B || B || B || B || a | 2    2
         *         .___..___..___..___..___..___.
         *         .---..---..---..---..---..---.
         *   9  4  | B || B || B || B || B || b |
         *         .___..___..___..___..___..___.
         *         .---..---..---..---..---..---.
         *   7  3  | b || b || - || - || - || - | 1    1
         *         .___..___..___..___..___..___.
         *
         * Meter Color Write Switchbox is configured to
         * - set b_oflo_color_write_o_sel_r_color_write_i (1'b1)
         *   This routes meter alu 3 data down to rows 6 & 5 where
         *   meter 1 color maprams are located [6,4] [6,5] [5,5]
         *
         * Meter ALU 2 is configured to
         * - set b_oflo_color_write_o_sel_t_oflo_color_write_i (1'b1)
         *   This routes meter alu data from above to the
         *   meter 1 color mapram located at [5,5]
         * - set b_oflo_color_write_o_sel_r_color_write_i (1'b1)
         *   This routes meter alu 2 data down to rows 4 & 3 where
         *   meter 2 color maprams are located [4,5] [3,0] [3,1]
         */
        if (row.row != curr_home_row / 2) { /* ALU home row */
            map_alu.mapram_color_write_switchbox[curr_home_row / 4U]
                .ctl.b_oflo_color_write_o_mux_select.b_oflo_color_write_o_sel_r_color_write_i = 1;
            map_alu.mapram_color_write_switchbox[row.row / 2U].ctl.r_oflo_color_write_o_mux_select =
                1;
            BUG_CHECK(curr_home_row / 4U >= row.row / 2U);
            /* b_oflo_color_write_o_sel_t_oflo_color_write_i must be set for all
             * switchboxes below the homerow and above current row
             * It should never be set for a switchbox above the home row
             * It should never be set on the switchbox on the current row
             * as that would drive the top overflow down to any color maprams below.
             * This is invalid and can cause corruption if there is another meter occupying
             * color maprams on the below row.
             */
            // Switch box below home row
            int switchbox_upper = curr_home_row / 4U - 1;
            // Switch box above current row
            int switchbox_lower = row.row % 2 ? (int)row.row / 2U + 1 : (int)row.row / 2U;
            for (int i = switchbox_upper; i >= switchbox_lower; i--) {
                if (i == 3) continue;  // Never set on top switchbox

                map_alu.mapram_color_write_switchbox[i]
                    .ctl.b_oflo_color_write_o_mux_select
                    .b_oflo_color_write_o_sel_t_oflo_color_write_i = 1;
            }
        }
        auto &map_alu_row = map_alu.row[row.row];
        auto vpn = row.vpns.begin();
        if (color_mapram_addr == STATS_MAP_ADDR) {
            BUG_CHECK((row.row % 2) == 0);
            for (MatchTable *m : match_tables)
                adrdist.mau_ad_stats_virt_lt[row.row / 2] |= (1U << m->logical_id);
        }
        // Enable the row to be used (even if only color maprams are on this row)
        map_alu_row.i2portctl.synth2port_ctl.synth2port_enable = 1;
        // If the color mapram is not on the same row as the meter ALU, even if no meter
        // RAMs are on the same row, the address still needs to overflow to that row
        if (row.row < curr_home_row / 2) {
            auto &adr_ctl = map_alu_row.vh_xbars.adr_dist_oflo_adr_xbar_ctl[1];
            // Mapram rows are 0-7, not 0-15 like logical rows
            if (curr_home_row >= UPPER_MATCH_CENTRAL_FIRST_LOGICAL_ROW &&
                row.row < UPPER_MATCH_CENTRAL_FIRST_ROW) {
                adr_ctl.adr_dist_oflo_adr_xbar_source_index = 0;
                adr_ctl.adr_dist_oflo_adr_xbar_source_sel = AdrDist::OVERFLOW;
                push_on_overflow = true;
                BUG_CHECK(options.target == TOFINO);
            } else {
                adr_ctl.adr_dist_oflo_adr_xbar_source_index = curr_home_row % 8;
                adr_ctl.adr_dist_oflo_adr_xbar_source_sel = AdrDist::METER;
            }
            adr_ctl.adr_dist_oflo_adr_xbar_enable = 1;
        }

        for (auto &memunit : row.memunits) {
            int col = memunit.col;
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == row.row, "bogus %s in row %d",
                      memunit.desc(), row.row);
            auto &mapram_config = map_alu_row.adrmux.mapram_config[col];
            if (row.row == curr_home_row / 2)
                mapram_config.mapram_color_bus_select = MapRam::ColorBus::COLOR;
            else
                mapram_config.mapram_color_bus_select = MapRam::ColorBus::OVERFLOW;
            mapram_config.mapram_type = MapRam::COLOR;
            mapram_config.mapram_logical_table = logical_id;
            BUG_CHECK(vpn != row.vpns.end(), "vpn not found!");
            mapram_config.mapram_vpn = *vpn;
            // These two registers must be programmed for meter-color map rams in this way as a
            // work-around for hardware issue as described in TOF-1944
            // The basic problem is that software reads of the meter color map ram are only
            // returning 6-bits of data instead of the necessary 8-bits.  Hardware defaults to
            // 6 bits, since the meter color map ram case is not explicitly called out.
            // By setting these bits, all 8-bits will be returned.
            mapram_config.mapram_parity_generate = 1;
            mapram_config.mapram_parity_check = 0;
            // glass does not set ecc for color maprams?
            // mapram_config.mapram_ecc_check = 1;
            // mapram_config.mapram_ecc_generate = 1;
            if (gress == INGRESS)
                mapram_config.mapram_ingress = 1;
            else
                mapram_config.mapram_egress = 1;
            mapram_config.mapram_enable = 1;
            if (row.row != curr_home_row / 2) { /* ALU home row */
                mapram_config.mapram_color_write_bus_select = 1;
            }
            auto &ram_address_mux_ctl = map_alu_row.adrmux.ram_address_mux_ctl[1][col];
            if (row.row == curr_home_row / 2) { /* ALU home row */
                ram_address_mux_ctl.synth2port_radr_mux_select_home_row = 1;
            } else {
                ram_address_mux_ctl.synth2port_radr_mux_select_oflo = 1;
            }
            map_alu_row.i2portctl.synth2port_ctl.synth2port_mapram_color |= 1U << col;
            ram_address_mux_ctl.map_ram_wadr_shift = 1;
            ram_address_mux_ctl.map_ram_wadr_mux_select = MapRam::Mux::COLOR;
            ram_address_mux_ctl.map_ram_wadr_mux_enable = 1;
            ram_address_mux_ctl.map_ram_radr_mux_select_color = 1;
            ram_address_mux_ctl.ram_ofo_stats_mux_select_statsmeter = 1;
            // Indicating what bus to pull from, either stats or idletime for the color mapram
            if (color_mapram_addr == IDLE_MAP_ADDR) {
                ram_address_mux_ctl.ram_stats_meter_adr_mux_select_idlet = 1;
                setup_muxctl(map_alu_row.vh_xbars.adr_dist_idletime_adr_xbar_ctl[col],
                             row.bus.at(Layout::IDLE_BUS) % 10);
            } else if (color_mapram_addr == STATS_MAP_ADDR) {
                ram_address_mux_ctl.ram_stats_meter_adr_mux_select_stats = 1;
            }
            if (gress)
                regs.cfg_regs.mau_cfg_mram_thread[col / 3U] |= 1U << (col % 3U * 8U + row.row);
            ++vpn;
        }
    }

    // Additional BUG_CHECK to verify that both these regs are not set on a switchbox
    // - map_alu.mapram_color_write_switchbox[x].ctl.b_oflo_color_write_o_sel_r_color_write_i
    // - map_alu.mapram_color_write_switchbox[x].ctl.b_oflo_color_write_o_sel_t_oflo_color_write_i
    // Both these regs should never be set on a swithbox as it implies routing from both top and
    // right map alu to the bottom rows. This leads to corruption of color data.
    // Additional BUG_CHECK to verify that top row switchbox does not have
    // this regs set
    // - map_alu.mapram_color_write_switchbox[x].ctl.b_oflo_color_write_o_sel_t_oflo_color_write_i
    for (int i = 0; i <= 3; i++) {
        auto t_oflo_write_i = map_alu.mapram_color_write_switchbox[i]
                                  .ctl.b_oflo_color_write_o_mux_select
                                  .b_oflo_color_write_o_sel_t_oflo_color_write_i == 1;
        if (i == 3) {
            BUG_CHECK(!t_oflo_write_i,
                      "Color maprams have invalid configuration"
                      " may cause corruption of color data from meter");
        }
        auto r_oflo_write_i =
            map_alu.mapram_color_write_switchbox[i]
                .ctl.b_oflo_color_write_o_mux_select.b_oflo_color_write_o_sel_r_color_write_i == 1;
        LOG5("i: " << i << "t_oflo: " << t_oflo_write_i << ", r_oflo: " << r_oflo_write_i);
        BUG_CHECK(!(t_oflo_write_i & r_oflo_write_i),
                  "Color maprams have invalid configuration"
                  " may cause corruption of color data from meter");
    }
}

template <class REGS>
void MeterTable::write_regs_vt(REGS &regs) {
    LOG1("### Meter table " << name() << " write_regs " << loc());
    for (auto &ixb : input_xbar) ixb->write_regs(regs);
    Layout *home = nullptr;
    bool push_on_overflow = false;
    auto &map_alu = regs.rams.map_alu;
    auto &adrdist = regs.rams.match.adrdist;
    DataSwitchboxSetup<REGS> *swbox = nullptr;
    for (Layout &logical_row : layout) {
        unsigned row = logical_row.row / 2U;
        unsigned side = logical_row.row & 1; /* 0 == left  1 == right */
        BUG_CHECK(side == 1);                /* no map rams or alus on left side anymore */
        auto vpn = logical_row.vpns.begin();
        auto mapram = logical_row.maprams.begin();
        auto &map_alu_row = map_alu.row[row];
        auto home_it = home_rows.find(logical_row.row);
        if (home_it != home_rows.end()) {
            home = &logical_row;
            swbox = new DataSwitchboxSetup<REGS>(regs, this, logical_row.row,
                                                 (++home_it == home_rows.end()) ? -1 : *home_it);
        }
        BUG_CHECK(home != nullptr);
        LOG2("# DataSwitchbox.setup_row(" << row << ") home=" << home->row / 2U);
        swbox->setup_row(row);
        for (auto &memunit : logical_row.memunits) {
            int logical_col = memunit.col;
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == logical_row.row,
                      "bogus %s in logical row %d", memunit.desc(), logical_row.row);
            unsigned col = logical_col + 6 * side;
            LOG2("# DataSwitchbox.setup_row_col(" << row << ", " << col << ", vpn=" << *vpn
                                                  << ") home=" << home->row / 2U);
            swbox->setup_row_col(row, col, *vpn);
            write_mapram_regs(regs, row, *mapram, *vpn, MapRam::METER);
            if (gress) regs.cfg_regs.mau_cfg_uram_thread[col / 4U] |= 1U << (col % 4U * 8U + row);
            ++mapram, ++vpn;
        }
        if (&logical_row == home) {
            write_regs_home_row(regs, row);
        } else {
            auto &adr_ctl = map_alu_row.vh_xbars.adr_dist_oflo_adr_xbar_ctl[side];
            if (home->row >= UPPER_MATCH_CENTRAL_FIRST_LOGICAL_ROW &&
                logical_row.row < UPPER_MATCH_CENTRAL_FIRST_LOGICAL_ROW) {
                adr_ctl.adr_dist_oflo_adr_xbar_source_index = 0;
                adr_ctl.adr_dist_oflo_adr_xbar_source_sel = AdrDist::OVERFLOW;
                push_on_overflow = true;
                BUG_CHECK(options.target == TOFINO);
            } else {
                adr_ctl.adr_dist_oflo_adr_xbar_source_index = home->row % 8;
                adr_ctl.adr_dist_oflo_adr_xbar_source_sel = AdrDist::METER;
            }
            adr_ctl.adr_dist_oflo_adr_xbar_enable = 1;
        }
    }
    auto &merge = regs.rams.match.merge;
    write_mapram_color_regs(regs, push_on_overflow);
    if (home_rows.size() > 1) write_alu_vpn_range(regs);

    for (int home_row : home_rows) {
        for (MatchTable *m : match_tables) {
            adrdist.adr_dist_meter_adr_icxbar_ctl[m->logical_id] |= 1U << (home_row / 4U);
            // auto &icxbar = adrdist.adr_dist_meter_adr_icxbar_ctl[m->logical_id];
            // icxbar.address_distr_to_logical_rows = 1U << home->row;
            // icxbar.address_distr_to_overflow = push_on_overflow;
            // if (direct)
            //     regs.cfg_regs.mau_cfg_lt_meter_are_direct |= 1 << m->logical_id;
            adrdist.meter_color_output_map[m->logical_id].set_subfield(green_value, 0, 8);
            adrdist.meter_color_output_map[m->logical_id].set_subfield(yellow_value, 8, 8);
            adrdist.meter_color_output_map[m->logical_id].set_subfield(yellow_value, 16, 8);
            adrdist.meter_color_output_map[m->logical_id].set_subfield(red_value, 24, 8);
            if (type != LPF) adrdist.meter_enable |= 1U << m->logical_id;
            /*auto &movereg_ad_ctl = adrdist.movereg_ad_ctl[m->logical_id];
            movereg_ad_ctl.movereg_meter_deferred = 1;
            if (!color_maprams.empty())
                movereg_ad_ctl.movereg_ad_idle_as_mc = 1;
            else
                movereg_ad_ctl.movereg_ad_stats_as_mc = 1;
            movereg_ad_ctl.movereg_ad_direct_meter = direct;
            movereg_ad_ctl.movereg_ad_meter_shift = 7; */
            meter_color_logical_to_phys(regs, m->logical_id, home_row / 4U);
            adrdist.mau_ad_meter_virt_lt[home_row / 4U] |= 1 << m->logical_id;
        }
        if (run_at_eop()) {
            if (teop >= 0) {
                setup_teop_regs(regs, home_row / 4U);
            } else {
                adrdist.deferred_ram_ctl[1][home_row / 4U].deferred_ram_en = 1;
                adrdist.deferred_ram_ctl[1][home_row / 4U].deferred_ram_thread = gress;
                if (gress) regs.cfg_regs.mau_cfg_dram_thread |= 0x10 << (home_row / 4U);
            }
            adrdist.meter_bubble_req[timing_thread(gress)].bubble_req_1x_class_en |=
                1 << ((home_row / 4U) + 4);
        } else {
            adrdist.meter_bubble_req[timing_thread(gress)].bubble_req_1x_class_en |=
                1 << (home_row / 4U);
            adrdist.packet_action_at_headertime[1][home_row / 4U] = 1;
        }
        if (push_on_overflow) {
            adrdist.oflo_adr_user[0] = adrdist.oflo_adr_user[1] = AdrDist::METER;
            adrdist.deferred_oflo_ctl = 1 << ((home_row - 8) / 2U);
        }
        if (gress == INGRESS || gress == GHOST) {
            merge.meter_alu_thread[0].meter_alu_thread_ingress |= 1U << home_row / 4U;
            merge.meter_alu_thread[1].meter_alu_thread_ingress |= 1U << home_row / 4U;
        } else if (gress == EGRESS) {
            merge.meter_alu_thread[0].meter_alu_thread_egress |= 1U << home_row / 4U;
            merge.meter_alu_thread[1].meter_alu_thread_egress |= 1U << home_row / 4U;
        }
    }

    for (auto &hd : hash_dist) hd.write_regs(regs, this);
}

// FIXME -- refactor these specializations better
template <>
void MeterTable::meter_color_logical_to_phys(Target::Tofino::mau_regs &regs, int logical_id,
                                             int alu) {
    auto &merge = regs.rams.match.merge;
    auto &adrdist = regs.rams.match.adrdist;
    if (!color_maprams.empty()) {
        merge.mau_mapram_color_map_to_logical_ctl[logical_id / 8].set_subfield(
            0x4 | alu, 3 * (logical_id % 8U), 3);

        // Determining which buses to send the color mapram address to
        if (color_mapram_addr == IDLE_MAP_ADDR) {
            adrdist.movereg_idle_ctl[logical_id].movereg_idle_ctl_mc = 1;
            for (auto lo : color_maprams) {
                int bus_index = lo.bus.at(Layout::IDLE_BUS);
                // upper and lower idletime busses appear to be independent with
                // no overflow between them
                if (lo.row >= UPPER_MATCH_CENTRAL_FIRST_ROW) bus_index += IDLETIME_BUSSES_PER_HALF;
                adrdist.adr_dist_idletime_adr_oxbar_ctl[bus_index / 4].set_subfield(
                    logical_id | 0x10, 5 * (bus_index % 4), 5);
            }

        } else if (color_mapram_addr == STATS_MAP_ADDR) {
            for (auto lo : color_maprams) {
                adrdist.adr_dist_stats_adr_icxbar_ctl[logical_id] |= (1U << (lo.row / 2));
                adrdist.packet_action_at_headertime[0][lo.row / 2] = 1;
            }
        } else {
            BUG();
        }
        setup_muxctl(adrdist.meter_color_logical_to_phys_ixbar_ctl[logical_id], alu);
    }
}

template <>
void MeterTable::meter_color_logical_to_phys(Target::JBay::mau_regs &regs, int logical_id,
                                             int alu) {
    auto &merge = regs.rams.match.merge;
    auto &adrdist = regs.rams.match.adrdist;
    if (!color_maprams.empty()) {
        merge.mau_mapram_color_map_to_logical_ctl[alu] |= 1 << logical_id;
        // Determining which buses to send the color mapram address to
        if (color_mapram_addr == IDLE_MAP_ADDR) {
            adrdist.movereg_idle_ctl[logical_id].movereg_idle_ctl_mc = 1;
            for (auto lo : color_maprams) {
                int bus_index = lo.bus.at(Layout::IDLE_BUS);
                // No overflow bus exist between upper and lower half so every color mapram have
                // to use their respective bus
                if (lo.row >= UPPER_MATCH_CENTRAL_FIRST_ROW) bus_index += IDLETIME_BUSSES_PER_HALF;
                adrdist.adr_dist_idletime_adr_oxbar_ctl[bus_index / 4].set_subfield(
                    logical_id | 0x10, 5 * (bus_index % 4), 5);
            }

        } else if (color_mapram_addr == STATS_MAP_ADDR) {
            for (auto lo : color_maprams) {
                adrdist.adr_dist_stats_adr_icxbar_ctl[logical_id] |= (1U << (lo.row / 2));
                adrdist.packet_action_at_headertime[0][lo.row / 2] = 1;
            }
        } else {
            BUG();
        }
    }
    adrdist.meter_color_logical_to_phys_icxbar_ctl[logical_id] |= 1 << alu;
}

void MeterTable::gen_tbl_cfg(json::vector &out) const {
    // FIXME -- factor common Synth2Port stuff
    auto spare_mems = determine_spare_bank_memory_units();
    int size = (layout_size() - spare_mems.size()) * SRAM_DEPTH;
    json::map &tbl = *base_tbl_cfg(out, "meter", size);
    json::map &stage_tbl = *add_stage_tbl_cfg(tbl, "meter", size);
    stage_tbl["color_memory_resource_allocation"] =
        gen_memory_resource_allocation_tbl_cfg("map_ram", color_maprams);
    switch (type) {
        case STANDARD:
            tbl["meter_type"] = "standard";
            tbl["meter_profile"] = profile;
            break;
        case LPF:
            tbl["meter_type"] = "lpf";
            break;
        case RED:
            tbl["meter_type"] = "red";
            break;
        default:
            tbl["meter_type"] = "standard";
            break;
    }
    switch (count) {
        case PACKETS:
            tbl["meter_granularity"] = "packets";
            break;
        case BYTES:
            tbl["meter_granularity"] = "bytes";
            break;
        default:
            tbl["meter_granularity"] = "packets";
            break;
    }
    tbl["enable_color_aware_pfe"] = color_aware_per_flow_enable;
    /* this is not needed. but the driver asserts on existence of
     * this or enable_color_aware which both seem to be redundant */
    tbl["pre_color_field_name"] = "";
    tbl["enable_pfe"] = per_flow_enable;
    tbl["pfe_bit_position"] = per_flow_enable_bit();
    tbl["color_aware_pfe_address_type_bit_position"] = 0;  // FIXME
    tbl["reference_dictionary"] = json::map();             // To be removed in future
    stage_tbl["default_lower_huffman_bits_included"] = METER_LOWER_HUFFMAN_BITS;
    if (home_rows.size() > 1)
        add_alu_indexes(stage_tbl, "meter_alu_index");
    else
        add_alu_index(stage_tbl, "meter_alu_index");
    if (context_json) stage_tbl.merge(*context_json);
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(MeterTable, TARGET_CLASS)
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void MeterTable::write_merge_regs,
                      (mau_regs & regs, MatchTable *match, int type, int bus,
                       const std::vector<Call::Arg> &args),
                      { write_merge_regs_vt(regs, match, type, bus, args); })
