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
#include "lib/hex.h"
#include "misc.h"

void AlgTcamMatchTable::setup(VECTOR(pair_t) & data) {
    common_init_setup(data, false, P4Table::MatchEntry);
    for (auto &kv : MapIterChecked(data, {"meter", "stats", "stateful"})) {
        if (common_setup(kv, data, P4Table::MatchEntry)) {
        } else if (kv.key == "number_partitions") {
            if (CHECKTYPE(kv.value, tINT)) number_partitions = kv.value.i;
        } else if (kv.key == "partition_field_name") {
            if (CHECKTYPE(kv.value, tSTR)) {
                partition_field_name = kv.value.s;
                if (auto *p = find_p4_param(partition_field_name))
                    if (!p->key_name.empty()) partition_field_name = p->key_name;
            }
        } else if (kv.key == "subtrees_per_partition") {
            if (CHECKTYPE(kv.value, tINT)) max_subtrees_per_partition = kv.value.i;
        } else if (kv.key == "bins_per_partition") {
            if (CHECKTYPE(kv.value, tINT)) bins_per_partition = kv.value.i;
        } else if (kv.key == "atcam_subset_width") {
            if (CHECKTYPE(kv.value, tINT)) atcam_subset_width = kv.value.i;
        } else if (kv.key == "shift_granularity") {
            if (CHECKTYPE(kv.value, tINT)) shift_granularity = kv.value.i;
        } else if (kv.key == "search_bus" || kv.key == "result_bus") {
            // already dealt with in Table::setup_layout via common_init_setup
        } else {
            common_sram_setup(kv, data);
        }
    }
    common_sram_checks();
}

// TODO: This could probably be rewritten in a simpler way. Below
// function checks the ways extracted from assembly for atcam and assumes the
// way no's are not sorted with column priority. Therefore the code sorts the
// first ram column and sets the column priority based on this column. Then this
// ordering is used to check if column priority is maintained if the ways are
// traversed in this column priority order for all other columns
void AlgTcamMatchTable::setup_column_priority() {
    int no_ways = ways.size();
    int no_entries_per_way = ways[0].rams.size();
    // FIXME-P4C: Ideally RAM's 6 & 7 can be on both left and right RAM Units.
    // Brig currently does not support this behavior and RAM 6 is always on
    // left, while RAM 7 on right. Once this supported is added below function
    // must be modified accordingly to accommodate these rams in lrams and rrams
    // and the traversal mechanism must be changed to determine column priority
    std::set<int> lrams = {2, 3, 4, 5, 6};
    std::set<int> rrams = {7, 8, 9, 10, 11};
    // Check if column is on left(0) or right(1) RAMs

    std::vector<std::pair<int, int>> first_entry_priority;
    // Determine the side and which way corresponds to which column
    int side = -1;
    for (int w = 0; w < no_ways; w++) {
        int col = ways[w].rams[0].col;
        int row = ways[w].rams[0].row;
        if (side == 0) {
            if (lrams.find(col) == lrams.end()) {
                error(lineno,
                      "ram(%d, %d) is not on correct side compared to rest in column "
                      "priority",
                      row, col);
            }
        } else if (side == 1) {
            if (rrams.find(col) == rrams.end()) {
                error(lineno,
                      "ram(%d, %d) is not on correct side compare to rest of column "
                      "priority",
                      row, col);
            }
        } else if (lrams.find(col) != lrams.end()) {
            side = 0;
        } else if (rrams.find(col) != rrams.end()) {
            side = 1;
        } else {
            error(lineno, "ram(%d, %d) invalid for ATCAM", row, col);
        }
        first_entry_priority.push_back(std::make_pair(w, col));
    }

    // Sort ways based on column priority for first column
    std::sort(first_entry_priority.begin(), first_entry_priority.end(),
              [side](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                  return side == 0 ? a.second < b.second : a.second > b.second;
              });

    int index = 0;
    for (auto &entry : first_entry_priority) {
        col_priority_way[index] = entry.first;
        index++;
    }

    // Ensure that the remaining columns match up with the first column ram
    for (int i = 1; i < no_entries_per_way; i++) {
        auto way_it = col_priority_way.begin();
        side = -1;
        int prev_col = -1;
        int prev_row = -1;
        while (way_it != col_priority_way.end()) {
            int row = ways[way_it->second].rams[i].row;
            int col = ways[way_it->second].rams[i].col;
            if (way_it != col_priority_way.begin()) {
                if (!(((side == 0 && prev_col < col && lrams.find(col) != lrams.end()) ||
                       (side == 1 && prev_col > col && rrams.find(col) != rrams.end())) &&
                      prev_row == row)) {
                    error(lineno,
                          "ram(%d, %d) and ram(%d, %d) column priority is not "
                          "compatible",
                          prev_row, prev_col, row, col);
                }
            }
            way_it++;
            prev_col = col;
            prev_row = row;
            if (lrams.find(col) != lrams.end())
                side = 0;
            else if (rrams.find(col) != rrams.end())
                side = 1;
            else
                error(lineno, "ram(%d, %d) invalid for ATCAM", row, col);
        }
    }
}

/**
 * Guarantees that the order of the entries provided in the ATCAM table format are order
 * in HW priority 0-4, (where in HW entry 4 will be favored).  This is required to guarantee that
 * the entries in the ATCAM pack format are in priority order for the driver.
 *
 * @seealso bf-p4c/mau/table_format.cpp - no_overhead_atcam_result_bus_words
 */
void AlgTcamMatchTable::verify_entry_priority() {
    int result_bus_word = -1;
    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        BUG_CHECK(group_info[i].result_bus_word >= 0);
        if (result_bus_word == -1) {
            result_bus_word = group_info[i].result_bus_word;
        } else if (result_bus_word != group_info[i].result_bus_word) {
            error(format->lineno, "ATCAM tables can at most have only one overhead word");
            return;
        }
        auto mg_it = group_info[i].match_group.find(result_bus_word);
        if (mg_it == group_info[i].match_group.end() || mg_it->second != i) {
            error(format->lineno,
                  "Each ATCAM entry must coordinate its entry with the "
                  "correct priority");
            return;
        }
    }

    if (word_info[result_bus_word].size() != group_info.size()) {
        error(format->lineno, "ATCAM tables do not chain to the same overhead word");
        return;
    }

    for (int i = 0; i < static_cast<int>(word_info[result_bus_word].size()); i++) {
        if (i != word_info[result_bus_word][i]) {
            error(format->lineno, "ATCAM priority not correctly formatted in the compiler");
            return;
        }
    }
}

/**
 * @seealso bf-p4c/mau/table_format.cpp - no_overhead_atcam_result_bus_words.  This matches
 * this function exactly
 */
void AlgTcamMatchTable::no_overhead_determine_result_bus_usage() {
    int result_bus_word = -1;
    int shared_groups = 0;
    for (int i = group_info.size() - 1; i >= 0; i--) {
        if (result_bus_word == -1) {
            result_bus_word = group_info[i].match_group.begin()->first;
        }
        bool is_shared_group = false;

        if (group_info[i].match_group.size() > 1)
            is_shared_group = true;
        else if (group_info[i].match_group.begin()->first != result_bus_word)
            is_shared_group = true;

        if (is_shared_group) {
            if (i > 1) error(format->lineno, "ATCAM chaining of shared groups is not correct");
            shared_groups++;
        }

        group_info[i].result_bus_word = result_bus_word;
        group_info[i].match_group[result_bus_word] = i;
    }

    word_info[result_bus_word].clear();
    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        word_info[result_bus_word].push_back(i);
    }

    if (shared_groups > 2)
        error(format->lineno, "ATCAM cannot safely send hit signals to same result bus");
}

void AlgTcamMatchTable::verify_format(Target::Tofino targ) {
    SRamMatchTable::verify_format(targ);
    if (!error_count) verify_entry_priority();
}

void AlgTcamMatchTable::pass1() {
    LOG1("### ATCAM match table " << name() << " pass1 " << loc());
    SRamMatchTable::pass1();
    if (format) {
        setup_column_priority();
        find_tcam_match();
    }
}

void AlgTcamMatchTable::setup_nibble_mask(Table::Format::Field *match, int group,
                                          std::map<int, match_element> &elems, bitvec &mask) {
    for (auto &el : Values(elems)) {
        int bit = match->bit(el.offset);
        if (match->hi(bit) < bit + el.width - 1)
            error(el.field->lineno, "match bits for %s not contiguous in match(%d)",
                  el.field->desc().c_str(), group);
        // Determining the nibbles dedicated to s0q1 or s1q0
        int start_bit = bit;
        int end_bit = start_bit + el.width - 1;
        int start_nibble = start_bit / 4U;
        int end_nibble = end_bit / 4U;
        mask.setrange(start_nibble, end_nibble - start_nibble + 1);
    }
}

void AlgTcamMatchTable::find_tcam_match() {
    std::map<Phv::Slice, match_element> exact;
    std::map<Phv::Slice, std::pair<match_element, match_element>> tcam;
    unsigned off = 0;
    /* go through the match fields and find duplicates -- those are the tcam matches */
    for (auto match_field : match) {
        auto phv_p = dynamic_cast<Phv::Ref *>(match_field);
        if (phv_p == nullptr) {
            BUG();
            continue;
        }
        auto phv_ref = *phv_p;
        auto sl = *phv_ref;
        if (!sl) continue;
        if (exact.count(sl)) {
            if (tcam.count(sl))
                error(phv_ref.lineno, "%s appears more than twice in atcam match",
                      phv_ref.desc().c_str());
            if ((sl.size() % 4U) != 0) {
                if ((sl.size() == 1) && (phv_ref.desc().find("$valid") != std::string::npos)) {
                } else
                    warning(phv_ref.lineno, "tcam match field %s not a multiple of 4 bits",
                            phv_ref.desc().c_str());
            }
            tcam.emplace(sl, std::make_pair(exact.at(sl),
                                            match_element{new Phv::Ref(phv_ref), off, sl->size()}));
            exact.erase(sl);
        } else {
            exact.emplace(sl, match_element{new Phv::Ref(phv_ref), off, sl->size()});
        }
        off += sl.size();
    }
    for (auto e : exact)
        for (auto t : tcam)
            if (e.first.overlaps(t.first))
                error(e.second.field->lineno, "%s overlaps %s in atcam match",
                      e.second.field->desc().c_str(), t.second.first.field->desc().c_str());
    if (error_count > 0) return;

    /* for the tcam pairs, treat first as s0q1 and second as s1q0 */
    for (auto &el : Values(tcam)) {
        s0q1[el.first.offset] = el.first;
        s1q0[el.second.offset] = el.second;
    }
    /* now find the bits in each group that match with the tcam pairs, ensure that they
     * are nibble-aligned, and setup the nibble masks */
    for (unsigned i = 0; i < format->groups(); i++) {
        if (Format::Field *match = format->field("match", i)) {
            setup_nibble_mask(match, i, s0q1, s0q1_nibbles);
            setup_nibble_mask(match, i, s1q0, s1q0_nibbles);
            if (!(s0q1_nibbles & s1q0_nibbles).empty())
                error(format->lineno, "Cannot determine if a ternary nibble is s0q1 or s1q0");
        } else {
            error(format->lineno, "no 'match' field in format group %d", i);
        }
    }
}

void AlgTcamMatchTable::pass2() {
    LOG1("### ATCAM match table " << name() << " pass2 " << loc());
    if (logical_id < 0) choose_logical_id();
    for (auto &ixb : input_xbar) ixb->pass2();
    setup_word_ixbar_group();
    ixbar_subgroup.resize(word_ixbar_group.size());
    ixbar_mask.resize(word_ixbar_group.size());
    // FIXME -- need a method of specifying these things in the asm code?
    // FIXME -- should at least check that these are sane
    for (unsigned i = 0; i < word_ixbar_group.size(); ++i) {
        if (word_ixbar_group[i] < 0) {
            // Word with no match data, only version/valid; used for direct lookup
            // tables -- can it happen with an atcam table?
            continue;
        }
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        bitvec ixbar_use = input_xbar[0]->hash_group_bituse(word_ixbar_group[i]);
        // Which 10-bit address group to use for this word -- use the lowest one with
        // a bit set in the hash group.  Can it be different for different words?
        ixbar_subgroup[i] = ixbar_use.min().index() / EXACT_HASH_ADR_BITS;
        // Assume that any hash bits usuable for select are used for select
        ixbar_mask[i] = ixbar_use.getrange(EXACT_HASH_FIRST_SELECT_BIT, EXACT_HASH_SELECT_BITS);
    }
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
    if (gateway) gateway->pass2();
    if (idletime) idletime->pass2();
    if (format) format->pass2(this);
    for (auto &hd : hash_dist) hd.pass2(this);
}

void AlgTcamMatchTable::pass3() {
    LOG1("### ATCAM match table " << name() << " pass3 " << loc());
    SRamMatchTable::pass3();
    if (action_bus) action_bus->pass3(this);
}

template <class REGS>
void AlgTcamMatchTable::write_regs_vt(REGS &regs) {
    LOG1("### ATCAM match table " << name() << " write_regs " << loc());
    SRamMatchTable::write_regs(regs);

    for (auto &row : layout) {
        auto &rams_row = regs.rams.array.row[row.row];
        for (auto &ram : row.memunits) {
            auto &way = way_map[ram];
            BUG_CHECK(ram.stage == INT_MIN && ram.row == row.row, "bogus %s in row %d", ram.desc(),
                      row.row);
            auto &ram_cfg = rams_row.ram[ram.col];
            ram_cfg.match_nibble_s0q1_enable = version_nibble_mask.getrange(way.word * 32U, 32) &
                                               ~s1q0_nibbles.getrange(way.word * 32U, 32);
            ram_cfg.match_nibble_s1q0_enable =
                0xffffffffUL & ~s0q1_nibbles.getrange(way.word * 32U, 32);
        }
    }
}

std::unique_ptr<json::vector> AlgTcamMatchTable::gen_memory_resource_allocation_tbl_cfg() const {
    if (col_priority_way.size() == 0)
        error(lineno, "No column priority determined for table %s", name());
    unsigned fmt_width = format ? (format->size + 127) / 128 : 0;
    json::vector mras;
    for (auto &entry : col_priority_way) {
        json::map mra;
        mra["column_priority"] = entry.first;
        json::vector mem_units;
        json::vector &mem_units_and_vpns = mra["memory_units_and_vpns"] = json::vector();
        auto &way = ways[entry.second];
        unsigned vpn_ctr = 0;
        for (auto &ram : way.rams) {
            if (mem_units.empty())
                vpn_ctr = layout_get_vpn(ram);
            else
                BUG_CHECK(vpn_ctr == layout_get_vpn(ram));
            mem_units.push_back(json_memunit(ram));
            if (mem_units.size() == fmt_width) {
                json::map tmp;
                tmp["memory_units"] = std::move(mem_units);
                mem_units = json::vector();
                json::vector vpns;
                // Because the entries in the context JSON are reversed, the VPNs have to
                // be reversed as well
                for (unsigned i = 0; i < format->groups(); i++) {
                    vpns.push_back(vpn_ctr + format->groups() - 1 - i);
                }
                vpn_ctr += format->groups();
                tmp["vpns"] = std::move(vpns);
                mem_units_and_vpns.push_back(std::move(tmp));
            }
        }
        BUG_CHECK(mem_units.empty());
        mras.push_back(std::move(mra));
    }
    return json::mkuniq<json::vector>(std::move(mras));
}

std::string AlgTcamMatchTable::get_match_mode(const Phv::Ref &pref, int offset) const {
    for (auto &p : s0q1) {
        if ((p.first == offset) && (*p.second.field == pref)) return "s0q1";
    }
    for (auto &p : s1q0) {
        if ((p.first == offset) && (*p.second.field == pref)) return "s1q0";
    }
    return "unused";
}

void AlgTcamMatchTable::gen_unit_cfg(json::vector &units, int size) const {
    json::map tbl;
    tbl["direction"] = P4Table::direction_name(gress);
    tbl["handle"] =
        p4_table ? is_alpm() ? p4_table->get_alpm_atcam_table_handle() : p4_table->get_handle() : 0;
    tbl["name"] = name();
    tbl["size"] = size;
    tbl["table_type"] = "match";
    json::map &stage_tbl =
        *add_common_sram_tbl_cfgs(tbl, "algorithmic_tcam_unit", "algorithmic_tcam_match");
    // Assuming atcam next hit table cannot be multiple tables
    stage_tbl["default_next_table"] =
        !hit_next.empty() ? hit_next[0].next_table_id() : Target::END_OF_PIPE();
    stage_tbl["memory_resource_allocation"] = gen_memory_resource_allocation_tbl_cfg();
    // Hash functions not necessary currently for ATCAM matches, as the result comes from
    // the partition_field_name
    stage_tbl["hash_functions"] = json::vector();
    add_pack_format(stage_tbl, format.get(), false);
    units.push_back(std::move(tbl));
}

bool AlgTcamMatchTable::has_directly_attached_synth2port() const {
    auto mt = this;
    if (auto a = mt->get_attached()) {
        if (a->selector && is_directly_referenced(a->selector)) return true;
        for (auto &m : a->meters) {
            if (is_directly_referenced(m)) return true;
        }
        for (auto &s : a->stats) {
            if (is_directly_referenced(s)) return true;
        }
        for (auto &s : a->statefuls) {
            if (is_directly_referenced(s)) return true;
        }
    }
    return false;
}

void AlgTcamMatchTable::gen_alpm_cfg(json::map &tbl) const {
    tbl["default_action_handle"] = get_default_action_handle();
    tbl["action_profile"] = action_profile();
    // FIXME -- setting next_table_mask unconditionally only works because we process the
    // stage table in stage order (so we'll end up with the value from the last stage table,
    // which is what we want.)  Should we check in case the ordering ever changes?
    tbl["default_next_table_mask"] = next_table_adr_mask;
    // FIXME -- the driver currently always assumes this is 0, so we arrange for it to be
    // when choosing the action encoding.  But we should be able to choose something else
    tbl["default_next_table_default"] = 0;
    // FIXME-JSON: PD related, check glass examples for false (ALPM)
    tbl["is_resource_controllable"] = true;
    tbl["uses_range"] = false;
    if (p4_table && p4_table->disable_atomic_modify) tbl["disable_atomic_modify"] = true;
    tbl["ap_bind_indirect_res_to_match"] = json::vector();
    tbl["static_entries"] = json::vector();
    if (context_json) {
        add_json_node_to_table(tbl, "ap_bind_indirect_res_to_match");
    }
    LOG1("populate alpm " << name());
    // FIXME-DRIVER
    // 'actions' and 'table_refs' on the alpm are redundant as they are
    // already present in the atcam table. These should probably be cleaned
    // up from the context json and driver parsing.
    if (actions) {
        actions->gen_tbl_cfg(tbl["actions"]);
    } else if (action && action->actions) {
        action->actions->gen_tbl_cfg(tbl["actions"]);
    }
    add_all_reference_tables(tbl);
    json::map &alpm_match_attributes = tbl["match_attributes"];
    alpm_match_attributes["max_subtrees_per_partition"] = max_subtrees_per_partition;
    alpm_match_attributes["partition_field_name"] = get_partition_field_name();
    alpm_match_attributes["lpm_field_name"] = get_lpm_field_name();
    alpm_match_attributes["bins_per_partition"] = bins_per_partition;
    alpm_match_attributes["atcam_subset_width"] = atcam_subset_width;
    alpm_match_attributes["shift_granularity"] = shift_granularity;
    if (context_json) {
        add_json_node_to_table(alpm_match_attributes, "excluded_field_msb_bits");
    }
    auto pa_hdl = get_partition_action_handle();
    // Throw an error if partition action handle is not set. The alpm
    // pre-classifier should have a single action which sets the partition
    // handle. If no handle is present, it is either not generated by the
    // compiler or assembler is not able to find it within actions. In
    // either case this is a problem as driver will error out
    if (pa_hdl.empty())
        error(lineno, "Cannot find partition action handle for ALPM table %s", name());
    // backward-compatible mode
    if (pa_hdl.size() == 1) {
        alpm_match_attributes["set_partition_action_handle"] = *pa_hdl.begin();
    } else {
        json::vector &action_handles = alpm_match_attributes["set_partition_action_handle"] =
            json::vector();
        for (auto hdl : pa_hdl) action_handles.push_back(hdl);
    }
    alpm_match_attributes["stage_tables"] = json::vector();
}

void AlgTcamMatchTable::gen_tbl_cfg(json::vector &out) const {
    json::map *atcam_tbl_ptr;
    unsigned number_entries = get_number_entries();
    if (is_alpm()) {
        // Add ALPM ATCAM config to ALPM table (generated by pre-classifier in
        // previous ostage)
        json::map *alpm_tbl_ptr = base_tbl_cfg(out, "match", number_entries);
        if (!alpm_tbl_ptr) {
            error(lineno, "No alpm table generated by alpm pre-classifier");
            return;
        }
        json::map &alpm_tbl = *alpm_tbl_ptr;
        gen_alpm_cfg(alpm_tbl);
        json::map &alpm_match_attributes = alpm_tbl["match_attributes"];
        json::map &atcam_tbl = alpm_match_attributes["atcam_table"];
        base_alpm_atcam_tbl_cfg(atcam_tbl, "match", number_entries);
        atcam_tbl_ptr = &atcam_tbl;
    } else {
        atcam_tbl_ptr = base_tbl_cfg(out, "match", number_entries);
    }
    json::map &tbl = *atcam_tbl_ptr;
    common_tbl_cfg(tbl);
    json::map &match_attributes = tbl["match_attributes"];
    match_attributes["match_type"] = "algorithmic_tcam";
    if (actions) {
        actions->gen_tbl_cfg(tbl["actions"]);
    } else if (action && action->actions) {
        action->actions->gen_tbl_cfg(tbl["actions"]);
    }
    json::vector &units = match_attributes["units"];
    gen_unit_cfg(units, number_entries);
    match_attributes["number_partitions"] = number_partitions;
    match_attributes["partition_field_name"] = partition_field_name;
    add_all_reference_tables(tbl);
    if (units.size() > 1 && has_directly_attached_synth2port())
        error(lineno,
              "The ability to split directly addressed counters/meters/stateful "
              "resources across multiple logical tables of an algorithmic tcam match table "
              "is not currently supported.");
    // Empty stage table node in atcam. These are moved inside the
    // units->MatchTable->stage_table node
    match_attributes["stage_tables"] = json::vector();
}

DEFINE_TABLE_TYPE(AlgTcamMatchTable)
