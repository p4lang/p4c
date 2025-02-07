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

#include "tofino/exact_match.h"

#include "action_bus.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "hashexpr.h"
#include "input_xbar.h"
#include "instruction.h"
#include "lib/algorithm.h"
#include "lib/hex.h"
#include "misc.h"

void ExactMatchTable::setup(VECTOR(pair_t) & data) {
    common_init_setup(data, false, P4Table::MatchEntry);
    for (auto &kv : MapIterChecked(data, {"meter", "stats", "stateful"})) {
        if (common_setup(kv, data, P4Table::MatchEntry)) {
            // Dynamic key masks are only on exact match tables
        } else if (kv.key == "dynamic_key_masks") {
            if (CHECKTYPE(kv.value, tSTR))
                dynamic_key_masks = (strncmp(kv.value.s, "true", 4) == 0);
        } else if (kv.key == "stash") {
            CHECKTYPE(kv.value, tMAP);
            for (auto &m : kv.value.map) {
                if (m.key == "row") {
                    if (CHECKTYPE(m.value, tVEC)) {
                        auto rows = m.value.vec;
                        for (value_t &r : rows) {
                            if (CHECKTYPE(r, tINT)) stash_rows.push_back(r.i);
                        }
                    }
                }
                if (m.key == "col") {
                    if (CHECKTYPE(m.value, tVEC)) {
                        auto cols = m.value.vec;
                        for (value_t &c : cols) {
                            if (CHECKTYPE(c, tINT)) stash_cols.push_back(c.i);
                        }
                    }
                }
                if (m.key == "unit") {
                    if (CHECKTYPE(m.value, tVEC)) {
                        auto units = m.value.vec;
                        for (value_t &u : units) {
                            if (CHECKTYPE(u, tINT)) stash_units.push_back(u.i);
                        }
                    }
                }
            }
            if (stash_rows.size() == 0) {
                error(kv.value.lineno, "No 'row' attribute for stash info in exact match table %s",
                      name());
                return;
            }
            if (stash_cols.size() == 0) {
                error(kv.value.lineno, "No 'col' attribute for stash info in exact match table %s",
                      name());
                return;
            }
            if (stash_units.size() == 0) {
                error(kv.value.lineno, "No 'unit' attribute for stash info in exact match table %s",
                      name());
                return;
            }
            if (stash_units.size() != stash_rows.size()) {
                error(kv.value.lineno,
                      "Stash units not specified correctly for each row entry "
                      "in exact match table %s",
                      name());
                return;
            }
        } else if (kv.key == "search_bus" || kv.key == "result_bus") {
            // already dealt with in Table::setup_layout via common_init_setup
        } else {
            common_sram_setup(kv, data);
        }
    }
    common_sram_checks();
}

void ExactMatchTable::pass1() {
    LOG1("### Exact match table " << name() << " pass1 " << loc());
    SRamMatchTable::pass1();
    // Check if stashes are allocated (only for exact match tables). Note
    // stashes are disabled on JBAY
    if (stash_rows.size() == 0 && options.target == TOFINO && layout_size() > 0)
        error(lineno, "No stashes allocated for exact match table %s in stage %d", name(),
              stage->stageno);
}

/**
 * Any bits that are not matched directly against, but appear in the key of the p4 table,
 * are ghost bits.  The rules for ghost bits on exact match tables are:
 *
 *    1. Any field that does not appear in the match key must appear in the hash function.  This
 *       is considered a ghost bit
 *    2. A hash column can have at most one ghost bit, in order to maintain the linear
 *       independence of the impact of each ghost bit.
 *
 * The following function verifies these two properties, and saves them in a map to be output
 * in the gen_ghost_bits function call
 */
void ExactMatchTable::determine_ghost_bits() {
    std::set<std::pair<std::string, int>> ghost_bits;
    // Determine ghost bits by determine what is not in the match
    for (auto &p4_param : p4_params_list) {
        for (int bit = p4_param.start_bit; bit < p4_param.start_bit + p4_param.bit_width; bit++) {
            if (!p4_param.mask.empty() && !p4_param.mask[bit]) continue;  // Skip non-masked bits.
            bool found = false;
            for (auto ms : match) {
                std::string field_name = ms->name();
                int field_bit_lo = remove_name_tail_range(field_name) + ms->fieldlobit();
                int field_bit_hi = field_bit_lo + ms->size() - 1;
                if (field_name == p4_param.name && field_bit_lo <= bit && field_bit_hi >= bit) {
                    found = true;
                    break;
                }
            }
            if (found) continue;
            ghost_bits.emplace(p4_param.name, bit);
        }
    }

    BUG_CHECK(!input_xbar.empty(), "%s does not have an input xbar", name());
    for (const auto &ixb : input_xbar) {
        int way_index = 0;
        for (auto way : ways) {
            bitvec hash_tables;
            if (auto *hash_group = ixb->get_hash_group(way.group_xme)) {
                hash_tables = bitvec(hash_group->tables);
            } else {
                for (auto &ht : ixb->get_hash_tables()) {
                    BUG_CHECK(ht.first.type == InputXbar::HashTable::EXACT);
                    hash_tables[ht.first.index] = 1;
                }
            }

            // key is the field name/field bit that is the ghost bit
            // value is the bits that the ghost bit appears in within this way
            std::map<std::pair<std::string, int>, bitvec> ghost_bit_impact;

            // Check a phv ref against the ghost bits for sanity
            auto check_ref = [this, way_index, &ghost_bits, &ghost_bit_impact, &ixb](Phv::Ref &ref,
                                                                                     int hash_bit) {
                std::string field_name = ref.name();
                int field_bit = remove_name_tail_range(field_name) + ref.fieldlobit();
                for (int i = 0; i < ref.size(); ++i) {
                    auto key = std::make_pair(field_name, field_bit + i);
                    auto ghost_bit_it = ghost_bits.find(key);
                    if (ghost_bit_it == ghost_bits.end()) continue;

                    // This is a check to make sure that the ghost bit appears only once
                    // in the hash column, as an even number of appearances would
                    // xor each other out, and cancel the hash out.  This check
                    // should be done on all hash bits
                    if (ghost_bit_impact[key].getbit(hash_bit)) {
                        error(ixb->lineno,
                              "Ghost bit %s:%d appears multiple times "
                              "in the same hash col %d",
                              key.first.c_str(), key.second, way_index);
                        return;
                    }
                    ghost_bit_impact[key].setbit(hash_bit);
                }
            };

            // Calculate the ghost bit per hash way
            for (unsigned hash_table_id : hash_tables) {
                auto &hash_table = ixb->get_hash_table(hash_table_id);
                for (auto hash_bit : way.select_bits()) {
                    if (hash_table.count(hash_bit) == 0) continue;
                    const HashCol &hash_col = hash_table.at(hash_bit);
                    if (hash_col.fn) {
                        for (auto &ref : hash_col.fn->get_sources(hash_col.bit))
                            check_ref(ref, hash_bit);
                    } else {
                        for (const auto &input_bit : hash_col.data)
                            if (auto ref = ixb->get_hashtable_bit(hash_table_id, input_bit))
                                check_ref(ref, hash_bit);
                    }
                }
            }

            // Verify that each ghost bit appears in the hash function
            for (auto gb : ghost_bits) {
                if (ghost_bit_impact.find(gb) == ghost_bit_impact.end()) {
                    error(ixb->lineno,
                          "Ghost bit %s:%d does not appear on the hash function "
                          "for way %d",
                          gb.first.c_str(), gb.second, way_index);
                    return;
                }
            }

            // Verify that the ghost bits are linearly independent, that only one ghost bit
            // exists per column
            bitvec total_use;
            for (auto gbi : ghost_bit_impact) {
                if (!(total_use & gbi.second).empty())
                    error(ixb->lineno, "The ghost bits are not linear independent on way %d",
                          way_index);
                total_use |= gbi.second;
            }

            auto &ghost_bit_position = ghost_bit_positions[way.group_xme];
            for (auto gbi : ghost_bit_impact) {
                ghost_bit_position[gbi.first] |= gbi.second;
            }
            way_index++;
        }
    }
}

void ExactMatchTable::pass2() {
    LOG1("### Exact match table " << name() << " pass2 " << loc());
    // FIXME -- does some of this common stuff belong in SRamMatch::pass2
    if (logical_id < 0) choose_logical_id();
    for (auto &ixb : input_xbar) ixb->pass2();
    setup_word_ixbar_group();
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
    if (gateway) gateway->pass2();
    if (idletime) idletime->pass2();
    if (format) format->pass2(this);
    unsigned usable = -1;
    for (auto &ixb : input_xbar) usable &= ixb->exact_physical_ids();
    allocate_physical_ids(usable);
    determine_ghost_bits();
    // Derive a stash format from current table format with a single entry (we
    // use group 0 entry) and all fields except 'version' and 'action' (match
    // overhead). The version bits are set by the driver.
    if (format) {
        stash_format.reset(new Format(this));
        stash_format->size = MEM_WORD_WIDTH;
        stash_format->log2size = ceil_log2(MEM_WORD_WIDTH);
        auto group = 0;
        for (auto f = format->begin(group); f != format->end(group); f++) {
            if (f->first == "action" || f->first == "version") continue;
            stash_format->add_field(f->second, f->first, group);
        }
    }
    for (auto &hd : hash_dist) hd.pass2(this);
    if (format) verify_format_pass2();
}

void ExactMatchTable::pass3() {
    LOG1("### Exact match table " << name() << " pass3 " << loc());
    SRamMatchTable::pass3();
    if (action_bus) action_bus->pass3(this);
}

// Check way_map for each stash row/col pair to determine which word the ram is
// assigned to and verify if it is the match overhead word. Allocate stash
// overhead row for each stash row/col pair.
void ExactMatchTable::generate_stash_overhead_rows() {
    auto mem_units_per_word = format ? format->get_mem_units_per_table_word() : 1;
    for (int i = 0; i < stash_rows.size(); i++) {
        auto idx = (i + mem_units_per_word) / mem_units_per_word;
        if (stash_overhead_rows.size() >= idx) continue;
        auto stash_row = stash_rows[i];
        auto stash_col = stash_cols[i];
        for (auto &row : layout) {
            if (row.row == stash_row) {
                Ram stash_ram(stash_row, stash_col);
                if (way_map.count(stash_ram) > 0) {
                    auto way_word = way_map[stash_ram].word;
                    BUG_CHECK(format);
                    if (way_word == format->overhead_word) {
                        stash_overhead_rows.push_back(stash_row);
                        break;
                    }
                }
            }
        }
    }
}

/* FIXME -- should have ExactMatchTable::write_merge_regs write some of the merge stuff
 * from write_regs? */
template <class REGS>
void ExactMatchTable::write_regs_vt(REGS &regs) {
    LOG1("### Exact match table " << name() << " write_regs " << loc());
    SRamMatchTable::write_regs(regs);

    for (auto &row : layout) {
        auto &rams_row = regs.rams.array.row[row.row];
        for (auto &ram : row.memunits) {
            auto &way = way_map[ram];
            BUG_CHECK(ram.stage == INT_MIN && ram.row == row.row, "bogus %s in row %d", ram.desc(),
                      row.row);
            auto &ram_cfg = rams_row.ram[ram.col];
            ram_cfg.match_nibble_s0q1_enable = version_nibble_mask.getrange(way.word * 32U, 32);
            ram_cfg.match_nibble_s1q0_enable = UINT64_C(0xffffffff);
        }
    }

    // Write stash regs if stashes are allocated
    if (stash_rows.size() == 0) return;
    auto &merge = regs.rams.match.merge;
    auto &stash_hitmap_output_map = merge.stash_hitmap_output_map;
    generate_stash_overhead_rows();
    auto mem_units_per_word = format ? format->get_mem_units_per_table_word() : 1;
    for (int i = 0; i < stash_rows.size(); i++) {
        auto stash_row = stash_rows[i];
        auto stash_col = stash_cols[i];
        auto stash_unit_id = stash_units[i];
        MemUnit stash_memunit(stash_row, stash_col);
        auto idx = i / mem_units_per_word;
        auto physical_row_with_overhead =
            stash_overhead_rows.size() > idx ? stash_overhead_rows[idx] : ways[0].rams[0].row;
        LOG5("Setting cfg for stash Row: " << stash_row << ", stash Unit: " << stash_unit_id
                                           << " with overhead word row: "
                                           << physical_row_with_overhead);
        auto &stash_map_entry = stash_hitmap_output_map[stash_unit_id][stash_row];
        stash_map_entry.enabled_3bit_muxctl_select = physical_row_with_overhead;
        stash_map_entry.enabled_3bit_muxctl_enable = 1;
        auto &stash_reg = regs.rams.array.row[stash_row].stash;
        auto &input_data_ctl = stash_reg.stash_match_input_data_ctl[stash_unit_id];
        input_data_ctl.stash_hash_adr_select = ways[0].index / EXACT_HASH_ADR_BITS;
        input_data_ctl.stash_enable = 1;
        input_data_ctl.stash_logical_table = logical_id;
        input_data_ctl.stash_thread = (gress == EGRESS);
        auto &stash_row_nxtable_bus_drive =
            merge.stash_row_nxtable_bus_drive[stash_unit_id][stash_row];
        for (auto &row : layout) {
            if (row.row != stash_row) continue;
            if (contains(row.memunits, stash_memunit)) {
                // Assumption is that the search or match and result buses are
                // always generated on the same index
                auto &stash_match_mask = stash_reg.stash_match_mask[stash_unit_id];
                if (stash_row == physical_row_with_overhead) {
                    // FIXME -- the overhead row should always have a result bus allocated, but
                    // sometimes it does not.  This hack has been here for awhile and is needed
                    // for p4_16/compile_only/meters_0.p4 at least, but seems wrong and unsafe
                    int result_bus = row.bus.count(Layout::RESULT_BUS)
                                         ? row.bus.at(Layout::RESULT_BUS)
                                         : row.bus.at(Layout::SEARCH_BUS);
                    stash_row_nxtable_bus_drive = 1 << result_bus;
                    stash_reg.stash_match_result_bus_select[stash_unit_id] = 1 << result_bus;

                    // Set default next table only when there is a single next table
                    auto &nxt_table_lut = merge.stash_next_table_lut[stash_unit_id][stash_row];
                    std::set<Ref> nxt_tables;
                    for (auto &n : hit_next) {
                        for (auto &n1 : n) {
                            nxt_tables.emplace(n1);
                        }
                    }
                    if (nxt_tables.size() == 0) {
                        nxt_table_lut = Stage::end_of_pipe();
                    } else if (nxt_tables.size() == 1) {
                        nxt_table_lut = miss_next.next_table_id();
                    } else {
                        nxt_table_lut = 0;
                    }

                    // 2 entries per stash unit
                    nxt_table_lut |= (nxt_table_lut << 8);

                    bitvec match_mask;
                    match_mask.setrange(0, 128);
                    // Since stash format can only have one entry (and no version bits) we
                    // generate the stash mask on exact match format with group 0
                    if (Format::Field *match = format->field("match", 0)) {
                        for (auto &piece : match->bits)
                            match_mask.clrrange(piece.lo, piece.hi + 1 - piece.lo);
                    }
                    for (int word = 0; word < 4; word++) {
                        stash_match_mask[word] = match_mask.getrange(word * 32, 32);
                    }
                } else {
                    stash_row_nxtable_bus_drive = 0;
                    stash_reg.stash_match_result_bus_select[stash_unit_id] = 0;
                    for (int word = 0; word < 4; word++) {
                        stash_match_mask[word] = 0;
                    }
                }
                input_data_ctl.stash_match_data_select = row.bus.at(Layout::SEARCH_BUS);
                input_data_ctl.stash_hashbank_select = row.bus.at(Layout::SEARCH_BUS);
                break;
            }
        }
    }
}

void ExactMatchTable::gen_tbl_cfg(json::vector &out) const {
    LOG3("### Exact match table " << name() << " gen_tbl_cfg " << loc());
    unsigned size = get_number_entries();
    json::map &tbl = *base_tbl_cfg(out, "match", size);
    add_all_reference_tables(tbl);
    json::map &stage_tbl = *add_common_sram_tbl_cfgs(tbl, "exact", "hash_match");
    add_pack_format(stage_tbl, format.get(), true, false);
    stage_tbl["memory_resource_allocation"] = nullptr;
    if (stash_rows.size() > 0) {
        json::map &stash_allocation = stage_tbl["stash_allocation"] = json::map();
        // Add 'action' field if present
        if (format && stash_format) {
            int group = 0;
            for (auto f = format->begin(group); f != format->end(group); f++) {
                if (f->first == "action") stash_format->add_field(f->second, f->first, group);
            }
        }
        add_pack_format(stash_allocation, stash_format.get(), false, true);
        auto mem_units_per_word = format ? format->get_mem_units_per_table_word() : 1;
        auto &stash_pack_formats = stash_allocation["pack_format"]->to<json::vector>();
        for (auto &stash_pack_format : stash_pack_formats) {
            json::map &pack = stash_pack_format->to<json::map>();
            pack["number_memory_units_per_table_word"] = mem_units_per_word;
            pack["table_word_width"] = MEM_WORD_WIDTH * mem_units_per_word;
        }
        auto num_stash_entries = stash_rows.size() / mem_units_per_word * 2;
        stash_allocation["num_stash_entries"] = num_stash_entries;
        json::vector &stash_entries = stash_allocation["stash_entries"] = json::vector();
        for (int k = 0; k < stash_rows.size() / mem_units_per_word; k++) {
            for (int i = 0; i < 2; i++) {
                json::vector stash_entry;
                for (int j = 0; j < mem_units_per_word; j++) {
                    auto stash_row = stash_rows[k * mem_units_per_word + j];
                    auto stash_col = stash_cols[k * mem_units_per_word + j];
                    auto stash_unit = stash_units[k * mem_units_per_word + j];
                    MemUnit stash_memunit(stash_row, stash_col);
                    json::map stash_entry_per_unit;
                    stash_entry_per_unit["stash_entry_id"] = (4 * stash_row) + (2 * stash_unit) + i;
                    for (auto &row : layout) {
                        if (row.row != stash_row) continue;
                        if (contains(row.memunits, stash_memunit)) {
                            int bus = row.bus.at(Layout::SEARCH_BUS);
                            stash_entry_per_unit["stash_match_data_select"] = bus;
                            stash_entry_per_unit["stash_hashbank_select"] = bus;
                            stash_entry_per_unit["hash_function_id"] = k;
                            break;
                        }
                    }
                    stash_entry.push_back(std::move(stash_entry_per_unit));
                }
                stash_entries.push_back(std::move(stash_entry));
            }
        }
    } else {
        stage_tbl["stash_allocation"] = nullptr;
    }
    json::map &match_attributes = tbl["match_attributes"];
    match_attributes["uses_dynamic_key_masks"] = dynamic_key_masks;
    if (ways.size() > 0) {
        json::vector &way_stage_tables = stage_tbl["ways"] = json::vector();
        unsigned way_number = 0;
        for (auto &way : ways) {
            json::map way_tbl;
            way_tbl["stage_number"] = stage->stageno;
            way_tbl["way_number"] = way_number++;
            way_tbl["stage_table_type"] = "hash_way";
            auto fmt_width = get_format_width();
            BUG_CHECK(fmt_width);
            unsigned ram_depth = way.rams.at(0).isLamb() ? LAMB_DEPTH : SRAM_DEPTH;
            way_tbl["size"] = way.rams.size() / fmt_width * format->groups() * ram_depth;
            add_pack_format(way_tbl, format.get(), false);
            way_tbl["memory_resource_allocation"] = gen_memory_resource_allocation_tbl_cfg(way);
            way_stage_tables.push_back(std::move(way_tbl));
        }
    }
    if (size == 0) {
        if (!match_attributes.count("match_type"))
            match_attributes["match_type"] = "match_with_no_key";
        if (!stage_tbl["stage_table_type"]) stage_tbl["stage_table_type"] = "match_with_no_key";
        stage_tbl["size"] = 1;
    }
    if (stage_tbl["stage_table_type"] == "hash_match") {
        // hash_match table schema requires 'hash_functions' and 'ways' so add (empty) if
        // they are not present
        if (!stage_tbl["hash_functions"]) stage_tbl["hash_functions"] = json::vector();
        if (!stage_tbl["ways"]) stage_tbl["ways"] = json::vector();
    }
}

/**
 * The ghost_bits information is required by the driver to correctly run an entry read from
 * hardware.  Ghost bits are bits that do not appear in the key, and must be calculated
 * from the hash matrix.
 *
 * The ghost_bits information is broken into two vectors:
 *
 * - ghost_bit_info: a vector of information on ghost bits, maps of 2 fields
 *      1. field_name - name of the field being ghosted
 *      2. bit_in_match_spec - awfully named for the field bit (not the bit in the entire key)
 *
 * - ghost_bit_to_hash_bit: a vector per each entry in the ghost_bit_info describing which
 *   hash bits coordinate to which ghost bits
 */
void ExactMatchTable::gen_ghost_bits(int hash_function_number,
                                     json::vector &ghost_bits_to_hash_bits,
                                     json::vector &ghost_bits_info) const {
    if (ghost_bit_positions.count(hash_function_number) == 0) return;
    auto ghost_bit_pos = ghost_bit_positions.at(hash_function_number);

    for (auto kv : ghost_bit_pos) {
        json::map ghost_bit_info;
        auto field_name = kv.first.first;
        auto global_name = field_name;
        auto p4_param = find_p4_param(field_name);
        if (p4_param && !p4_param->key_name.empty()) field_name = p4_param->key_name;
        ghost_bit_info["field_name"] = field_name;
        ghost_bit_info["global_name"] = global_name;
        ghost_bit_info["bit_in_match_spec"] = kv.first.second;
        ghost_bits_info.push_back(std::move(ghost_bit_info));

        json::vector ghost_bit_to_hash_bits;
        for (auto hash_bit : kv.second) ghost_bit_to_hash_bits.push_back(hash_bit);
        ghost_bits_to_hash_bits.push_back(std::move(ghost_bit_to_hash_bits));
    }
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(ExactMatchTable, TARGET_CLASS)
