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

#include "action_bus.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "input_xbar.h"
#include "instruction.h"
#include "lib/algorithm.h"

// template specialization declarations
#include "tofino/action_table.h"

/// See 6.2.8.4.3 of the MAU Micro-Architecture document.
const unsigned MAX_AD_SHIFT = 5U;

std::string ActionTable::find_field(Table::Format::Field *field) {
    for (auto &af : action_formats) {
        auto name = af.second->find_field(field);
        if (!name.empty() && name[0] != '<') return af.first + ":" + name;
    }
    return Table::find_field(field);
}

int ActionTable::find_field_lineno(Table::Format::Field *field) {
    int rv = -1;
    for (auto &af : action_formats)
        if ((rv = af.second->find_field_lineno(field)) >= 0) return rv;
    return Table::find_field_lineno(field);
}

Table::Format::Field *ActionTable::lookup_field(const std::string &name,
                                                const std::string &action) const {
    if (action == "*" || action == "") {
        if (auto *rv = format ? format->field(name) : 0) return rv;
        if (action == "*")
            for (auto &fmt : action_formats)
                if (auto *rv = fmt.second->field(name)) return rv;
    } else {
        if (action_formats.count(action)) {
            if (auto *rv = action_formats.at(action)->field(name)) return rv;
        } else if (auto *rv = format ? format->field(name) : 0) {
            return rv;
        }
    }
    for (auto *match_table : match_tables) {
        BUG_CHECK((Table *)match_table != (Table *)this);
        if (auto *rv = match_table->lookup_field(name)) return rv;
    }
    return 0;
}
void ActionTable::pad_format_fields() {
    format->size = get_size();
    format->log2size = get_log2size();
    for (auto &fmt : action_formats) {
        if (fmt.second->size < format->size) {
            fmt.second->size = format->size;
            fmt.second->log2size = format->log2size;
        }
    }
}

void ActionTable::apply_to_field(const std::string &n, std::function<void(Format::Field *)> fn) {
    for (auto &fmt : action_formats) fmt.second->apply_to_field(n, fn);
    if (format) format->apply_to_field(n, fn);
}
int ActionTable::find_on_actionbus(const ActionBusSource &src, int lo, int hi, int size, int pos) {
    int rv;
    if (action_bus && (rv = action_bus->find(src, lo, hi, size, pos)) >= 0) return rv;
    for (auto *match_table : match_tables) {
        BUG_CHECK((Table *)match_table != (Table *)this);
        if ((rv = match_table->find_on_actionbus(src, lo, hi, size, pos)) >= 0) return rv;
    }
    return -1;
}

int ActionTable::find_on_actionbus(const char *name, TableOutputModifier mod, int lo, int hi,
                                   int size, int *len) {
    int rv;
    if (action_bus && (rv = action_bus->find(name, mod, lo, hi, size, len)) >= 0) return rv;
    for (auto *match_table : match_tables) {
        BUG_CHECK((Table *)match_table != (Table *)this);
        if ((rv = match_table->find_on_actionbus(name, mod, lo, hi, size, len)) >= 0) return rv;
    }
    return -1;
}

void ActionTable::need_on_actionbus(const ActionBusSource &src, int lo, int hi, int size) {
    if (src.type == ActionBusSource::Field) {
        auto f = src.field;
        if (f->fmt == format.get()) {
            Table::need_on_actionbus(src, lo, hi, size);
            return;
        }
        for (auto &af : Values(action_formats)) {
            if (f->fmt == af.get()) {
                Table::need_on_actionbus(f, lo, hi, size);
                return;
            }
        }
        for (auto *match_table : match_tables) {
            BUG_CHECK((Table *)match_table != (Table *)this);
            if (f->fmt == match_table->get_format()) {
                match_table->need_on_actionbus(f, lo, hi, size);
                return;
            }
        }
        BUG_CHECK(!"Can't find table associated with field");
        // TBD - Add allocation for ActionBusSource::HashDistPair. Compiler does
        // action bus allocation so this path is never used.
    } else if (src.type == ActionBusSource::HashDist) {
        auto hd = src.hd;
        for (auto &hash_dist : this->hash_dist) {
            if (&hash_dist == hd) {
                Table::need_on_actionbus(hd, lo, hi, size);
                return;
            }
        }
        for (auto *match_table : match_tables) {
            if (match_table->find_hash_dist(hd->id) == hd) {
                match_table->need_on_actionbus(hd, lo, hi, size);
                return;
            }
        }
        BUG_CHECK(!"Can't find table associated with hash_dist");
    } else if (src.type == ActionBusSource::RandomGen) {
        auto rng = src.rng;
        int attached_count = 0;
        for (auto *match_table : match_tables) {
            match_table->need_on_actionbus(rng, lo, hi, size);
            ++attached_count;
        }
        if (attached_count > 1) {
            error(-1,
                  "Assembler cannot allocate action bus space for rng %d as it "
                  "used by mulitple tables",
                  rng.unit);
        }
    } else {
        error(-1, "Assembler cannot allocate action bus space for %s", src.toString(this).c_str());
    }
}

void ActionTable::need_on_actionbus(Table *att, TableOutputModifier mod, int lo, int hi, int size) {
    int attached_count = 0;
    for (auto *match_table : match_tables) {
        if (match_table->is_attached(att)) {
            match_table->need_on_actionbus(att, mod, lo, hi, size);
            ++attached_count;
        }
    }
    if (attached_count > 1) {
        error(att->lineno,
              "Assembler cannot allocate action bus space for table %s as it "
              "used by mulitple tables",
              att->name());
    }
}

/**
 * Necessary for determining the actiondata_adr_exact/tcam_shiftcount register value.
 */
unsigned ActionTable::determine_shiftcount(Table::Call &call, int group, unsigned word,
                                           int tcam_shift) const {
    int lo_huffman_bits =
        std::min(get_log2size() - 2, static_cast<unsigned>(ACTION_ADDRESS_ZERO_PAD));
    int extra_shift = ACTION_ADDRESS_ZERO_PAD - lo_huffman_bits;
    if (call.args[0] == "$DIRECT") {
        return 64 + extra_shift + tcam_shift;
    } else if (call.args[0].field()) {
        BUG_CHECK(call.args[0].field()->by_group[group]->bit(0) / 128U == word);
        return call.args[0].field()->by_group[group]->bit(0) % 128U + extra_shift;
    } else if (call.args[1].field()) {
        return call.args[1].field()->bit(0) + ACTION_ADDRESS_ZERO_PAD;
    }
    return 0;
}

/**
 * Calculates the actiondata_adr_default value.  Will default in the required huffman bits
 * described in section 6.2.8.4.3 Action RAM Addressing of the uArch, as well as the
 * per flow enable bit if indicated
 */
unsigned ActionTable::determine_default(Table::Call &call) const {
    int huffman_ones = std::max(static_cast<int>(get_log2size()) - 3, 0);
    BUG_CHECK(huffman_ones <= ACTION_DATA_HUFFMAN_BITS);
    unsigned huffman_mask = (1 << huffman_ones) - 1;
    // lower_huffman_mask == 0x1f, upper_huffman_mask = 0x60
    unsigned lower_huffman_mask = (1U << ACTION_DATA_LOWER_HUFFMAN_BITS) - 1;
    unsigned upper_huffman_mask = ((1U << ACTION_DATA_HUFFMAN_BITS) - 1) & ~lower_huffman_mask;
    unsigned rv = (huffman_mask & upper_huffman_mask) << ACTION_DATA_HUFFMAN_DIFFERENCE;
    rv |= huffman_mask & lower_huffman_mask;
    if (call.args[1].name() && call.args[1] == "$DEFAULT") {
        rv |= 1 << ACTION_DATA_PER_FLOW_ENABLE_START_BIT;
    }
    return rv;
}

/**
 * Calculates the actiondata_adr_mask value for a given table.
 */
unsigned ActionTable::determine_mask(Table::Call &call) const {
    int lo_huffman_bits =
        std::min(get_log2size() - 2, static_cast<unsigned>(ACTION_DATA_LOWER_HUFFMAN_BITS));
    unsigned rv = 0;
    if (call.args[0] == "$DIRECT") {
        rv |= ((1U << ACTION_ADDRESS_BITS) - 1) & (~0U << lo_huffman_bits);
    } else if (call.args[0].field()) {
        rv = ((1U << call.args[0].size()) - 1) << lo_huffman_bits;
    }
    return rv;
}

/**
 * Calculates the actiondata_adr_vpn_shiftcount register.  As described in section 6.2.8.4.3
 * for action data tables sized at 256, 512 and 1024, the Huffman bits for these addresses are
 * no longer at the bottom of the address, but rather near the top.  For direct action data
 * addresses, a hole in the address needs to be created.
 */
unsigned ActionTable::determine_vpn_shiftcount(Table::Call &call) const {
    if (call.args[0].name() && call.args[0] == "$DIRECT") {
        return std::max(0, static_cast<int>(get_log2size()) - 2 - ACTION_DATA_LOWER_HUFFMAN_BITS);
    }
    return 0;
}

int ActionTable::get_start_vpn() {
    // Based on the format width, the starting vpn is determined as follows (See
    // Section 6.2.8.4.3 in MAU MicroArchitecture Doc)
    //    WIDTH     LOG2SIZE START_VPN
    // <= 128 bits  -  7       - 0
    //  = 256 bits  -  8       - 0
    //  = 512 bits  -  9       - 1
    //  = 1024 bits - 10       - 3
    int size = get_log2size();
    if (size <= 8) return 0;
    if (size == 9) return 1;
    if (size == 10) return 3;
    return 0;
}

void ActionTable::vpn_params(int &width, int &depth, int &period, const char *&period_name) const {
    width = 1;
    depth = layout_size();
    period = format ? 1 << std::max(static_cast<int>(format->log2size) - 7, 0) : 0;
    // Based on the format width, the vpn are numbered as follows (See Section
    // 6.2.8.4.3 in MAU MicroArchitecture Doc)
    //    WIDTH     PERIOD  VPN'S
    // <= 128 bits  - +1 - 0, 1, 2, 3, ...
    //  = 256 bits  - +2 - 2, 4, 6, 8, ...
    //  = 512 bits  - +4 - 1, 5, 9, 13, ...
    //  = 1024 bits - +8 - 3, 11, 19, 27, ...
    for (auto &fmt : Values(action_formats))
        period = std::max(period, 1 << std::max(static_cast<int>(fmt->log2size) - 7, 0));
    period_name = "action data width";
}

void ActionTable::setup(VECTOR(pair_t) & data) {
    action_id = -1;
    setup_layout(layout, data);
    for (auto &kv : MapIterChecked(data, true)) {
        if (kv.key == "format") {
            const char *action = nullptr;
            if (kv.key.type == tCMD) {
                if (!PCHECKTYPE(kv.key.vec.size > 1, kv.key[1], tSTR)) continue;
                if (action_formats.count((action = kv.key[1].s))) {
                    error(kv.key.lineno, "Multiple formats for action %s", kv.key[1].s);
                    continue;
                }
            }
            if (CHECKTYPEPM(kv.value, tMAP, kv.value.map.size > 0, "non-empty map")) {
                auto *fmt = new Format(this, kv.value.map, true);
                if (fmt->size < 8) {  // pad out to minimum size
                    fmt->size = 8;
                    fmt->log2size = 3;
                }
                if (action)
                    action_formats[action].reset(fmt);
                else
                    format.reset(fmt);
            }
        }
    }
    if (!format && action_formats.empty()) error(lineno, "No format in action table %s", name());
    for (auto &kv : MapIterChecked(data, true)) {
        if (kv.key == "format") {
            /* done above to be done before action_bus and vpns */
        } else if (kv.key.type == tCMD && kv.key[0] == "format") {
            /* done above to be done before action_bus */
        } else if (kv.key == "actions") {
            if (CHECKTYPE(kv.value, tMAP)) actions.reset(new Actions(this, kv.value.map));
        } else if (kv.key == "action_bus") {
            if (CHECKTYPE(kv.value, tMAP)) action_bus = ActionBus::create(this, kv.value.map);
        } else if (kv.key == "action_id") {
            if (CHECKTYPE(kv.value, tINT)) action_id = kv.value.i;
        } else if (kv.key == "vpns") {
            if (kv.value == "null")
                no_vpns = true;
            else if (CHECKTYPE(kv.value, tVEC))
                setup_vpns(layout, &kv.value.vec);
        } else if (kv.key == "home_row") {
            home_lineno = kv.value.lineno;
            // Builds the map of home rows possible per word, as different words of the
            // action row is on different home rows
            if (CHECKTYPE2(kv.value, tINT, tVEC)) {
                int word = 0;
                if (kv.value.type == tINT) {
                    if (kv.value.i >= 0 || kv.value.i < LOGICAL_SRAM_ROWS)
                        home_rows_per_word[word].setbit(kv.value.i);
                    else
                        error(kv.value.lineno, "Invalid home row %" PRId64 "", kv.value.i);
                } else {
                    for (auto &v : kv.value.vec) {
                        if (CHECKTYPE2(v, tINT, tVEC)) {
                            if (v.type == tINT) {
                                if (v.i >= 0 || v.i < LOGICAL_SRAM_ROWS)
                                    home_rows_per_word[word].setbit(v.i);
                                else
                                    error(v.lineno, "Invalid home row %" PRId64 "", v.i);
                            } else if (v.type == tVEC) {
                                for (auto &v2 : v.vec) {
                                    if (CHECKTYPE(v2, tINT)) {
                                        if (v2.i >= 0 || v2.i < LOGICAL_SRAM_ROWS)
                                            home_rows_per_word[word].setbit(v2.i);
                                        else
                                            error(v.lineno, "Invalid home row %" PRId64 "", v2.i);
                                    }
                                }
                            }
                        }
                        word++;
                    }
                }
            }
        } else if (kv.key == "p4") {
            if (CHECKTYPE(kv.value, tMAP))
                p4_table = P4Table::get(P4Table::ActionData, kv.value.map);
        } else if (kv.key == "context_json") {
            setup_context_json(kv.value);
        } else if (kv.key == "row" || kv.key == "logical_row" || kv.key == "column" ||
                   kv.key == "word") {
            /* already done in setup_layout */
        } else if (kv.key == "logical_bus") {
            if (CHECKTYPE2(kv.value, tSTR, tVEC)) {
                if (kv.value.type == tSTR) {
                    if (*kv.value.s != 'A' && *kv.value.s != 'O' && *kv.value.s != 'S')
                        error(kv.value.lineno, "Invalid logical bus %s", kv.value.s);
                } else {
                    for (auto &v : kv.value.vec) {
                        if (CHECKTYPE(v, tSTR)) {
                            if (*v.s != 'A' && *v.s != 'O' && *v.s != 'S')
                                error(v.lineno, "Invalid logical bus %s", v.s);
                        }
                    }
                }
            }
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (Target::SRAM_GLOBAL_ACCESS())
        alloc_global_srams();
    else
        alloc_rams(true, stage->sram_use, 0);
    if (!action_bus) action_bus = ActionBus::create();
}

void ActionTable::pass1() {
    LOG1("### Action table " << name() << " pass1 " << loc());
    if (default_action.empty()) default_action = get_default_action();
    if (!p4_table)
        p4_table = P4Table::alloc(P4Table::ActionData, this);
    else
        p4_table->check(this);
    alloc_vpns();
    std::sort(layout.begin(), layout.end(), [](const Layout &a, const Layout &b) -> bool {
        if (a.word != b.word) return a.word < b.word;
        return a.row > b.row;
    });
    int width = format ? (format->size - 1) / 128 + 1 : 1;
    for (auto &fmt : action_formats) {
#if 0
        for (auto &fld : *fmt.second) {
            if (auto *f = format ? format->field(fld.first) : 0) {
                if (fld.second.bits != f->bits || fld.second.size != f->size) {
                    error(fmt.second->lineno, "Action %s format for field %s incompatible "
                          "with default format", fmt.first.c_str(), fld.first.c_str());
                    continue; } }
            for (auto &fmt2 : action_formats) {
                if (fmt.second == fmt2.second) break;
                if (auto *f = fmt2.second->field(fld.first)) {
                    if (fld.second.bits != f->bits || fld.second.size != f->size) {
                        error(fmt.second->lineno, "Action %s format for field %s incompatible "
                              "with action %s format", fmt.first.c_str(), fld.first.c_str(),
                              fmt2.first.c_str());
                        break; } } } }
#endif
        width = std::max(width, int((fmt.second->size - 1) / 128U + 1));
    }
    unsigned depth = layout_size() / width;
    std::vector<int> slice_size(width, 0);
    unsigned idx = 0;  // ram index within depth
    int word = 0;      // word within wide table;
    int home_row = -1;
    std::map<int, bitvec> final_home_rows;
    Layout *prev = nullptr;
    for (auto row = layout.begin(); row != layout.end(); ++row) {
        if (row->word > 0) word = row->word;
        if (!prev || prev->word != word || home_rows_per_word[word].getbit(row->row) ||
            home_row / 2 - row->row / 2 > 5 /* can't go over 5 physical rows for timing */
            || (!Target::SUPPORT_OVERFLOW_BUS() && home_row >= 8 && row->row < 8)
            /* can't flow between logical row 7 and 8 in JBay*/
        ) {
            if (prev && prev->row == row->row) prev->home_row = false;
            home_row = row->row;
            row->home_row = true;
            final_home_rows[word].setbit(row->row);
            need_bus(row->lineno, stage->action_data_use, row->row, "action data");
        }
        if (row->word >= 0) {
            if (row->word > width) {
                error(row->lineno, "Invalid word %u for row %d", row->word, row->row);
                continue;
            }
            slice_size[row->word] += row->memunits.size();
        } else {
            if (slice_size[word] + row->memunits.size() > depth) {
                int split = depth - slice_size[word];
                row = layout.insert(row, Layout(*row));
                row->memunits.erase(row->memunits.begin() + split, row->memunits.end());
                row->vpns.erase(row->vpns.begin() + split, row->vpns.end());
                auto next = row + 1;
                next->memunits.erase(next->memunits.begin(), next->memunits.begin() + split);
                next->vpns.erase(next->vpns.begin(), next->vpns.begin() + split);
            }
            row->word = word;
            if ((slice_size[word] += row->memunits.size()) == int(depth)) ++word;
        }
        prev = &*row;
    }
    if (!home_rows_per_word.empty()) {
        for (word = 0; word < width; ++word) {
            for (unsigned row : home_rows_per_word[word] - final_home_rows[word]) {
                error(home_lineno, "home row %u not present in table %s", row, name());
                break;
            }
        }
    }
    home_rows_per_word = final_home_rows;
    for (word = 0; word < width; ++word)
        if (slice_size[word] != int(depth)) {
            error(layout.front().lineno, "Incorrect size for word %u in layout of table %s", word,
                  name());
            break;
        }
    for (auto &r : layout) LOG4("  " << r);
    action_bus->pass1(this);
    if (actions) actions->pass1(this);
    AttachedTable::pass1();
    SelectionTable *selector = nullptr;
    for (auto mtab : match_tables) {
        auto *s = mtab->get_selector();
        if (s && selector && s != selector)
            error(lineno, "Inconsistent selectors %s and %s for table %s", s->name(),
                  selector->name(), name());
        if (s) selector = s;
    }
}

void ActionTable::pass2() {
    LOG1("### Action table " << name() << " pass2 " << loc());
    if (match_tables.empty()) error(lineno, "No match table for action table %s", name());
    if (!format) format.reset(new Format(this));
    /* Driver does not support formats with different widths. Need all formats
     * to be the same size, so pad them out */
    pad_format_fields();
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
}

/**
 * FIXME: Due to get_match_tables function not being a const function (which itself should be
 * a separate PR), in order to get all potentialy pack formats from all of the actions in all
 * associated match tables, an initial pass is required to perform this lookup.
 *
 * Thus a map is saved in this pass containing a copy of an action, with a listing of all of
 * the possible aliases.  This will only currently work if the aliases are identical across
 * actions, which at the moment, they are.  We will need to change this functionality when
 * actions could potentially be different across action profiles, either by gathering a union
 * of the aliases across actions with the same action handle, or perhaps de-alias the pack
 * formats before context JSON generation
 */
void ActionTable::pass3() {
    LOG1("### Action table " << name() << " pass3 " << loc());
    action_bus->pass3(this);

    if (!actions) {
        Actions *tbl_actions = nullptr;
        for (auto mt : get_match_tables()) {
            if (mt->actions) {
                tbl_actions = mt->actions.get();
            } else if (auto tern = mt->to<TernaryMatchTable>()) {
                if (tern->indirect && tern->indirect->actions) {
                    tbl_actions = tern->indirect->actions.get();
                }
            }
            BUG_CHECK(tbl_actions);
            for (auto &act : *tbl_actions) {
                if (pack_actions.count(act.name) == 0) pack_actions[act.name] = &act;
            }
        }
    } else {
        for (auto &act : *actions) {
            if (pack_actions.count(act.name) == 0) pack_actions[act.name] = &act;
        }
    }

    for (auto &fmt : action_formats) {
        if (pack_actions.count(fmt.first) == 0) {
            error(fmt.second->lineno, "Format for non-existant action %s", fmt.first.c_str());
            continue;
        }
    }
}

template <class REGS>
static void flow_selector_addr(REGS &regs, int from, int to) {
    BUG_CHECK(from > to);
    BUG_CHECK((from & 3) == 3);
    if (from / 2 == to / 2) {
        /* R to L */
        regs.rams.map_alu.selector_adr_switchbox.row[from / 4]
            .ctl.l_oflo_adr_o_mux_select.l_oflo_adr_o_sel_selector_adr_r_i = 1;
        return;
    }
    if (from & 1) /* R down */
        regs.rams.map_alu.selector_adr_switchbox.row[from / 4]
            .ctl.b_oflo_adr_o_mux_select.b_oflo_adr_o_sel_selector_adr_r_i = 1;
    // else
    //     /* L down */
    //     regs.rams.map_alu.selector_adr_switchbox.row[from/4].ctl
    //         .b_oflo_adr_o_mux_select.b_oflo_adr_o_sel_selector_adr_l_i = 1;

    /* Include all selection address switchboxes needed when the action RAMs
     * reside on overflow rows */
    for (int row = from / 4 - 1; row >= to / 4; row--)
        if (row != to / 4 || (to % 4) < 2) /* top to bottom */
            regs.rams.map_alu.selector_adr_switchbox.row[row]
                .ctl.b_oflo_adr_o_mux_select.b_oflo_adr_o_sel_oflo_adr_t_i = 1;

    switch (to & 3) {
        case 3:
            /* flow down to R */
            regs.rams.map_alu.selector_adr_switchbox.row[to / 4].ctl.r_oflo_adr_o_mux_select = 1;
            break;
        case 2:
            /* flow down to L */
            regs.rams.map_alu.selector_adr_switchbox.row[to / 4]
                .ctl.l_oflo_adr_o_mux_select.l_oflo_adr_o_sel_oflo_adr_t_i = 1;
            break;
        default:
            /* even physical rows are hardwired to flow down to both L and R */
            break;
    }
}

template <class REGS>
void ActionTable::write_regs_vt(REGS &regs) {
    LOG1("### Action table " << name() << " write_regs " << loc());
    unsigned fmt_log2size = format ? format->log2size : 0;
    unsigned width = format ? (format->size - 1) / 128 + 1 : 1;
    for (auto &fmt : Values(action_formats)) {
        fmt_log2size = std::max(fmt_log2size, fmt->log2size);
        width = std::max(width, (fmt->size - 1) / 128U + 1);
    }
    unsigned depth = layout_size() / width;
    bool push_on_overflow = false;  // true if we overflow from bottom to top
    unsigned idx = 0;
    int word = 0;
    Layout *home = nullptr;
    int prev_logical_row = -1;
    decltype(regs.rams.array.switchbox.row[0].ctl) *home_switch_ctl = 0, *prev_switch_ctl = 0;
    auto &adrdist = regs.rams.match.adrdist;
    auto &icxbar = adrdist.adr_dist_action_data_adr_icxbar_ctl;
    for (Layout &logical_row : layout) {
        unsigned row = logical_row.row / 2;
        unsigned side = logical_row.row & 1; /* 0 == left  1 == right */
        unsigned top = logical_row.row >= 8; /* 0 == bottom  1 == top */
        auto vpn = logical_row.vpns.begin();
        auto &switch_ctl = regs.rams.array.switchbox.row[row].ctl;
        auto &map_alu_row = regs.rams.map_alu.row[row];
        if (logical_row.home_row) {
            home = &logical_row;
            home_switch_ctl = &switch_ctl;
            action_bus->write_action_regs(regs, this, logical_row.row, word);
            if (side)
                switch_ctl.r_action_o_mux_select.r_action_o_sel_action_rd_r_i = 1;
            else
                switch_ctl.r_l_action_o_mux_select.r_l_action_o_sel_action_rd_l_i = 1;
            for (auto mtab : match_tables)
                icxbar[mtab->logical_id].address_distr_to_logical_rows |= 1U << logical_row.row;
        } else {
            BUG_CHECK(home);
            // FIXME use DataSwitchboxSetup for this somehow?
            if (&switch_ctl == home_switch_ctl) {
                /* overflow from L to R action */
                switch_ctl.r_action_o_mux_select.r_action_o_sel_oflo_rd_l_i = 1;
            } else {
                if (side) {
                    /* overflow R up */
                    switch_ctl.t_oflo_rd_o_mux_select.t_oflo_rd_o_sel_oflo_rd_r_i = 1;
                } else {
                    /* overflow L up */
                    switch_ctl.t_oflo_rd_o_mux_select.t_oflo_rd_o_sel_oflo_rd_l_i = 1;
                }
                if (prev_switch_ctl != &switch_ctl) {
                    if (prev_switch_ctl != home_switch_ctl)
                        prev_switch_ctl->t_oflo_rd_o_mux_select.t_oflo_rd_o_sel_oflo_rd_b_i = 1;
                    else if (home->row & 1)
                        home_switch_ctl->r_action_o_mux_select.r_action_o_sel_oflo_rd_b_i = 1;
                    else
                        home_switch_ctl->r_l_action_o_mux_select.r_l_action_o_sel_oflo_rd_b_i = 1;
                }
            }
            /* if we're skipping over full rows and overflowing over those rows, need to
             * propagate overflow from bottom to top.  This effectively uses only the
             * odd (right side) overflow busses.  L ovfl can still go to R action */
            for (int r = prev_logical_row / 2 - 1; r > static_cast<int>(row); r--) {
                prev_switch_ctl = &regs.rams.array.switchbox.row[r].ctl;
                prev_switch_ctl->t_oflo_rd_o_mux_select.t_oflo_rd_o_sel_oflo_rd_b_i = 1;
            }

            auto &oflo_adr_xbar = map_alu_row.vh_xbars.adr_dist_oflo_adr_xbar_ctl[side];
            if ((home->row >= 8) == top) {
                oflo_adr_xbar.adr_dist_oflo_adr_xbar_source_index = home->row % 8;
                oflo_adr_xbar.adr_dist_oflo_adr_xbar_source_sel = 0;
            } else {
                BUG_CHECK(home->row >= 8);
                BUG_CHECK(options.target == TOFINO);
                oflo_adr_xbar.adr_dist_oflo_adr_xbar_source_index = 0;
                oflo_adr_xbar.adr_dist_oflo_adr_xbar_source_sel = 3;
                push_on_overflow = true;
                for (auto mtab : match_tables)
                    if (!icxbar[mtab->logical_id].address_distr_to_overflow)
                        icxbar[mtab->logical_id].address_distr_to_overflow = 1;
            }
            oflo_adr_xbar.adr_dist_oflo_adr_xbar_enable = 1;
        }
        SelectionTable *selector = get_selector();
        if (selector) {
            if (logical_row.row != selector->home_row()) {
                if (logical_row.row > selector->home_row())
                    error(lineno, "Selector data from %s on row %d cannot flow up to %s on row %d",
                          selector->name(), selector->home_row(), name(), logical_row.row);
                else
                    flow_selector_addr(regs, selector->home_row(), logical_row.row);
            }
        }
        for (auto &memunit : logical_row.memunits) {
            int logical_col = memunit.col;
            unsigned col = logical_col + 6 * side;
            auto &ram = regs.rams.array.row[row].ram[col];
            auto &unitram_config = map_alu_row.adrmux.unitram_config[side][logical_col];
            if (logical_row.home_row) unitram_config.unitram_action_subword_out_en = 1;
            ram.unit_ram_ctl.match_ram_write_data_mux_select = UnitRam::DataMux::NONE;
            ram.unit_ram_ctl.match_ram_read_data_mux_select =
                home == &logical_row ? UnitRam::DataMux::ACTION : UnitRam::DataMux::OVERFLOW;
            unitram_config.unitram_type = UnitRam::ACTION;
            if (!no_vpns) unitram_config.unitram_vpn = *vpn++;
            unitram_config.unitram_logical_table = action_id >= 0 ? action_id : logical_id;
            if (gress == INGRESS || gress == GHOST)
                unitram_config.unitram_ingress = 1;
            else
                unitram_config.unitram_egress = 1;
            unitram_config.unitram_enable = 1;
            auto &ram_mux = map_alu_row.adrmux.ram_address_mux_ctl[side][logical_col];
            auto &adr_mux_sel = ram_mux.ram_unitram_adr_mux_select;
            if (selector) {
                int shift = std::min(fmt_log2size - 2, MAX_AD_SHIFT);
                auto &shift_ctl = regs.rams.map_alu.mau_selector_action_adr_shift[row];
                if (logical_row.row == selector->layout[0].row) {
                    /* we're on the home row of the selector, so use it directly */
                    if (home == &logical_row)
                        adr_mux_sel = UnitRam::AdrMux::SELECTOR_ALU;
                    else
                        adr_mux_sel = UnitRam::AdrMux::SELECTOR_ACTION_OVERFLOW;
                    if (side)
                        shift_ctl.mau_selector_action_adr_shift_right = shift;
                    else
                        shift_ctl.mau_selector_action_adr_shift_left = shift;
                } else {
                    /* not on the home row -- use overflows */
                    if (home == &logical_row)
                        adr_mux_sel = UnitRam::AdrMux::SELECTOR_OVERFLOW;
                    else
                        adr_mux_sel = UnitRam::AdrMux::SELECTOR_ACTION_OVERFLOW;
                    if (side)
                        shift_ctl.mau_selector_action_adr_shift_right_oflo = shift;
                    else
                        shift_ctl.mau_selector_action_adr_shift_left_oflo = shift;
                }
            } else {
                if (home == &logical_row) {
                    adr_mux_sel = UnitRam::AdrMux::ACTION;
                } else {
                    adr_mux_sel = UnitRam::AdrMux::OVERFLOW;
                    ram_mux.ram_oflo_adr_mux_select_oflo = 1;
                }
            }
            if (gress == EGRESS)
                regs.cfg_regs.mau_cfg_uram_thread[col / 4U] |= 1U << (col % 4U * 8U + row);
            regs.rams.array.row[row].actiondata_error_uram_ctl[timing_thread(gress)] |=
                1 << (col - 2);
            if (++idx == depth) {
                idx = 0;
                home = nullptr;
                ++word;
            }
        }
        prev_switch_ctl = &switch_ctl;
        prev_logical_row = logical_row.row;
    }
    if (push_on_overflow) adrdist.oflo_adr_user[0] = adrdist.oflo_adr_user[1] = AdrDist::ACTION;
    if (actions) actions->write_regs(regs, this);
}

// Action data address huffman encoding
//    { 0,      {"xxx", "xxxxx"} },
//    { 8,      {"xxx", "xxxx0"} },
//    { 16,     {"xxx", "xxx01"} },
//    { 32,     {"xxx", "xx011"} },
//    { 64,     {"xxx", "x0111"} },
//    { 128,    {"xxx", "01111"} },
//    { 256,    {"xx0", "11111"} },
//    { 512,    {"x01", "11111"} },
//    { 1024,   {"011", "11111"} };

// Track the actions added to json per action table. gen_tbl_cfg can be called
// multiple times for the same action for each stage table in case of an action
// table split across multiple stages, but must be added to json only once.
static std::map<std::string, std::set<std::string>> actions_in_json;
void ActionTable::gen_tbl_cfg(json::vector &out) const {
    // FIXME -- this is wrong if actions have different format sizes
    unsigned number_entries = (layout_size() * 128 * 1024) / (1 << format->log2size);
    json::map &tbl = *base_tbl_cfg(out, "action_data", number_entries);
    json::map &stage_tbl = *add_stage_tbl_cfg(tbl, "action_data", number_entries);
    for (auto &act : pack_actions) {
        auto *fmt = format.get();
        if (action_formats.count(act.first)) fmt = action_formats.at(act.first).get();
        add_pack_format(stage_tbl, fmt, true, true, act.second);
        auto p4Name = p4_name();
        if (!p4Name) {
            error(lineno, "No p4 table name found for table : %s", name());
            continue;
        }
        std::string tbl_name = p4Name;
        std::string act_name = act.second->name;
        if (actions_in_json.count(tbl_name) == 0) {
            actions_in_json[tbl_name].insert(act_name);
            act.second->gen_simple_tbl_cfg(tbl["actions"]);
        } else {
            auto acts_added = actions_in_json[tbl_name];
            if (acts_added.count(act_name) == 0) {
                actions_in_json[tbl_name].emplace(act_name);
                act.second->gen_simple_tbl_cfg(tbl["actions"]);
            }
        }
    }
    stage_tbl["memory_resource_allocation"] =
        gen_memory_resource_allocation_tbl_cfg("sram", layout);
    // FIXME: what is the check for static entries?
    tbl["static_entries"] = json::vector();
    std::string hr = how_referenced();
    if (hr.empty()) hr = indirect ? "indirect" : "direct";
    tbl["how_referenced"] = hr;
    merge_context_json(tbl, stage_tbl);
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(ActionTable, TARGET_CLASS)  // NOLINT(readability/fn_size)
