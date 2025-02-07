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

#include "tofino/stateful.h"

#include "algorithm.h"
#include "data_switchbox.h"
#include "input_xbar.h"
#include "instruction.h"
#include "jbay/stateful.h"
#include "misc.h"
#include "stage.h"
#include "tables.h"

void StatefulTable::parse_register_params(int idx, const value_t &val) {
    if (idx < 0 || idx > Target::STATEFUL_REGFILE_ROWS())
        error(lineno,
              "Index out of range of the number of the register file rows (%d). "
              "Reduce the number of large constants or RegisterParams.",
              Target::STATEFUL_REGFILE_ROWS());
    if (const_vals.size() <= size_t(idx)) const_vals.resize(idx + 1);
    if (CHECKTYPE(val, tMAP) && val.map.size == 1)
        if (CHECKTYPE(val.map.data->key, tSTR) && CHECKTYPE(val.map.data->value, tINT))
            const_vals[idx] = std::move(const_info_t(val.lineno, val.map.data->value.i, true,
                                                     std::string(val.map.data->key.s)));
}

void StatefulTable::setup(VECTOR(pair_t) & data) {
    common_init_setup(data, false, P4Table::Stateful);
    if (!format) error(lineno, "No format specified in table %s", name());
    for (auto &kv : MapIterChecked(data, true)) {
        if (common_setup(kv, data, P4Table::Stateful)) {
        } else if (kv.key == "initial_value") {
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &v : kv.value.map) {
                    if (v.key == "lo") {
                        if (CHECKTYPE2(v.value, tINT, tBIGINT)) {
                            if (v.value.type == tINT) {
                                initial_value_lo = v.value.i;
                            } else {
                                initial_value_lo = v.value.bigi.data[0];
                                if (v.value.bigi.size > 1) initial_value_hi = v.value.bigi.data[1];
                            }
                        }
                    } else if (v.key == "hi") {
                        if (CHECKTYPE(v.value, tINT)) initial_value_hi = v.value.i;
                    }
                }
            }
        } else if (kv.key == "input_xbar") {
            if (CHECKTYPE(kv.value, tMAP))
                input_xbar.emplace_back(InputXbar::create(this, false, kv.key, kv.value.map));
        } else if (kv.key == "data_bytemask") {
            if (CHECKTYPE(kv.value, tINT)) data_bytemask = kv.value.i;
        } else if (kv.key == "hash_bytemask") {
            if (CHECKTYPE(kv.value, tINT)) hash_bytemask = kv.value.i;
        } else if (kv.key == "hash_dist") {
            /* parsed in common_init_setup */
        } else if (kv.key == "actions") {
            if (CHECKTYPE(kv.value, tMAP)) actions.reset(new Actions(this, kv.value.map));
        } else if (kv.key == "selection_table") {
            bound_selector = kv.value;
        } else if (kv.key == "register_params") {
            if (!CHECKTYPE2(kv.value, tVEC, tMAP)) continue;
            if (kv.value.type == tVEC) {
                for (auto &v : kv.value.vec) parse_register_params(const_vals.size(), v);
            } else {
                for (auto &v : kv.value.map)
                    if (CHECKTYPE(v.key, tINT)) parse_register_params(v.key.i, v.value);
            }
        } else if (kv.key == "math_table") {
            if (!CHECKTYPE(kv.value, tMAP)) continue;
            math_table.lineno = kv.value.lineno;
            for (auto &v : kv.value.map) {
                if (v.key == "data") {
                    if (!CHECKTYPE2(v.value, tVEC, tMAP)) continue;
                    if (v.value.type == tVEC) {
                        parse_vector(math_table.data, v.value);
                    } else {
                        math_table.data.resize(16);
                        for (auto &d : v.value.map)
                            if (CHECKTYPE(d.key, tINT) && CHECKTYPE(d.value, tINT)) {
                                if (d.key.i < 0 || d.key.i >= 16)
                                    error(v.key.lineno, "invalid index for math_table");
                                else
                                    math_table.data[v.key.i] = v.value.i;
                            }
                    }
                } else if (v.key == "invert") {
                    math_table.invert = get_bool(v.value);
                } else if (v.key == "shift") {
                    if (CHECKTYPE(v.value, tINT)) math_table.shift = v.value.i;
                } else if (v.key == "scale") {
                    if (CHECKTYPE(v.value, tINT)) math_table.scale = v.value.i;
                } else if (v.key.type == tINT && v.key.i >= 0 && v.key.i < 16) {
                    if (CHECKTYPE(v.value, tINT)) math_table.data[v.key.i] = v.value.i;
                } else {
                    error(v.key.lineno, "Unknow item %s in math_table", value_desc(kv.key));
                }
            }
#if HAVE_JBAY
        } else if (options.target >= JBAY && setup_jbay(kv)) {
            /* jbay specific extensions done in setup_jbay */
            // FIXME -- these should probably be based on individual Target::FEATURE() queries
#endif /* HAVE_JBAY */
        } else if (kv.key == "log_vpn") {
            logvpn_lineno = kv.value.lineno;
            if (CHECKTYPE2(kv.value, tINT, tRANGE)) {
                if (kv.value.type == tINT) {
                    logvpn_min = logvpn_max = kv.value.i;
                } else {
                    logvpn_min = kv.value.lo;
                    logvpn_max = kv.value.hi;
                }
            }
        } else if (kv.key == "pred_shift") {
            if (CHECKTYPE(kv.value, tINT))
                if ((pred_shift = kv.value.i) < 0 || pred_shift >= 32 || (pred_shift & 3) != 0)
                    error(kv.value.lineno, "Invalid pred_shift value %d: %s", pred_shift,
                          pred_shift < 0     ? "negative"
                          : pred_shift >= 32 ? "too large"
                                             : "must be a mulitple of 4");
        } else if (kv.key == "pred_comb_shift") {
            if (CHECKTYPE(kv.value, tINT))
                if ((pred_comb_shift = kv.value.i) < 0 || pred_comb_shift >= 32)
                    error(kv.value.lineno, "Invalid pred_comb_shift value %d: %s", pred_comb_shift,
                          pred_comb_shift < 0 ? "negative" : "too large");
        } else if (kv.key == "busy_value" && Target::SUPPORT_SALU_FAST_CLEAR()) {
            if (CHECKTYPE(kv.value, tINT)) busy_value = kv.value.i;
        } else if (kv.key == "clear_value" && Target::SUPPORT_SALU_FAST_CLEAR()) {
            if (CHECKTYPE2(kv.value, tINT, tBIGINT))
                clear_value = get_bitvec(kv.value, 128, "Value too large for 128 bits");
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
}

bool match_table_layouts(Table *t1, Table *t2) {
    if (t1->layout.size() != t2->layout.size()) return false;
    auto it = t2->layout.begin();
    for (auto &row : t1->layout) {
        if (row.row != it->row) return false;
        if (row.memunits != it->memunits) return false;
        if (row.maprams.empty()) row.maprams = it->maprams;
        if (row.maprams != it->maprams) return false;
        ++it;
    }
    return true;
}

void StatefulTable::MathTable::check() {
    if (data.size() > 16) error(lineno, "math table only has 16 data entries");
    data.resize(16);
    for (auto &v : data)
        if (v < 0 || v >= 256) error(lineno, "%d out of range for math_table data", v);
    if (shift < -1 || shift > 1) error(lineno, "%d out of range for math_table shift", shift);
    if (scale < -32 || scale >= 32) error(lineno, "%d out of range for math_table scale", scale);
}

void StatefulTable::pass1() {
    LOG1("### Stateful table " << name() << " pass1 " << loc());
    if (!p4_table)
        p4_table = P4Table::alloc(P4Table::Stateful, this);
    else
        p4_table->check(this);
    alloc_vpns();
    if (bound_selector.check()) {
        if (layout.empty())
            layout = bound_selector->layout;
        else if (!match_table_layouts(this, bound_selector))
            error(layout[0].lineno, "Layout in %s does not match selector %s", name(),
                  bound_selector->name());
        // Add a back reference to this table
        if (!bound_selector->get_stateful()) bound_selector->set_stateful(this);
        if (logical_id < 0) logical_id = bound_selector->logical_id;
    } else {
        alloc_maprams();
        if (Target::SRAM_GLOBAL_ACCESS())
            alloc_global_srams();
        else
            alloc_rams(true, stage->sram_use);
    }
    std::sort(layout.begin(), layout.end(),
              [](const Layout &a, const Layout &b) -> bool { return a.row > b.row; });
    stage->table_use[timing_thread(gress)] |= Stage::USE_STATEFUL;
    for (auto &hd : hash_dist) hd.pass1(this, HashDistribution::OTHER, false);
    for (auto &ixb : input_xbar) ixb->pass1();
    int prev_row = -1;
    for (auto &row : layout) {
        if (prev_row >= 0)
            need_bus(lineno, stage->overflow_bus_use, row.row, "Overflow");
        else
            need_bus(lineno, stage->meter_bus_use, row.row, "Meter data");
        for (int r = (row.row + 1) | 1; r < prev_row; r += 2)
            need_bus(lineno, stage->overflow_bus_use, r, "Overflow");
        prev_row = row.row;
    }
    unsigned idx = 0, size = 0;
    for (auto &fld : *format) {
        switch (idx++) {
            case 0:
                if ((size = fld.second.size) != 1 && size != 8 && size != 16 && size != 32 &&
                    ((size != 64 && size != 128) || options.target == TOFINO)) {
                    error(format->lineno, "invalid size %d for stateful format field %s", size,
                          fld.first.c_str());
                    break;
                }
                break;
            case 1:
                if (size != fld.second.size)
                    error(format->lineno, "stateful fields must be the same size");
                else if (size == 1)
                    error(format->lineno, "one bit stateful tables can only have a single field");
                break;
            default:
                error(format->lineno, "only two fields allowed in a stateful table");
        }
    }
    if ((idx == 2) && (format->size == 2 * size)) dual_mode = true;
    if (actions) {
        actions->pass1(this);
        bool stop = false;
        for (auto &act : *actions) {
            for (auto &inst : act.instr) {
                if (inst->salu_output()) {
                    need_bus(layout.at(0).lineno, stage->action_data_use, home_row(),
                             "action data");
                    stop = true;
                    break;
                }
            }
            if (stop) break;
        }
    } else {
        error(lineno, "No actions in stateful table %s", name());
    }
    if (math_table) math_table.check();
    for (auto &r : sbus_learn)
        if (r.check() && (r->table_type() != STATEFUL || r->stage != stage))
            error(r.lineno, "%s is not a stateful table in the same stage as %s", r->name(),
                  name());
    for (auto &r : sbus_match)
        if (r.check() && (r->table_type() != STATEFUL || r->stage != stage))
            error(r.lineno, "%s is not a stateful table in the same stage as %s", r->name(),
                  name());
    Synth2Port::pass1();
    if (underflow_action.set() && (!actions || !actions->exists(underflow_action.name)))
        error(underflow_action.lineno, "No action %s in table %s", underflow_action.name.c_str(),
              name());
    if (overflow_action.set() && (!actions || !actions->exists(overflow_action.name)))
        error(overflow_action.lineno, "No action %s in table %s", overflow_action.name.c_str(),
              name());
}

int StatefulTable::get_const(int lineno, int64_t v) {
    size_t rv;
    for (rv = 0; rv < const_vals.size(); rv++) {
        // Skip constants allocated for RegisterParams as they cannot be shared
        // as they are subject to change.
        if (const_vals[rv].is_param) continue;
        if (const_vals[rv].value == v) break;
    }
    if (rv == const_vals.size()) {
        if (rv >= Target::STATEFUL_REGFILE_ROWS())
            error(lineno,
                  "Out of the number of register file rows (%d). Reduce the number"
                  " of large constants or RegisterParams.",
                  Target::STATEFUL_REGFILE_ROWS());
        const_vals.push_back(std::move(const_info_t(lineno, v)));
    }
    return rv;
}

void StatefulTable::pass2() {
    LOG1("### Stateful table " << name() << " pass2 " << loc());
    for (auto &ixb : input_xbar) ixb->pass2();
    if (actions) actions->stateful_pass2(this);
    if (stateful_counter_mode) {
        if (logvpn_min < 0) {
            layout_vpn_bounds(logvpn_min, logvpn_max, true);
        } else if (!offset_vpn) {
            int min, max;
            layout_vpn_bounds(min, max, true);
            if (logvpn_min < min || logvpn_max > max)
                error(logvpn_lineno, "log_vpn out of range (%d..%d)", min, max);
        }
    }

    for (auto &ixb : input_xbar) {
        if (!data_bytemask && !hash_bytemask) {
            hash_bytemask = bitmask2bytemask(ixb->hash_group_bituse()) & phv_byte_mask;
            // should we also mask off bits not set in the ixbar of this table?
            // as long as the compiler explicitly zeroes everything in the hash
            // that needs to be zero, it should be ok.
            data_bytemask = phv_byte_mask & ~hash_bytemask;
        }
    }
    if (input_xbar.empty()) {
        if (data_bytemask || hash_bytemask) {
            error(lineno, "No input_xbar in %s, but %s is present", name(),
                  data_bytemask ? "data_bytemask" : "hash_bytemask");
        } else if (phv_byte_mask) {
            error(lineno, "No input_xbar in %s, but raw phv_%s use is present", name(),
                  (phv_byte_mask & 1) ? "lo" : "hi");
        }
    }
    for (auto &hd : hash_dist) hd.pass2(this);
}

void StatefulTable::pass3() { LOG1("### Stateful table " << name() << " pass3 " << loc()); }

int StatefulTable::direct_shiftcount() const {
    return 64 + METER_ADDRESS_ZERO_PAD - address_shift();
}

int StatefulTable::indirect_shiftcount() const { return METER_ADDRESS_ZERO_PAD - address_shift(); }

int StatefulTable::address_shift() const { return ceil_log2(format->size) - meter_adr_shift; }

unsigned StatefulTable::per_flow_enable_bit(MatchTable *match) const {
    if (!per_flow_enable)
        return METER_ADDRESS_BITS - METER_TYPE_BITS - 1;
    else
        return AttachedTable::per_flow_enable_bit(match);
}

unsigned StatefulTable::determine_shiftcount(Table::Call &call, int group, unsigned word,
                                             int tcam_shift) const {
    return determine_meter_shiftcount(call, group, word, tcam_shift);
}

/** Determine which stateful action a given table action invokes (if any)
 *  In theory, the stateful action to run could be an action data param or even come from
 *  hash_dist (so the action could run any stateful action), but currently the compiler will
 *  never geneate such code.  If we add that ability, we'll need to revisit this, and need
 *  to revise the context.json appropriately.  Currently, this code will return a nullptr
 *  for such bfa code (meter_type_arg would be a Field or HashDist)
 */
Table::Actions::Action *StatefulTable::action_for_table_action(const MatchTable *tbl,
                                                               const Actions::Action *act) const {
    // Check for action args to determine which stateful action is
    // called. If no args are present skip as the action does not
    // invoke stateful
    if (indirect) {
        for (auto att : act->attached) {
            if (att != this) continue;
            if (att.args.size() == 0) continue;
            auto meter_type_arg = att.args[0];
            if (meter_type_arg.type == Call::Arg::Name) {
                // Check if stateful has this called action
                return actions->action(meter_type_arg.name());
            } else if (meter_type_arg.type == Call::Arg::Const) {
                int index = -1;
                switch (meter_type_arg.value()) {
                    case STATEFUL_INSTRUCTION_0:
                        index = 0;
                        break;
                    case STATEFUL_INSTRUCTION_1:
                        index = 1;
                        break;
                    case STATEFUL_INSTRUCTION_2:
                        index = 2;
                        break;
                    case STATEFUL_INSTRUCTION_3:
                        index = 3;
                        break;
                }
                if (index == -1) continue;
                auto it = actions->begin();
                while (it != actions->end() && index > 0) {
                    --index;
                    ++it;
                }
                if (it != actions->end()) return &(*it);
            }
        }
    } else {
        // If stateful is direct, all user defined actions will
        // invoke stateful except for the miss action. This is
        // defined as 'default_only' in p4, if not the compiler
        // generates a default_only action and adds it
        // FIXME: Brig should add these generated actions as
        // default_only in assembly
        if (!((act->name == tbl->default_action) && tbl->default_only_action)) {
            // Direct has only one action
            if (actions) return &*actions->begin();
        }
    }
    return nullptr;
}

template <class REGS>
void StatefulTable::write_action_regs_vt(REGS &regs, const Actions::Action *act) {
    int meter_alu = layout[0].row / 4U;
    auto &stateful_regs = regs.rams.map_alu.meter_group[meter_alu].stateful;
    auto &salu_instr_common = stateful_regs.salu_instr_common[act->code];
    if (act->minmax_use) {
        salu_instr_common.salu_datasize = 7;
        salu_instr_common.salu_op_dual = is_dual_mode();
    } else if (is_dual_mode() || p4c_5192_workaround(act)) {
        salu_instr_common.salu_datasize = format->log2size - 1;
        salu_instr_common.salu_op_dual = 1;
    } else {
        salu_instr_common.salu_datasize = format->log2size;
    }
}

template <class REGS>
void StatefulTable::write_merge_regs_vt(REGS &regs, MatchTable *match, int type, int bus,
                                        const std::vector<Call::Arg> &args) {
    auto &merge = regs.rams.match.merge;
    unsigned adr_mask = 0U;
    unsigned per_entry_en_mux_ctl = 0U;
    unsigned adr_default = 0U;
    unsigned meter_type_position = 0U;
    METER_ACCESS_TYPE default_type = match->default_meter_access_type(true);
    AttachedTable::determine_meter_merge_regs(match, type, bus, args, default_type, adr_mask,
                                              per_entry_en_mux_ctl, adr_default,
                                              meter_type_position);
    merge.mau_meter_adr_default[type][bus] = adr_default;
    merge.mau_meter_adr_mask[type][bus] = adr_mask;
    merge.mau_meter_adr_per_entry_en_mux_ctl[type][bus] = per_entry_en_mux_ctl;
    merge.mau_meter_adr_type_position[type][bus] = meter_type_position;
}

template <class REGS>
void StatefulTable::write_regs_vt(REGS &regs) {
    LOG1("### Stateful table " << name() << " write_regs " << loc());
    // FIXME -- factor common AttachedTable::write_regs
    // FIXME -- factor common Synth2Port::write_regs
    // FIXME -- factor common CounterTable::write_regs
    // FIXME -- factor common MeterTable::write_regs
    for (auto &ixb : input_xbar) ixb->write_regs(regs);
    Layout *home = &layout[0];
    bool push_on_overflow = false;
    auto &map_alu = regs.rams.map_alu;
    auto &merge = regs.rams.match.merge;
    auto &adrdist = regs.rams.match.adrdist;
    DataSwitchboxSetup<REGS> swbox(regs, this);
    int minvpn, maxvpn;
    layout_vpn_bounds(minvpn, maxvpn, true);
    if (!bound_selector) {
        for (Layout &logical_row : layout) {
            unsigned row = logical_row.row / 2U;
            unsigned side = logical_row.row & 1; /* 0 == left  1 == right */
            BUG_CHECK(side == 1);                /* no map rams or alus on left side anymore */
            auto vpn = logical_row.vpns.begin();
            auto mapram = logical_row.maprams.begin();
            auto &map_alu_row = map_alu.row[row];
            LOG2("# DataSwitchbox.setup(" << row << ") home=" << home->row / 2U);
            swbox.setup_row(row);
            for (auto &memunit : logical_row.memunits) {
                BUG_CHECK(memunit.stage == INT_MIN && memunit.row == logical_row.row,
                          "bogus %s in logical row %d", memunit.desc(), logical_row.row);
                unsigned col = memunit.col + 6 * side;
                swbox.setup_row_col(row, col, *vpn);
                write_mapram_regs(regs, row, *mapram, *vpn, MapRam::STATEFUL);
                if (gress)
                    regs.cfg_regs.mau_cfg_uram_thread[col / 4U] |= 1U << (col % 4U * 8U + row);
                ++mapram, ++vpn;
            }
            /* FIXME -- factor with selector/meter? */
            if (&logical_row == home) {
                auto &vh_adr_xbar = regs.rams.array.row[row].vh_adr_xbar;
                auto &data_ctl = regs.rams.array.row[row].vh_xbar[side].stateful_meter_alu_data_ctl;
                for (auto &ixb : input_xbar) {
                    if (hash_bytemask != 0U) {
                        vh_adr_xbar.alu_hashdata_bytemask.alu_hashdata_bytemask_right =
                            hash_bytemask;
                        setup_muxctl(vh_adr_xbar.exactmatch_row_hashadr_xbar_ctl[2 + side],
                                     ixb->hash_group());
                    }
                    if (data_bytemask != 0) {
                        data_ctl.stateful_meter_alu_data_bytemask = data_bytemask;
                        data_ctl.stateful_meter_alu_data_xbar_ctl = 8 | ixb->match_group();
                    }
                }
                map_alu_row.i2portctl.synth2port_vpn_ctl.synth2port_vpn_base = minvpn;
                map_alu_row.i2portctl.synth2port_vpn_ctl.synth2port_vpn_limit = maxvpn;
                int meter_group_index = row / 2U;
                auto &delay_ctl = map_alu.meter_alu_group_data_delay_ctl[meter_group_index];
                delay_ctl.meter_alu_right_group_delay =
                    Target::METER_ALU_GROUP_DATA_DELAY() + row / 4 + stage->tcam_delay(gress);
                delay_ctl.meter_alu_right_group_enable =
                    meter_alu_fifo_enable_from_mask(regs, phv_byte_mask);
                auto &error_ctl = map_alu.meter_alu_group_error_ctl[meter_group_index];
                error_ctl.meter_alu_group_ecc_error_enable = 1;
                if (output_used) {
                    auto &action_ctl = map_alu.meter_alu_group_action_ctl[meter_group_index];
                    action_ctl.right_alu_action_enable = 1;
                    action_ctl.right_alu_action_delay = stage->meter_alu_delay(gress, divmod_used);
                    auto &switch_ctl = regs.rams.array.switchbox.row[row].ctl;
                    switch_ctl.r_action_o_mux_select.r_action_o_sel_action_rd_r_i = 1;
                    // disable action data address huffman decoding, on the assumtion we're not
                    // trying to combine this with an action data table on the same home row.
                    // Otherwise, the huffman decoding will think this is an 8-bit value and
                    // replicate it.
                    regs.rams.array.row[row]
                        .action_hv_xbar.action_hv_xbar_disable_ram_adr
                        .action_hv_xbar_disable_ram_adr_right = 1;
                }
            } else {
                auto &adr_ctl = map_alu_row.vh_xbars.adr_dist_oflo_adr_xbar_ctl[side];
                if (home->row >= 8 && logical_row.row < 8) {
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
    }
    if (actions) actions->write_regs(regs, this);
    unsigned meter_group = home->row / 4U;
    for (MatchTable *m : match_tables) {
        adrdist.mau_ad_meter_virt_lt[meter_group] |= 1U << m->logical_id;
        adrdist.adr_dist_meter_adr_icxbar_ctl[m->logical_id] |= 1 << meter_group;
    }
    if (!bound_selector) {
        bool first_match = true;
        for (MatchTable *m : match_tables) {
            adrdist.adr_dist_meter_adr_icxbar_ctl[m->logical_id] |= 1 << meter_group;
            adrdist.movereg_ad_meter_alu_to_logical_xbar_ctl[m->logical_id / 8U].set_subfield(
                4 | meter_group, 3 * (m->logical_id % 8U), 3);
            if (first_match)
                adrdist.movereg_meter_ctl[meter_group].movereg_meter_ctl_lt = m->logical_id;
            if (direct) {
                if (first_match)
                    adrdist.movereg_meter_ctl[meter_group].movereg_meter_ctl_direct = 1;
                adrdist.movereg_ad_direct[MoveReg::METER] |= 1U << m->logical_id;
            }
            first_match = false;
        }
        adrdist.movereg_meter_ctl[meter_group].movereg_ad_meter_shift = format->log2size;
        if (push_on_overflow) {
            adrdist.oflo_adr_user[0] = adrdist.oflo_adr_user[1] = AdrDist::METER;
            adrdist.deferred_oflo_ctl = 1 << (home->row - 8) / 2U;
        }
        adrdist.packet_action_at_headertime[1][meter_group] = 1;
    }
    write_logging_regs(regs);
    for (auto &hd : hash_dist) hd.write_regs(regs, this);
    if (gress == INGRESS || gress == GHOST) {
        merge.meter_alu_thread[0].meter_alu_thread_ingress |= 1U << meter_group;
        merge.meter_alu_thread[1].meter_alu_thread_ingress |= 1U << meter_group;
    } else if (gress == EGRESS) {
        merge.meter_alu_thread[0].meter_alu_thread_egress |= 1U << meter_group;
        merge.meter_alu_thread[1].meter_alu_thread_egress |= 1U << meter_group;
    }
    auto &salu = regs.rams.map_alu.meter_group[meter_group].stateful;
    salu.stateful_ctl.salu_enable = 1;
    salu.stateful_ctl.salu_output_pred_shift = pred_shift / 4;
    salu.stateful_ctl.salu_output_pred_comb_shift = pred_comb_shift;
    // The reset value for the CMP opcode is enabled by default -- we want to disable
    // any unused cmp units
    for (auto &inst : salu.salu_instr_cmp_alu) {
        for (auto &alu : inst) {
            if (!alu.salu_cmp_opcode.modified()) {
                alu.salu_cmp_opcode = 2;
            }
        }
    }
    if (gress == EGRESS) {
        regs.rams.map_alu.meter_group[meter_group].meter.meter_ctl.meter_alu_egress = 1;
    }
    if (math_table) {
        for (size_t i = 0; i < math_table.data.size(); ++i)
            salu.salu_mathtable[i / 4U].set_subfield(math_table.data[i], 8 * (i % 4U), 8);
        salu.salu_mathunit_ctl.salu_mathunit_output_scale = math_table.scale & 0x3fU;
        salu.salu_mathunit_ctl.salu_mathunit_exponent_invert = math_table.invert;
        switch (math_table.shift) {
            case -1:
                salu.salu_mathunit_ctl.salu_mathunit_exponent_shift = 2;
                break;
            case 0:
                salu.salu_mathunit_ctl.salu_mathunit_exponent_shift = 0;
                break;
            case 1:
                salu.salu_mathunit_ctl.salu_mathunit_exponent_shift = 1;
                break;
        }
    }
}

void StatefulTable::gen_tbl_cfg(json::vector &out) const {
    // FIXME -- factor common Synth2Port stuff
    int size = (layout_size() - 1) * 1024 * (128U / format->size);
    json::map &tbl = *base_tbl_cfg(out, "stateful", size);
    unsigned alu_width = format->size / (dual_mode ? 2 : 1);
    tbl["initial_value_lo"] = initial_value_lo;
    tbl["initial_value_hi"] = initial_value_hi;
    tbl["alu_width"] = alu_width;
    tbl["dual_width_mode"] = dual_mode;
    json::vector &act_to_sful_instr_slot = tbl["action_to_stateful_instruction_slot"];
    if (actions) {
        for (auto &a : *actions) {
            for (auto &i : a.instr) {
                if ((i->name() == "set_bit_at") || (i->name() == "set_bitc_at"))
                    tbl["set_instr_adjust_total"] = a.code;
                if ((i->name() == "set_bit") || (i->name() == "set_bitc"))
                    tbl["set_instr"] = a.code;
                if ((i->name() == "clr_bit_at") || (i->name() == "clr_bitc_at"))
                    tbl["clr_instr_adjust_total"] = a.code;
                if ((i->name() == "clr_bit") || (i->name() == "clr_bitc"))
                    tbl["clr_instr"] = a.code;
            }
        }
    }
    // Add action handle and instr slot for action which references stateful
    for (auto *m : match_tables) {
        if (auto *acts = m->get_actions()) {
            for (auto &a : *acts) {
                Actions::Action *stful_action = action_for_table_action(m, &a);
                if (!stful_action) continue;
                bool act_present = false;
                // Do not add handle if already present, if stateful spans
                // multiple stages this can happen as action handles are unique
                // and this code will get called again
                for (auto &s : act_to_sful_instr_slot) {
                    auto s_handle = s->to<json::map>()["action_handle"];
                    if (*s_handle->as_number() == a.handle) {
                        act_present = true;
                        break;
                    }
                }
                if (act_present) continue;
                json::map instr_slot;
                instr_slot["action_handle"] = a.handle;
                instr_slot["instruction_slot"] = stful_action->code;
                act_to_sful_instr_slot.push_back(std::move(instr_slot));
            }
        }
    }
    json::vector &register_file = tbl["register_params"];
    for (size_t i = 0; i < const_vals.size(); i++) {
        if (!const_vals[i].is_param) continue;
        json::map register_file_row;
        register_file_row["register_file_index"] = i;
        register_file_row["initial_value"] = const_vals[i].value;
        register_file_row["name"] = const_vals[i].param_name;
        register_file_row["handle"] = const_vals[i].param_handle;
        register_file.push_back(std::move(register_file_row));
    }
    if (bound_selector) tbl["bound_to_selection_table_handle"] = bound_selector->handle();
    json::map &stage_tbl = *add_stage_tbl_cfg(tbl, "stateful", size);
    add_alu_index(stage_tbl, "meter_alu_index");
    gen_tbl_cfg(tbl, stage_tbl);
    if (context_json) stage_tbl.merge(*context_json);
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(StatefulTable, TARGET_CLASS)
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void StatefulTable::write_action_regs,
                      (mau_regs & regs, const Actions::Action *act),
                      { write_action_regs_vt(regs, act); })
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void StatefulTable::write_merge_regs,
                      (mau_regs & regs, MatchTable *match, int type, int bus,
                       const std::vector<Call::Arg> &args),
                      { write_merge_regs_vt(regs, match, type, bus, args); })
