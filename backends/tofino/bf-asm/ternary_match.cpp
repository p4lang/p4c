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

#include "tofino/ternary_match.h"

#include "action_bus.h"
#include "lib/algorithm.h"
#include "input_xbar.h"
#include "instruction.h"
#include "misc.h"
#include "lib/range.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"

Table::Format::Field *TernaryMatchTable::lookup_field(const std::string &n,
                                                      const std::string &act) const {
    auto *rv = format ? format->field(n) : nullptr;
    if (!rv && gateway) rv = gateway->lookup_field(n, act);
    if (!rv && indirect) rv = indirect->lookup_field(n, act);
    if (!rv && !act.empty()) {
        if (auto call = get_action()) {
            rv = call->lookup_field(n, act);
        }
    }
    return rv;
}

Table::Format::Field *TernaryIndirectTable::lookup_field(const std::string &n,
                                                         const std::string &act) const {
    auto *rv = format ? format->field(n) : nullptr;
    if (!rv && !act.empty()) {
        if (auto call = get_action()) rv = call->lookup_field(n, act);
    }
    return rv;
}

void TernaryMatchTable::vpn_params(int &width, int &depth, int &period,
                                   const char *&period_name) const {
    if ((width = match.size()) == 0) {
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        width = input_xbar[0]->tcam_width();
    }
    depth = width ? layout_size() / width : 0;
    period = 1;
    period_name = 0;
}

void TernaryMatchTable::alloc_vpns() {
    if (no_vpns || layout.size() == 0 || layout[0].vpns.size() > 0) return;
    int period, width, depth;
    const char *period_name;
    vpn_params(width, depth, period, period_name);
    if (width == 0) return;
    std::vector<Layout *> rows;
    std::set<std::pair<int, int>> stage_cols;
    for (auto &r : layout) {
        for (auto &mem : r.memunits) stage_cols.emplace(mem.stage, mem.col);
        rows.push_back(&r);
        r.vpns.resize(r.memunits.size());
    }
    std::sort(rows.begin(), rows.end(),
              [](Layout *const &a, Layout *const &b) -> bool { return a->row < b->row; });
    int vpn = 0;
    for (auto [stage, col] : stage_cols) {
        for (auto *r : rows) {
            unsigned idx = find(r->memunits, MemUnit(stage, r->row, col)) - r->memunits.begin();
            if (idx < r->vpns.size()) r->vpns[idx] = vpn++ / width;
        }
        if (vpn % width != 0)
            error(layout[0].lineno,
                  "%d-wide ternary match must use a multiple of %d tcams "
                  "in each column",
                  width, width);
    }
}

TernaryMatchTable::Match::Match(const value_t &v) : lineno(v.lineno) {
    if (v.type == tVEC) {
        if (v.vec.size < 2 || v.vec.size > 3) {
            error(v.lineno, "Syntax error");
            return;
        }
        if (!CHECKTYPE(v[0], tINT) || !CHECKTYPE(v[v.vec.size - 1], tINT)) return;
        if ((word_group = v[0].i) < 0 || v[0].i >= Target::TCAM_XBAR_GROUPS())
            error(v[0].lineno, "Invalid input xbar group %" PRId64, v[0].i);
        if (Target::TCAM_EXTRA_NIBBLE() && v.vec.size == 3 && CHECKTYPE(v[1], tINT)) {
            if ((byte_group = v[1].i) < 0 || v[1].i >= Target::TCAM_XBAR_GROUPS() / 2)
                error(v[1].lineno, "Invalid input xbar group %" PRId64, v[1].i);
        } else {
            byte_group = -1;
        }
        if ((byte_config = v[v.vec.size - 1].i) < 0 || byte_config >= 4)
            error(v[v.vec.size - 1].lineno, "Invalid input xbar byte control %d", byte_config);
    } else if (CHECKTYPE(v, tMAP)) {
        for (auto &kv : MapIterChecked(v.map)) {
            if (kv.key == "group") {
                if (kv.value.type != tINT || kv.value.i < 0 ||
                    kv.value.i >= Target::TCAM_XBAR_GROUPS())
                    error(kv.value.lineno, "Invalid input xbar group %s", value_desc(kv.value));
                else
                    word_group = kv.value.i;
            } else if (Target::TCAM_EXTRA_NIBBLE() && kv.key == "byte_group") {
                if (kv.value.type != tINT || kv.value.i < 0 ||
                    kv.value.i >= Target::TCAM_XBAR_GROUPS() / 2)
                    error(kv.value.lineno, "Invalid input xbar group %s", value_desc(kv.value));
                else
                    byte_group = kv.value.i;
            } else if (Target::TCAM_EXTRA_NIBBLE() && kv.key == "byte_config") {
                if (kv.value.type != tINT || kv.value.i < 0 || kv.value.i >= 4)
                    error(kv.value.lineno, "Invalid byte group config %s", value_desc(kv.value));
                else
                    byte_config = kv.value.i;
            } else if (kv.key == "dirtcam") {
                if (kv.value.type != tINT || kv.value.i < 0 || kv.value.i > 0xfff)
                    error(kv.value.lineno, "Invalid dirtcam mode %s", value_desc(kv.value));
                else
                    dirtcam = kv.value.i;
            } else {
                error(kv.key.lineno, "Unknown key '%s' in ternary match spec", value_desc(kv.key));
            }
        }
    }
}

void TernaryMatchTable::setup(VECTOR(pair_t) & data) {
    tcam_id = -1;
    indirect_bus = -1;
    common_init_setup(data, true, P4Table::MatchEntry);
    if (input_xbar.empty()) input_xbar.emplace_back(InputXbar::create(this));
    if (auto *m = get(data, "match")) {
        if (CHECKTYPE2(*m, tVEC, tMAP)) {
            if (m->type == tVEC)
                for (auto &v : m->vec) match.emplace_back(v);
            else
                match.emplace_back(*m);
        }
    }
    for (auto &kv : MapIterChecked(data, {"meter", "stats", "stateful"})) {
        if (common_setup(kv, data, P4Table::MatchEntry)) {
        } else if (kv.key == "match") {
            /* done above to be done before vpns */
        } else if (kv.key == "indirect") {
            setup_indirect(kv.value);
        } else if (kv.key == "indirect_bus") {
            if (CHECKTYPE(kv.value, tINT)) {
                if (kv.value.i < 0 || kv.value.i >= 16) {
                    error(kv.value.lineno, "Invalid ternary indirect bus number");
                } else {
                    indirect_bus = kv.value.i;
                    if (auto *old =
                            stage->tcam_indirect_bus_use[indirect_bus / 2][indirect_bus & 1])
                        error(kv.value.lineno, "Indirect bus %d already in use by table %s",
                              indirect_bus, old->name());
                }
            }
        } else if (kv.key == "tcam_id") {
            if (CHECKTYPE(kv.value, tINT)) {
                if ((tcam_id = kv.value.i) < 0 || tcam_id >= TCAM_TABLES_PER_STAGE)
                    error(kv.key.lineno, "Invalid tcam_id %d", tcam_id);
                else if (stage->tcam_id_use[tcam_id])
                    error(kv.key.lineno, "Tcam id %d already in use by table %s", tcam_id,
                          stage->tcam_id_use[tcam_id]->name());
                else
                    stage->tcam_id_use[tcam_id] = this;
                physical_ids[tcam_id] = 1;
            }
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (Target::TCAM_GLOBAL_ACCESS())
        alloc_global_tcams();
    else
        alloc_rams(false, stage->tcam_use, &stage->tcam_match_bus_use);
    check_tcam_match_bus(layout);
    if (indirect_bus >= 0) {
        stage->tcam_indirect_bus_use[indirect_bus / 2][indirect_bus & 1] = this;
    }
    if (indirect.set()) {
        if (indirect_bus >= 0)
            error(lineno, "Table %s has both ternary indirect table and explicit indirect bus",
                  name());
        if (!attached.stats.empty() || !attached.meters.empty() || !attached.statefuls.empty())
            error(lineno,
                  "Table %s has ternary indirect table and directly attached stats/meters"
                  " -- move them to indirect table",
                  name());
    } else if (!action.set() && !actions) {
        error(lineno, "Table %s has no indirect, action table or immediate actions", name());
    }
    if (action && !action_bus) action_bus = ActionBus::create();
}

bitvec TernaryMatchTable::compute_reachable_tables() {
    MatchTable::compute_reachable_tables();
    if (indirect) reachable_tables_ |= indirect->reachable_tables();
    return reachable_tables_;
}

void TernaryMatchTable::pass0() {
    MatchTable::pass0();
    if (indirect.check() && indirect->set_match_table(this, false) != TERNARY_INDIRECT)
        error(indirect.lineno, "%s is not a ternary indirect table", indirect->name());
}

void TernaryMatchTable::pass1() {
    LOG1("### Ternary match table " << name() << " pass1 " << loc());
    if (action_bus) action_bus->pass1(this);
    MatchTable::pass1();
    stage->table_use[timing_thread(gress)] |= Stage::USE_TCAM;
    if (layout_size() == 0) layout.clear();
    BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
    if (match.empty() && input_xbar[0]->tcam_width() && layout.size() != 0) {
        match.resize(input_xbar[0]->tcam_width());
        for (unsigned i = 0; i < match.size(); i++) {
            match[i].word_group = input_xbar[0]->tcam_word_group(i);
            match[i].byte_group = input_xbar[0]->tcam_byte_group(i / 2);
            match[i].byte_config = i & 1;
        }
        match.back().byte_config = 3;
    }
    if (match.size() == 0) {
        if (layout.size() != 0)
            error(layout[0].lineno, "No match or input_xbar in non-empty ternary table %s", name());
    } else if (layout.size() % match.size() != 0) {
        error(layout[0].lineno, "Rows not a multiple of the match width in tables %s", name());
    } else if (layout.size() == 0) {
        error(lineno, "Empty ternary table with non-empty match");
    } else {
        auto mg = match.begin();
        for (auto &row : layout) {
            if (!row.bus.count(Layout::SEARCH_BUS))
                row.bus[Layout::SEARCH_BUS] = row.memunits.at(0).col;
            auto bus = row.bus.at(Layout::SEARCH_BUS);
            if (mg->byte_group >= 0) {
                auto &bg_use = stage->tcam_byte_group_use[row.row / 2][bus];
                if (bg_use.first) {
                    if (bg_use.second != mg->byte_group) {
                        error(mg->lineno,
                              "Conflicting tcam byte group between rows %d and %d "
                              "in col %d for table %s",
                              row.row, row.row ^ 1, bus, name());
                        if (bg_use.first != this)
                            error(bg_use.first->lineno, "...also used in table %s",
                                  bg_use.first->name());
                    }
                } else {
                    bg_use.first = this;
                    bg_use.second = mg->byte_group;
                }
            }
            if (++mg == match.end()) mg = match.begin();
        }
    }
    if (error_count > 0) return;
    for (auto &chain_rows_col : chain_rows) chain_rows_col = 0;
    unsigned row_use = 0;
    for (auto &row : layout) row_use |= 1U << row.row;
    unsigned word = 0, wide_row_use = 0;
    int prev_row = -1;
    std::vector<MemUnit> *memunits = nullptr;
    for (auto &row : layout) {
        if (row.memunits.empty()) {
            error(row.lineno, "Empty row in ternary table %s", name());
            continue;
        }
        if (memunits) {
            if (row.memunits.size() != memunits->size())
                error(row.lineno, "Column mismatch across rows in wide tcam match");
            for (size_t i = 0; i < row.memunits.size(); ++i)
                if (row.memunits[i].stage != memunits->at(i).stage ||
                    row.memunits[i].col != memunits->at(i).col)
                    error(row.lineno, "Column mismatch across rows in wide tcam match");
        } else {
            memunits = &row.memunits;
        }
        wide_row_use |= 1U << row.row;
        if (++word == match.size()) {
            int top_row = floor_log2(wide_row_use);
            int bottom_row = top_row + 1 - match.size();
            if (wide_row_use + (1U << bottom_row) != 1U << (top_row + 1)) {
                error(row.lineno,
                      "Ternary match rows must be contiguous "
                      "within each group of rows in a wide match");
            } else {
                // rows chain towards row 6
                if (top_row < 6)
                    wide_row_use -= 1U << top_row;
                else if (bottom_row > 6)
                    wide_row_use -= 1U << bottom_row;
                else
                    wide_row_use -= 1U << 6;
                for (auto &memunit : *memunits) {
                    int col = memunit.col;
                    if (col < 0 || col >= TCAM_UNITS_PER_ROW)
                        error(row.lineno, "Invalid column %d in table %s", col, name());
                    else
                        chain_rows[col] |= wide_row_use;
                }
            }
            word = 0;
            memunits = nullptr;
            wide_row_use = 0;
        }
    }
    if (indirect) {
        if (hit_next.size() > 0 && indirect->hit_next.size() > 0)
            error(lineno, "Ternary Match table with both direct and indirect next tables");
        if (!indirect->p4_table) indirect->p4_table = p4_table;
        if (hit_next.size() > 1 || indirect->hit_next.size() > 1) {
            if (auto *next = indirect->format->field("next")) {
                if (next->bit(0) != 0)
                    error(indirect->format->lineno,
                          "ternary indirect 'next' field must be"
                          " at bit 0");
            } else if (auto *action = indirect->format->field("action")) {
                if (action->bit(0) != 0)
                    error(indirect->format->lineno,
                          "ternary indirect 'action' field must be"
                          " at bit 0 to be used as next table selector");
            } else {
                error(indirect->format->lineno, "No 'next' or 'action' field in format");
            }
        }
        if (format)
            error(format->lineno,
                  "Format unexpected in Ternary Match table %s with separate "
                  "Indirect table %s",
                  name(), indirect->name());
    } else if (format) {
        format->pass1(this);
    }
    attached.pass1(this);
    if (hit_next.size() > 2 && !indirect)
        error(lineno, "Ternary Match tables cannot directly specify more than 2 hit next tables");
}

void TernaryMatchTable::pass2() {
    LOG1("### Ternary match table " << name() << " pass2 " << loc());
    if (logical_id < 0) choose_logical_id();
    for (auto &ixb : input_xbar) ixb->pass2();
    if (!indirect && indirect_bus < 0) {
        for (int i = 0; i < 16; i++)
            if (!stage->tcam_indirect_bus_use[i / 2][i & 1]) {
                indirect_bus = i;
                stage->tcam_indirect_bus_use[i / 2][i & 1] = this;
                break;
            }
        if (indirect_bus < 0)
            error(lineno, "No ternary indirect bus available for table %s", name());
    }
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
    if (gateway) gateway->pass2();
    if (idletime) idletime->pass2();
    if (is_alpm()) {
        if (auto *acts = get_actions()) {
            for (auto act = acts->begin(); act != acts->end(); act++) {
                set_partition_action_handle(act->handle);
                if (act->p4_params_list.size() > 0) {
                    // assume first parameter is partition_field_name
                    set_partition_field_name(act->p4_params_list[0].name);
                }
            }
        }
    }
    for (auto &hd : hash_dist) hd.pass2(this);
}

void TernaryMatchTable::pass3() {
    LOG1("### Ternary match table " << name() << " pass3 " << loc());
    MatchTable::pass3();
    if (action_bus) action_bus->pass3(this);
}

extern int get_address_mau_actiondata_adr_default(unsigned log2size, bool per_flow_enable);

template <class REGS>
inline static void tcam_ghost_enable(REGS &regs, int row, int col) {
    regs.tcams.col[col].tcam_ghost_thread_en[row] = 1;
}
template <>
void tcam_ghost_enable(Target::Tofino::mau_regs &regs, int row, int col) {}

template <class REGS>
void TernaryMatchTable::tcam_table_map(REGS &regs, int row, int col) {
    if (tcam_id >= 0) {
        if (!((chain_rows[col] >> row) & 1))
            regs.tcams.col[col].tcam_table_map[tcam_id] |= 1U << row;
    }
}

static void set_tcam_mode_logical_table(ubits<4> &reg, int tcam_id, int logical_id) {
    reg = logical_id;
}
static void set_tcam_mode_logical_table(ubits<8> &reg, int tcam_id, int logical_id) {
    reg |= 1U << tcam_id;
}

template <class REGS>
void TernaryMatchTable::write_regs_vt(REGS &regs) {
    LOG1("### Ternary match table " << name() << " write_regs " << loc());
    MatchTable::write_regs(regs, 1, indirect);
    unsigned word = 0;
    auto &merge = regs.rams.match.merge;
    for (Layout &row : layout) {
        auto vpn = row.vpns.begin();
        for (const auto &tcam : row.memunits) {
            BUG_CHECK(tcam.stage == INT_MIN && tcam.row == row.row, "bogus tcam %s in row %d",
                      tcam.desc(), row.row);
            auto &tcam_mode = regs.tcams.col[tcam.col].tcam_mode[row.row];
            // tcam_mode.tcam_data1_select = row.bus; -- no longer used
            if (options.match_compiler) tcam_mode.tcam_data1_select = tcam.col;
            tcam_mode.tcam_chain_out_enable = (chain_rows[tcam.col] >> row.row) & 1;
            if (gress == INGRESS)
                tcam_mode.tcam_ingress = 1;
            else if (gress == EGRESS)
                tcam_mode.tcam_egress = 1;
            else if (gress == GHOST)
                tcam_ghost_enable(regs, row.row, tcam.col);
            tcam_mode.tcam_match_output_enable =
                ((~chain_rows[tcam.col] | ALWAYS_ENABLE_ROW) >> row.row) & 1;
            tcam_mode.tcam_vpn = *vpn++;
            set_tcam_mode_logical_table(tcam_mode.tcam_logical_table, tcam_id, logical_id);
            tcam_mode.tcam_data_dirtcam_mode = match[word].dirtcam & 0x3ff;
            tcam_mode.tcam_vbit_dirtcam_mode = match[word].dirtcam >> 10;
            /* TODO -- always disable tcam_validbit_xbar? */
            auto &tcam_vh_xbar = regs.tcams.vh_data_xbar;
            if (options.match_compiler) {
                for (int i = 0; i < 8; i++)
                    tcam_vh_xbar.tcam_validbit_xbar_ctl[tcam.col][row.row / 2][i] |= 15;
            }
            auto &halfbyte_mux_ctl = tcam_vh_xbar.tcam_row_halfbyte_mux_ctl[tcam.col][row.row];
            halfbyte_mux_ctl.tcam_row_halfbyte_mux_ctl_select = match[word].byte_config;
            halfbyte_mux_ctl.tcam_row_halfbyte_mux_ctl_enable = 1;
            halfbyte_mux_ctl.tcam_row_search_thread = timing_thread(gress);
            if (match[word].word_group >= 0)
                setup_muxctl(tcam_vh_xbar.tcam_row_output_ctl[tcam.col][row.row],
                             match[word].word_group);
            if (match[word].byte_group >= 0)
                setup_muxctl(tcam_vh_xbar.tcam_extra_byte_ctl[tcam.col][row.row / 2],
                             match[word].byte_group);
            tcam_table_map(regs, row.row, tcam.col);
        }
        if (++word == match.size()) word = 0;
    }
    if (tcam_id >= 0)
        setup_muxctl(merge.tcam_hit_to_logical_table_ixbar_outputmap[tcam_id], logical_id);
    if (tcam_id >= 0) {
        if (stage->table_use[timing_thread(gress)] & Stage::USE_TCAM)
            merge.tcam_table_prop[tcam_id].tcam_piped = 1;
        merge.tcam_table_prop[tcam_id].thread = timing_thread(gress);
        merge.tcam_table_prop[tcam_id].enabled = 1;
        regs.tcams.tcam_output_table_thread[tcam_id] = 1 << timing_thread(gress);
    }
    if (indirect_bus >= 0) {
        /* FIXME -- factor into corresponding code in MatchTable::write_regs */
        setup_muxctl(merge.match_to_logical_table_ixbar_outputmap[1][indirect_bus], logical_id);
        setup_muxctl(merge.match_to_logical_table_ixbar_outputmap[3][indirect_bus], logical_id);
        if (tcam_id >= 0) {
            setup_muxctl(merge.tcam_match_adr_to_physical_oxbar_outputmap[indirect_bus], tcam_id);
        }
        if (action) {
            /* FIXME -- factor with TernaryIndirect code below */
            if (auto adt = action->to<ActionTable>()) {
                merge.mau_actiondata_adr_default[1][indirect_bus] = adt->determine_default(action);
                merge.mau_actiondata_adr_mask[1][indirect_bus] = adt->determine_mask(action);
                merge.mau_actiondata_adr_vpn_shiftcount[1][indirect_bus] =
                    adt->determine_vpn_shiftcount(action);
                merge.mau_actiondata_adr_tcam_shiftcount[indirect_bus] =
                    adt->determine_shiftcount(action, 0, 0, 0);
            }
        }
        attached.write_tcam_merge_regs(regs, this, indirect_bus, 0);
        merge.tind_bus_prop[indirect_bus].tcam_piped = 1;
        merge.tind_bus_prop[indirect_bus].thread = timing_thread(gress);
        merge.tind_bus_prop[indirect_bus].enabled = 1;
        if (idletime)
            merge.mau_idletime_adr_tcam_shiftcount[indirect_bus] = idletime->direct_shiftcount();
    }
    if (actions) actions->write_regs(regs, this);
    if (gateway) gateway->write_regs(regs);
    if (idletime) idletime->write_regs(regs);
    for (auto &hd : hash_dist) hd.write_regs(regs, this);
    merge.exact_match_logical_result_delay |= 1 << logical_id;
    regs.cfg_regs.mau_cfg_movereg_tcam_only |= 1U << logical_id;

    // FIXME -- this is wrong; when should we use the actionbit?  glass never does any more?
    // if (hit_next.size() > 1 && !indirect)
    //     merge.next_table_tcam_actionbit_map_en |= 1 << logical_id;
    // if (!indirect)
    //     merge.mau_action_instruction_adr_tcam_actionbit_map_en |= 1 << logical_id;
}

std::unique_ptr<json::map> TernaryMatchTable::gen_memory_resource_allocation_tbl_cfg(
    const char *type, const std::vector<Layout> &layout, bool skip_spare_bank) const {
    if (layout.size() == 0) return nullptr;
    BUG_CHECK(!skip_spare_bank);  // never spares in tcam
    json::map mra{{"memory_type", json::string(type)}};
    json::vector &mem_units_and_vpns = mra["memory_units_and_vpns"];
    json::vector mem_units;
    unsigned word = 0;
    bool done = false;
    unsigned lrow = 0;
    for (auto colnum = 0U; !done; colnum++) {
        done = true;
        for (auto &row : layout) {
            if (colnum >= row.memunits.size()) continue;
            auto mu = row.memunits[colnum];
            auto vpn = row.vpns[colnum];
            mem_units.push_back(json_memunit(mu));
            lrow = json_memunit(mu);
            if (++word == match.size()) {
                mem_units_and_vpns.push_back(json::map{{"memory_units", std::move(mem_units)},
                                                       {"vpns", json::vector{json::number(vpn)}}});
                mem_units = json::vector();
                word = 0;
            }
            done = false;
        }
    }
    // For keyless table, add empty vectors
    if (mem_units_and_vpns.size() == 0)
        mem_units_and_vpns.push_back(
            json::map{{"memory_units", json::vector()}, {"vpns", json::vector()}});
    mra["spare_bank_memory_unit"] = lrow;
    return json::mkuniq<json::map>(std::move(mra));
}

void TernaryMatchTable::gen_entry_cfg2(json::vector &out, std::string field_name,
                                       std::string global_name, unsigned lsb_offset,
                                       unsigned lsb_idx, unsigned msb_idx, std::string source,
                                       unsigned start_bit, unsigned field_width,
                                       bitvec &tcam_bits) const {
    json::map entry;
    entry["field_name"] = field_name;
    entry["global_name"] = global_name;
    entry["lsb_mem_word_offset"] = lsb_offset;
    entry["lsb_mem_word_idx"] = lsb_idx;
    entry["msb_mem_word_idx"] = msb_idx;
    entry["source"] = source;
    entry["start_bit"] = start_bit;
    entry["field_width"] = field_width;
    out.push_back(std::move(entry));
    // For a range with field width < nibble width, mark the entire
    // nibble in tcam_bits as used. The driver expects no overlap with other
    // format entries with the unused bits in the nibble.
    int tcam_bit_width = source == "range" ? 4 : field_width;
    tcam_bits.setrange(lsb_offset, tcam_bit_width);
}

void TernaryMatchTable::gen_entry_range_cfg(json::map &entry, bool duplicate,
                                            unsigned nibble_offset) const {
    json::map &entry_range = entry["range"];
    entry_range["type"] = 4;
    entry_range["is_duplicate"] = duplicate;
    entry_range["nibble_offset"] = nibble_offset;
}

void TernaryMatchTable::gen_entry_cfg(json::vector &out, std::string name, unsigned lsb_offset,
                                      unsigned lsb_idx, unsigned msb_idx, std::string source,
                                      unsigned start_bit, unsigned field_width, unsigned index,
                                      bitvec &tcam_bits, unsigned nibble_offset = 0) const {
    LOG3("Adding entry to Ternary Table : name: "
         << name << " lsb_offset: " << lsb_offset << " lsb_idx: " << lsb_idx
         << " msb_idx: " << msb_idx << " source: " << source << " start_bit: " << start_bit
         << " field_width: " << field_width << " index: " << index << " tcam_bits: " << tcam_bits
         << " nibble_offset: " << nibble_offset);
    std::string field_name(name);

    // If the name has a slice in it, remove it and add the lo bit of
    // the slice to field_bit.  This takes the place of
    // canon_field_list(), rather than extracting the slice component
    // of the field name, if present, and appending it to the key name.
    int slice_offset = remove_name_tail_range(field_name);
    LOG4("    Field Name: " << field_name << " slice_offset: " << slice_offset);

    // Get the key name, if any.
    int param_start_bit = slice_offset + start_bit;
    auto params = find_p4_params(field_name, "", param_start_bit, field_width);
    std::string global_name = "";
    if (params.size() == 0) {
        gen_entry_cfg2(out, field_name, global_name, lsb_offset, lsb_idx, msb_idx, source,
                       param_start_bit, field_width, tcam_bits);
    } else {
        for (auto param : params) {
            if (!param) continue;
            if (!param->key_name.empty()) {
                LOG4("    Found param : " << *param);
                field_name = param->key_name;
                global_name = param->name;
            }
            // For multiple params concatenated within the field width, we only
            // chose the param width which represents the slice.
            field_width = std::min(param->bit_width, field_width);

            // For range match we need bytes to decide which nibble is being used, hence
            // split the field in bytes. For normal match entire slice can be used
            // directly.
            auto *p = find_p4_param(name, "range", param_start_bit, field_width);
            if (p) {
                int lsb_lo = lsb_offset - TCAM_MATCH_BITS_START;
                int lsb_hi = lsb_lo + field_width - 1;
                /**
                 * For each byte of range match, the range match happens over either the lower
                 * nibble or higher nibble given the encoding scheme.  The nibble is transformed
                 * into an encoding over a byte.  This breaks up the range over each match nibble on
                 * a byte by byte boundary, and outputs the JSON for that nibble
                 *
                 * @seealso bf-p4c/mau/table_format.cpp comments on range
                 * @seealso bf-p4c/mau/resource_estimate.cpp comments on range
                 *
                 * The range context JSON encoding is the following:
                 *     - The lsb_mem_word_offset is always the beginning of the byte (as the
                 * encoding takes the whole byte)
                 *     - The width is the width of the field in the nibble (up to 4 bits)
                 *     - The nibble_offset is where in the nibble the key starts in the ixbar byte
                 *
                 * A "is_duplicate" nibble is provided.The driver uses this for not double counting,
                 * maybe.  Henry and I both agree that is really doesn't make any sense and can be
                 * deleted, but remains in there now
                 */
                for (int bit = (lsb_lo / 8) * 8; bit <= (lsb_hi / 8) * 8; bit += 8) {
                    int lsb_lo_bit_in_byte = std::max(lsb_lo, bit) % 8;
                    int lsb_hi_bit_in_byte = std::min(lsb_hi, bit + 7) % 8;
                    auto dirtcam_mode = get_dirtcam_mode(index, (bit / 8));

                    if (!(DIRTCAM_4B_LO == dirtcam_mode || DIRTCAM_4B_HI == dirtcam_mode)) continue;

                    bitvec nibbles_of_range;
                    nibbles_of_range.setbit(lsb_lo_bit_in_byte / 4);
                    nibbles_of_range.setbit(lsb_hi_bit_in_byte / 4);
                    int range_start_bit = start_bit + slice_offset;
                    int range_width;
                    int nibble_offset;

                    // Determine which section of the byte based on which nibble is provided
                    if (dirtcam_mode == DIRTCAM_4B_LO) {
                        BUG_CHECK(nibbles_of_range.getbit(0));
                        // Add the difference from the first bit of this byte and the lowest bit
                        range_start_bit += bit + lsb_lo_bit_in_byte - lsb_lo;
                        range_width =
                            std::min(static_cast<int>(field_width), 4 - lsb_lo_bit_in_byte);
                        range_width = std::min(static_cast<int>(range_width), lsb_hi - bit + 1);
                        nibble_offset = lsb_lo_bit_in_byte % 4;
                    } else {
                        BUG_CHECK(nibbles_of_range.getbit(1));
                        // Because the bit starts at the upper nibble, the start bit is either the
                        // beginning of the nibble or more
                        range_start_bit += bit + std::max(4, lsb_lo_bit_in_byte) - lsb_lo;
                        range_width =
                            std::min(static_cast<int>(field_width), lsb_hi_bit_in_byte - 3);
                        range_width = std::min(static_cast<int>(range_width),
                                               lsb_hi_bit_in_byte - lsb_lo_bit_in_byte + 1);
                        nibble_offset = std::max(4, lsb_lo_bit_in_byte) % 4;
                    }

                    // Add the range entry
                    gen_entry_cfg2(out, field_name, global_name, bit + TCAM_MATCH_BITS_START,
                                   lsb_idx, msb_idx, "range", range_start_bit, range_width,
                                   tcam_bits);
                    auto &last_entry = out.back()->to<json::map>();
                    gen_entry_range_cfg(last_entry, false, nibble_offset);

                    // Adding the duplicate range entry
                    gen_entry_cfg2(out, field_name, global_name, bit + TCAM_MATCH_BITS_START + 4,
                                   lsb_idx, msb_idx, "range", range_start_bit, range_width,
                                   tcam_bits);
                    auto &last_entry_dup = out.back()->to<json::map>();
                    gen_entry_range_cfg(last_entry_dup, true, nibble_offset);
                }

            } else {
                gen_entry_cfg2(out, field_name, global_name, lsb_offset, lsb_idx, msb_idx, source,
                               param_start_bit, field_width, tcam_bits);
            }
            param_start_bit += field_width;
        }
    }
}

void TernaryMatchTable::gen_match_fields_pvp(json::vector &match_field_list, unsigned word,
                                             bool uses_versioning, unsigned version_word_group,
                                             bitvec &tcam_bits) const {
    // Tcam bits are arranged as follows in each tcam word
    // LSB -------------------------------------MSB
    // PAYLOAD BIT - TCAM BITS - [VERSION] - PARITY
    auto start_bit = 0;      // always 0 for fields not on input xbar
    auto dirtcam_index = 0;  // not relevant for fields not on input xbar
    auto payload_name = "--tcam_payload_" + std::to_string(word) + "--";
    auto parity_name = "--tcam_parity_" + std::to_string(word) + "--";
    auto version_name = "--version--";
    gen_entry_cfg(match_field_list, payload_name, TCAM_PAYLOAD_BITS_START, word, word, "payload",
                  start_bit, TCAM_PAYLOAD_BITS, dirtcam_index, tcam_bits);
    if (uses_versioning && (version_word_group == word)) {
        gen_entry_cfg(match_field_list, version_name, TCAM_VERSION_BITS_START, word, word,
                      "version", start_bit, TCAM_VERSION_BITS, dirtcam_index, tcam_bits);
    }
    gen_entry_cfg(match_field_list, parity_name, TCAM_PARITY_BITS_START, word, word, "parity",
                  start_bit, TCAM_PARITY_BITS, dirtcam_index, tcam_bits);
}

void TernaryMatchTable::gen_match_fields(json::vector &match_field_list,
                                         std::vector<bitvec> &tcam_bits) const {
    unsigned match_index = match.size() - 1;
    for (auto &ixb : input_xbar) {
        for (const auto &[field_group, field_phv] : *ixb) {
            switch (field_group.type) {
                case InputXbar::Group::EXACT:
                    continue;
                case InputXbar::Group::TERNARY: {
                    int word = match_index - match_word(field_group.index);
                    if (word < 0) continue;
                    std::string source = "spec";
                    std::string field_name = field_phv.what.name();
                    unsigned lsb_mem_word_offset = 0;
                    if (field_phv.hi > 40) {
                        // FIXME -- no longer needed if we always convert these to Group::BYTE?
                        // a field in the (mid) byte group, which is shared with the adjacent word
                        // group each word gets only 4 bits of the byte group and is placed at msb
                        // Check mid-byte field does not cross byte boundary (40-47)
                        BUG_CHECK(field_phv.hi < 48);
                        // Check mid-byte field is associated with even group
                        // | == 5 == | == 1 == | == 5 == | == 5 == | == 1 == | == 5 == |
                        // | Grp 0   | Midbyte0| Grp 1   | Grp 2   | Midbyte1| Grp 3   |
                        BUG_CHECK((field_group.index & 1) == 0);
                        // Find groups to place this byte nibble. Check group which has this
                        // group as the byte_group
                        for (auto &m : match) {
                            if (m.byte_group * 2 == field_group.index) {
                                // Check byte_config to determine where to place the nibble
                                lsb_mem_word_offset = 1 + field_phv.lo;
                                int nibble_offset = 0;
                                int hwidth = 44 - field_phv.lo;
                                int start_bit = 0;
                                if (m.byte_config == MIDBYTE_NIBBLE_HI) {
                                    nibble_offset += 4;
                                    start_bit = hwidth;
                                    hwidth = field_phv.hi - 43;
                                }
                                int midbyte_word_group = match_index - match_word(m.word_group);
                                gen_entry_cfg(match_field_list, field_name, lsb_mem_word_offset,
                                              midbyte_word_group, midbyte_word_group, source,
                                              field_phv.what.lobit() + start_bit, hwidth,
                                              field_group.index, tcam_bits[midbyte_word_group]);
                            }
                        }
                    } else {
                        lsb_mem_word_offset = 1 + field_phv.lo;
                        gen_entry_cfg(match_field_list, field_name, lsb_mem_word_offset, word, word,
                                      source, field_phv.what.lobit(),
                                      field_phv.hi - field_phv.lo + 1, field_group.index,
                                      tcam_bits[word], field_phv.what->lo % 4);
                    }
                    break;
                }
                case InputXbar::Group::BYTE:
                    // The byte group represents what goes in top nibble in the tcam
                    // word. Based on the byte config, the corresponding match word is
                    // selected and the field (slice) is placed in the nibble.
                    //   byte group 5: { 0: HillTop.Lamona.Whitefish(0..1) ,
                    //                   2: HillTop.RossFork.Adona(0..5) }
                    //   match:
                    //        - { group: 10, byte_group: 5, byte_config: 0, dirtcam: 0x555 }
                    //        - { group: 11, byte_group: 5, byte_config: 1, dirtcam: 0x555 }
                    // Placement
                    //  --------------------------
                    //  Group 10 - Midbyte Nibble Lo
                    //  --------------------------
                    //  Word 1    : 41  42  43  44
                    //  Whitefish :  0   1   X   X
                    //  Adona     :  X   X   0   1
                    //  --------------------------
                    //  Group 11 - Midbyte Nibble Hi
                    //  --------------------------
                    //  Word 0    : 41  42  43  44
                    //  Whitefish :  X   X   X   X
                    //  Adona     :  2   3   4   5
                    //  --------------------------
                    for (size_t word = 0; word < match.size(); word++) {
                        if (match[word].byte_group != field_group.index) continue;
                        auto source = "spec";
                        auto field_name = field_phv.what.name();
                        int byte_lo = field_phv.lo;
                        int field_lo = field_phv.what.lobit();
                        int width = field_phv.what.size();
                        int nibble_lo = byte_lo;
                        if (match[word].byte_config == MIDBYTE_NIBBLE_HI) {
                            if (byte_lo >= 4) {
                                // NIBBLE HI  | NIBBLE LO
                                // 7  6  5  4 | 3  2  1  0
                                // x  x  x    |
                                // byte_lo = 5 (start of byte)
                                nibble_lo = byte_lo - 4;  // Get nibble_lo from nibble boundary
                                // nibble_lo = 1
                            } else {
                                // NIBBLE HI  | NIBBLE LO
                                // 7  6  5  4 | 3  2  1  0
                                //       x  x | x  x
                                // say field f1(3..7)
                                // field_lo = 3
                                // byte_lo = 2 (start of byte)
                                // width   = 4
                                width -= 4 - byte_lo;      // Adjust width to what must
                                                           // fit in the nibble
                                if (width <= 0) continue;  // No field in nibble, skip
                                // width = 2
                                nibble_lo = 0;            // Field starts at nibble boundary
                                field_lo += 4 - byte_lo;  // Adjust field lo bit to start of nibble
                                // field_lo = 5
                            }
                        } else if (match[word].byte_config == MIDBYTE_NIBBLE_LO) {
                            if (byte_lo >= 4) {
                                // NIBBLE HI  | NIBBLE LO
                                // 7  6  5  4 | 3  2  1  0
                                // x  x  x    |
                                // byte_lo = 5 (start of byte)
                                continue;  // No field in nibble, skip
                            } else {
                                // NIBBLE HI  | NIBBLE LO
                                // 7  6  5  4 | 3  2  1  0
                                //    x  x  x | x  x
                                // byte_lo = 2 (start of byte)
                                // width   = 5
                                nibble_lo = byte_lo;
                                int nibble_left = 4 - nibble_lo;
                                width = (width > nibble_left) ? nibble_left : width;
                                // width = 2
                            }
                        }
                        gen_entry_cfg(match_field_list, field_name, 41 + nibble_lo,
                                      match_index - word, match_index - word, source, field_lo,
                                      width, match[word].byte_group, tcam_bits[match_index - word]);
                    }
                    break;
            }
        }
    }
}

json::map &TernaryMatchTable::get_tbl_top(json::vector &out) const {
    unsigned number_entries = match.size() ? layout_size() / match.size() * 512 : 0;
    // For ALPM tables, this sets up the top level ALPM table and this ternary
    // table as its preclassifier. As the pre_classifier is always in the
    // previous stage as the atcams, this function will be called before the
    // atcam cfg generation. The atcam will check for presence of this table and
    // add the atcam cfg gen
    if (is_alpm()) {
        json::map *alpm_ptr = base_tbl_cfg(out, "match_entry", number_entries);
        json::map &alpm = *alpm_ptr;
        json::map &match_attributes = alpm["match_attributes"];
        match_attributes["match_type"] = "algorithmic_lpm";
        json::map &alpm_pre_classifier = match_attributes["pre_classifier"];
        base_alpm_pre_classifier_tbl_cfg(alpm_pre_classifier, "match_entry", number_entries);
        // top level alpm table has the same key as alpm preclassifier
        add_match_key_cfg(alpm);
        return alpm_pre_classifier;
    } else {
        return *base_tbl_cfg(out, "match_entry", number_entries);
    }
}

void TernaryMatchTable::gen_tbl_cfg(json::vector &out) const {
    unsigned number_entries = match.size() ? layout_size() / match.size() * 512 : 0;
    json::map &tbl = get_tbl_top(out);
    bool uses_versioning = false;
    unsigned version_word_group = -1;
    unsigned match_index = match.size() - 1;
    unsigned index = 0;
    json::vector match_field_list;
    for (auto &m : match) {
        if (m.byte_config == 3) {
            uses_versioning = true;
            version_word_group = match_index - index;
            break;
        }
        index++;
    }
    // Determine the zero padding necessary by creating a bitvector (for each
    // word). While creating entries for pack format set bits used. The unused
    // bits must be padded with zero field entries.
    std::vector<bitvec> tcam_bits(match.size());
    // Set pvp bits for each tcam word
    for (unsigned i = 0; i < match.size(); i++) {
        gen_match_fields_pvp(match_field_list, i, uses_versioning, version_word_group,
                             tcam_bits[i]);
    }
    json::map &match_attributes = tbl["match_attributes"];
    json::vector &stage_tables = match_attributes["stage_tables"];
    json::map &stage_tbl = *add_stage_tbl_cfg(match_attributes, "ternary_match", number_entries);
    // This is a only a glass required field, as it is only required when no default action
    // is specified, which is impossible for Brig through p4-16
    stage_tbl["default_next_table"] = Stage::end_of_pipe();
    json::map &pack_fmt =
        add_pack_format(stage_tbl, Target::TCAM_MEMORY_FULL_WIDTH(), match.size(), 1);
    stage_tbl["memory_resource_allocation"] =
        gen_memory_resource_allocation_tbl_cfg("tcam", layout);
    // FIXME-JSON: If the next table is modifiable then we set it to what it's mapped
    // to. Otherwise, set it to the default next table for this stage.
    // stage_tbl["default_next_table"] = Target::END_OF_PIPE();
    // FIXME: How to deal with multiple next hit tables?
    stage_tbl["default_next_table"] =
        hit_next.size() > 0 ? hit_next[0].next_table_id() : Target::END_OF_PIPE();
    add_result_physical_buses(stage_tbl);
    gen_match_fields(match_field_list, tcam_bits);

    // For keyless table, just add parity & payload bits
    if (p4_params_list.empty()) {
        tcam_bits.resize(1);
        gen_match_fields_pvp(match_field_list, 0, false, -1, tcam_bits[0]);
    }

    // tcam_bits is a vector indexed by tcam word and has all used bits set. We
    // loop through this bitvec for each word and add a zero padding entry for
    // the unused bits.
    // For ternary all unused bits must be marked as source
    // 'zero' for correctness during entry encoding.
    for (unsigned word = 0; word < match.size(); word++) {
        bitvec &pb = tcam_bits[word];
        unsigned start_bit = 0;  // always 0 for padded fields
        int dirtcam_index = -1;  // irrelevant in this context
        if (pb != bitvec(0)) {
            int idx_lo = 0;
            std::string pad_name = "--unused--";
            for (auto p : pb) {
                if (p > idx_lo) {
                    gen_entry_cfg(match_field_list, pad_name, idx_lo, word, word, "zero", start_bit,
                                  p - idx_lo, dirtcam_index, tcam_bits[word]);
                }
                idx_lo = p + 1;
            }
            auto fw = TCAM_VERSION_BITS;
            if (idx_lo < fw) {
                gen_entry_cfg(match_field_list, pad_name, idx_lo, word, word, "zero", start_bit,
                              fw - idx_lo, dirtcam_index, tcam_bits[word]);
            }
        }
    }

    pack_fmt["entries"] = json::vector{
        json::map{{"entry_number", json::number(0)}, {"fields", std::move(match_field_list)}}};
    add_all_reference_tables(tbl);
    json::map &tind = stage_tbl["ternary_indirection_stage_table"] = json::map();
    if (indirect) {
        unsigned fmt_width = 1U << indirect->format->log2size;
        // json::map tind;
        tind["stage_number"] = stage->stageno;
        tind["stage_table_type"] = "ternary_indirection";
        tind["size"] = indirect->layout_size() * 128 / fmt_width * 1024;
        indirect->add_pack_format(tind, indirect->format.get());
        tind["memory_resource_allocation"] =
            indirect->gen_memory_resource_allocation_tbl_cfg("sram", indirect->layout);
        // Add action formats for actions present in table or attached action table
        auto *acts = indirect->get_actions();
        if (acts) acts->add_action_format(this, tind);
        add_all_reference_tables(tbl, indirect);
        if (indirect->actions)
            indirect->actions->gen_tbl_cfg(tbl["actions"]);
        else if (indirect->action && indirect->action->actions)
            indirect->action->actions->gen_tbl_cfg(tbl["actions"]);
        indirect->common_tbl_cfg(tbl);
    } else {
        // FIXME: Add a fake ternary indirect table (as otherwise driver complains)
        // if tind not present - to be removed with update on driver side
        auto *acts = get_actions();
        if (acts) acts->add_action_format(this, tind);
        tind["memory_resource_allocation"] = nullptr;
        json::vector &pack_format = tind["pack_format"] = json::vector();
        json::map pack_format_entry;
        pack_format_entry["memory_word_width"] = 128;
        pack_format_entry["entries_per_table_word"] = 1;
        json::vector &entries = pack_format_entry["entries"] = json::vector();
        entries.push_back(json::map{{"entry_number", json::number(0)}, {"fields", json::vector()}});
        pack_format_entry["table_word_width"] = 0;
        pack_format_entry["number_memory_units_per_table_word"] = 0;
        pack_format.push_back(std::move(pack_format_entry));
        tind["logical_table_id"] = logical_id;
        tind["stage_number"] = stage->stageno;
        tind["stage_table_type"] = "ternary_indirection";
        tind["size"] = 0;
    }
    common_tbl_cfg(tbl);
    if (actions)
        actions->gen_tbl_cfg(tbl["actions"]);
    else if (action && action->actions)
        action->actions->gen_tbl_cfg(tbl["actions"]);
    gen_idletime_tbl_cfg(stage_tbl);
    merge_context_json(tbl, stage_tbl);
    match_attributes["match_type"] = "ternary";
}

void TernaryIndirectTable::setup(VECTOR(pair_t) & data) {
    match_table = 0;
    common_init_setup(data, true, P4Table::MatchEntry);
    if (format) {
        if (format->size > 64) error(format->lineno, "ternary indirect format larger than 64 bits");
        if (format->size < 4) {
            /* pad out to minumum size */
            format->size = 4;
            format->log2size = 2;
        }
    } else {
        error(lineno, "No format specified in table %s", name());
    }
    for (auto &kv : MapIterChecked(data, {"meter", "stats", "stateful"})) {
        if (common_setup(kv, data, P4Table::MatchEntry)) {
        } else if (kv.key == "input_xbar") {
            if (CHECKTYPE(kv.value, tMAP))
                input_xbar.emplace_back(InputXbar::create(this, false, kv.key, kv.value.map));
        } else if (kv.key == "hash_dist") {
            /* parsed in common_init_setup */
        } else if (kv.key == "selector") {
            attached.selector.setup(kv.value, this);
        } else if (kv.key == "selector_length") {
            attached.selector_length.setup(kv.value, this);
        } else if (kv.key == "meter_color") {
            attached.meter_color.setup(kv.value, this);
        } else if (kv.key == "stats") {
            if (kv.value.type == tVEC)
                for (auto &v : kv.value.vec) attached.stats.emplace_back(v, this);
            else
                attached.stats.emplace_back(kv.value, this);
        } else if (kv.key == "meter") {
            if (kv.value.type == tVEC)
                for (auto &v : kv.value.vec) attached.meters.emplace_back(v, this);
            else
                attached.meters.emplace_back(kv.value, this);
        } else if (kv.key == "stateful") {
            if (kv.value.type == tVEC)
                for (auto &v : kv.value.vec) attached.statefuls.emplace_back(v, this);
            else
                attached.statefuls.emplace_back(kv.value, this);
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (Target::SRAM_GLOBAL_ACCESS())
        alloc_global_srams();
    else
        alloc_rams(false, stage->sram_use, &stage->tcam_indirect_bus_use, Layout::TIND_BUS);
    if (!action.set() && !actions)
        error(lineno, "Table %s has neither action table nor immediate actions", name());
    if (actions && !action_bus) action_bus = ActionBus::create();
}

Table::table_type_t TernaryIndirectTable::set_match_table(MatchTable *m, bool indirect) {
    if (match_table) {
        error(lineno, "Multiple references to ternary indirect table %s", name());
    } else if (!(match_table = dynamic_cast<TernaryMatchTable *>(m))) {
        error(lineno, "Trying to link ternary indirect table %s to non-ternary table %s", name(),
              m->name());
    } else {
        if (action.check() && action->set_match_table(m, !action.is_direct_call()) != ACTION)
            error(action.lineno, "%s is not an action table", action->name());
        attached.pass0(m);
        logical_id = m->logical_id;
        p4_table = m->p4_table;
    }
    return TERNARY_INDIRECT;
}

bitvec TernaryIndirectTable::compute_reachable_tables() {
    Table::compute_reachable_tables();
    if (match_table) reachable_tables_ |= match_table->reachable_tables();
    reachable_tables_ |= attached.compute_reachable_tables();
    return reachable_tables_;
}

void TernaryIndirectTable::pass1() {
    LOG1("### Ternary indirect table " << name() << " pass1");
    determine_word_and_result_bus();
    Table::pass1();
    if (action_enable >= 0)
        if (action.args.size() < 1 || action.args[0].size() <= (unsigned)action_enable)
            error(lineno, "Action enable bit %d out of range for action selector", action_enable);
    if (format) format->pass1(this);
    for (auto &hd : hash_dist) {
        hd.pass1(this, HashDistribution::OTHER, false);
    }
}

/**
 * The bus by definition for ternary indirect is the result bus, and all TernaryIndirect tables
 * are at most 64 bits, meaning that all their words are equal to 0.
 */
void TernaryIndirectTable::determine_word_and_result_bus() {
    for (auto &row : layout) {
        row.word = 0;
    }
}

void TernaryIndirectTable::pass2() {
    LOG1("### Ternary indirect table " << name() << " pass2");
    if (logical_id < 0 && match_table) logical_id = match_table->logical_id;
    if (!match_table) error(lineno, "No match table for ternary indirect table %s", name());
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
    if (format) format->pass2(this);
}

void TernaryIndirectTable::pass3() {
    LOG1("### Ternary indirect table " << name() << " pass3");
    if (action_bus) action_bus->pass3(this);
}

template <class REGS>
void TernaryIndirectTable::write_regs_vt(REGS &regs) {
    LOG1("### Ternary indirect table " << name() << " write_regs");
    int tcam_id = match_table->tcam_id;
    int tcam_shift = format->log2size - 2;
    if (tcam_id >= 0) regs.tcams.tcam_match_adr_shift[tcam_id] = tcam_shift;
    auto &merge = regs.rams.match.merge;
    for (Layout &row : layout) {
        int bus = row.bus.at(Layout::TIND_BUS);
        auto vpn = row.vpns.begin();
        auto &ram_row = regs.rams.array.row[row.row];
        for (auto &memunit : row.memunits) {
            int col = memunit.col;
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == row.row, "bogus %s in row %d",
                      memunit.desc(), row.row);
            auto &unit_ram_ctl = ram_row.ram[col].unit_ram_ctl;
            unit_ram_ctl.match_ram_write_data_mux_select = 7; /* disable */
            unit_ram_ctl.match_ram_read_data_mux_select = 7;  /* disable */
            unit_ram_ctl.tind_result_bus_select = 1U << bus;
            auto &mux_ctl =
                regs.rams.map_alu.row[row.row].adrmux.ram_address_mux_ctl[col / 6][col % 6];
            mux_ctl.ram_unitram_adr_mux_select = bus + 2;
            auto &unitram_config =
                regs.rams.map_alu.row[row.row].adrmux.unitram_config[col / 6][col % 6];
            unitram_config.unitram_type = 6;
            unitram_config.unitram_vpn = *vpn++;
            unitram_config.unitram_logical_table = logical_id;
            if (gress == INGRESS || gress == GHOST)
                unitram_config.unitram_ingress = 1;
            else
                unitram_config.unitram_egress = 1;
            unitram_config.unitram_enable = 1;
            auto &xbar_ctl =
                regs.rams.map_alu.row[row.row].vh_xbars.adr_dist_tind_adr_xbar_ctl[bus];
            if (tcam_id >= 0) setup_muxctl(xbar_ctl, tcam_id);
            if (gress == EGRESS)
                regs.cfg_regs.mau_cfg_uram_thread[col / 4U] |= 1U << (col % 4U * 8U + row.row);
            ram_row.tind_ecc_error_uram_ctl[timing_thread(gress)] |= 1 << (col - 2);
        }
        int r_bus = row.row * 2 + bus;
        merge.tind_ram_data_size[r_bus] = format->log2size - 1;
        if (tcam_id >= 0)
            setup_muxctl(merge.tcam_match_adr_to_physical_oxbar_outputmap[r_bus], tcam_id);
        merge.tind_bus_prop[r_bus].tcam_piped = 1;
        merge.tind_bus_prop[r_bus].thread = timing_thread(gress);
        merge.tind_bus_prop[r_bus].enabled = 1;
        if (instruction) {
            int shiftcount = 0;
            if (auto field = instruction.args[0].field())
                shiftcount = field->bit(0);
            else if (auto field = instruction.args[1].field())
                shiftcount = field->immed_bit(0);
            merge.mau_action_instruction_adr_tcam_shiftcount[r_bus] = shiftcount;
        }
        if (format->immed) merge.mau_immediate_data_tcam_shiftcount[r_bus] = format->immed->bit(0);
        if (action) {
            if (auto adt = action->to<ActionTable>()) {
                merge.mau_actiondata_adr_default[1][r_bus] = adt->determine_default(action);
                merge.mau_actiondata_adr_mask[1][r_bus] = adt->determine_mask(action);
                merge.mau_actiondata_adr_vpn_shiftcount[1][r_bus] =
                    adt->determine_vpn_shiftcount(action);
                merge.mau_actiondata_adr_tcam_shiftcount[r_bus] =
                    adt->determine_shiftcount(action, 0, 0, tcam_shift);
            }
        }
        if (attached.selector) {
            auto sel = get_selector();
            merge.mau_meter_adr_tcam_shiftcount[r_bus] =
                sel->determine_shiftcount(attached.selector, 0, 0, format->log2size - 2);
            merge.mau_selectorlength_shiftcount[1][r_bus] =
                sel->determine_length_shiftcount(attached.selector_length, 0, 0);
            merge.mau_selectorlength_mask[1][r_bus] =
                sel->determine_length_mask(attached.selector_length);
            merge.mau_selectorlength_default[1][r_bus] =
                sel->determine_length_default(attached.selector_length);
        }
        if (match_table->idletime)
            merge.mau_idletime_adr_tcam_shiftcount[r_bus] =
                66 + format->log2size - match_table->idletime->precision_shift();
        attached.write_tcam_merge_regs(regs, match_table, r_bus, tcam_shift);
    }
    if (actions) actions->write_regs(regs, this);
    for (auto &hd : hash_dist) hd.write_regs(regs, this);
}

void TernaryIndirectTable::gen_tbl_cfg(json::vector &out) const {}

void TernaryMatchTable::add_result_physical_buses(json::map &stage_tbl) const {
    json::vector &result_physical_buses = stage_tbl["result_physical_buses"] = json::vector();
    if (indirect) {
        for (auto l : indirect->layout) {
            if (l.bus.count(Layout::TIND_BUS)) {
                result_physical_buses.push_back(l.row * 2 + l.bus.at(Layout::TIND_BUS));
            }
        }
    } else {
        result_physical_buses.push_back(indirect_bus);
    }
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(TernaryMatchTable, TARGET_CLASS)
DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(TernaryIndirectTable, TARGET_CLASS)
