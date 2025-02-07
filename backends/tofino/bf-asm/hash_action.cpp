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

#include <string_view>

#include "action_bus.h"
#include "input_xbar.h"
#include "misc.h"
#include "stage.h"
#include "tables.h"

// target specific instantiatitions

Table::Format::Field *HashActionTable::lookup_field(const std::string &n,
                                                    const std::string &act) const {
    auto *rv = format ? format->field(n) : nullptr;
    if (!rv && gateway) rv = gateway->lookup_field(n, act);
    if (!rv && !act.empty()) {
        if (auto call = get_action()) {
            rv = call->lookup_field(n, act);
        }
    }
    return rv;
}

void HashActionTable::setup(VECTOR(pair_t) & data) {
    common_init_setup(data, false, P4Table::MatchEntry);
    for (auto &kv : MapIterChecked(data, {"meter", "stats", "stateful"})) {
        if (kv.key == "search_bus" || kv.key == "result_bus") {
            // already dealt with in Table::setup_layout via common_init_setup
        } else if (!common_setup(kv, data, P4Table::MatchEntry)) {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (!action.set() && !actions)
        error(lineno, "Table %s has neither action table nor immediate actions", name());
    if (action.args.size() > 2)
        error(lineno, "Unexpected number of action table arguments %zu", action.args.size());
    if (actions && !action_bus) action_bus = ActionBus::create();
}

void HashActionTable::pass1() {
    LOG1("### Hash Action " << name() << " pass1 " << loc());
    MatchTable::pass1();
    for (auto &hd : hash_dist) {
        if (hd.xbar_use == 0) hd.xbar_use |= HashDistribution::ACTION_DATA_ADDRESS;
        hd.pass1(this, HashDistribution::OTHER, false);
    }
    if (!gateway && !hash_dist.empty())
        warning(hash_dist[0].lineno, "No gateway in hash_action means hash_dist can't be used");
}

void HashActionTable::pass2() {
    LOG1("### Hash Action " << name() << " pass2 " << loc());
    if (logical_id < 0) choose_logical_id();
    if (Target::GATEWAY_NEEDS_SEARCH_BUS()) {  // FIXME -- misnamed param?
        if (layout.size() != 1 || layout[0].bus.empty()) {
            error(lineno, "Need explicit row/bus in hash_action table");
        } else if (layout[0].bus.size() > 1) {
            error(lineno, "Can't have both bus and result_bus in hash_action table");
        } else {
            BUG_CHECK(layout[0].bus.count(Layout::RESULT_BUS), "should have result bus (only)");
        }
    }
    allocate_physical_ids();
    determine_word_and_result_bus();
    for (auto &ixb : input_xbar) ixb->pass2();
    if (actions) actions->pass2(this);
    if (action_bus) action_bus->pass2(this);
    if (gateway) gateway->pass2();
    if (idletime) idletime->pass2();
    for (auto &hd : hash_dist) hd.pass2(this);
}

/**
 * Again by definition, the bus of the hash action table by definition is the result bus
 */
void HashActionTable::determine_word_and_result_bus() {
    for (auto &row : layout) {
        row.word = 0;
    }
}

void HashActionTable::pass3() {
    LOG1("### Hash Action " << name() << " pass3 " << loc());
    MatchTable::pass3();
    if (action_bus) action_bus->pass3(this);
}

template <class REGS>
void HashActionTable::write_merge_regs_vt(REGS &regs, int type, int bus) {
    attached.write_merge_regs(regs, this, type, bus);
}

template <class REGS>
void HashActionTable::write_regs_vt(REGS &regs) {
    LOG1("### Hash Action " << name() << " write_regs " << loc());
    /* FIXME -- setup layout with no rams so other functions can write registers properly */
    int bus_type = layout[0].bus[Layout::RESULT_BUS] >> 1;
    MatchTable::write_regs(regs, bus_type, this);
    auto &merge = regs.rams.match.merge;
    merge.exact_match_logical_result_en |= 1 << logical_id;
    if (stage->tcam_delay(gress)) merge.exact_match_logical_result_delay |= 1 << logical_id;
    if (actions) actions->write_regs(regs, this);
    if (idletime) idletime->write_regs(regs);
    if (gateway) gateway->write_regs(regs);
    for (auto &hd : hash_dist) hd.write_regs(regs, this);
    if (options.match_compiler && !enable_action_data_enable &&
        (!gateway || gateway->empty_match())) {
        /* this seems unneeded? (won't actually be used...) */
        merge.next_table_format_data[logical_id].match_next_table_adr_default =
            merge.next_table_format_data[logical_id].match_next_table_adr_miss_value.value;
    }
}

/**
 * Unlike the hash functions for exact match tables, the hash action table does not require
 * the Galois position.  On the contrary, the hash action just requires an identity matrix
 * of what the address that is to be generated, as they simply use this address as a baseline
 * for generating the corresponding address.
 *
 * Thus, the hash function that is provided starts at bit 0, and is in reverse p4 param order.
 * This is under the guarantee that the compiler will allocate the hash in reverse p4 param
 * order as well.
 *
 * FIXME: Possibly this should be validated before this is the output, but currently the
 * compiler will set up the hash in that order
 */
void HashActionTable::add_hash_functions(json::map &stage_tbl) const {
    json::vector &hash_functions = stage_tbl["hash_functions"] = json::vector();

    if (input_xbar.empty()) return;
    BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
    auto &ht = input_xbar[0]->get_hash_tables();
    if (ht.size() == 0) return;

    int hash_bit_index = 0;
    json::map hash_function;
    json::vector &hash_bits = hash_function["hash_bits"] = json::vector();
    for (auto it = p4_params_list.rbegin(); it != p4_params_list.rend(); it++) {
        auto &p4_param = *it;
        for (size_t i = p4_param.start_bit; i < p4_param.start_bit + p4_param.bit_width; i++) {
            // Check if the param bit is used in hash function before adding to
            // json. E.g. The param can have a mask which will exclude some bits
            // to not be a part of the hash function
            if (!input_xbar[0]->is_p4_param_bit_in_hash(p4_param.name, i)) continue;

            json::map hash_bit;
            hash_bit["hash_bit"] = hash_bit_index;
            hash_bit["seed"] = 0;
            json::vector &bits_to_xor = hash_bit["bits_to_xor"] = json::vector();
            json::map field;
            std::string field_name, global_name;
            field_name = p4_param.key_name.empty() ? p4_param.name : p4_param.key_name;
            global_name = p4_param.name;
            field["field_bit"] = i;
            field["field_name"] = field_name;
            field["global_name"] = global_name;
            field["hash_match_group"] = 0;
            field["hash_match_group_bit"] = 0;
            bits_to_xor.push_back(std::move(field));
            hash_bits.push_back(std::move(hash_bit));

            hash_bit_index++;
        }
    }
    hash_function["hash_function_number"] = 0;
    hash_functions.push_back(std::move(hash_function));
}

void HashActionTable::gen_tbl_cfg(json::vector &out) const {
    // FIXME: Support multiple hash_dist's
    int size = hash_dist.empty() ? 1 : 1 + hash_dist[0].mask;
    json::map &tbl = *base_tbl_cfg(out, "match_entry", size);
    std::string_view stage_tbl_type = "match_with_no_key";
    size = 1;
    if (p4_table && p4_table->p4_stage_table_type() == "gateway_with_entries") {
        stage_tbl_type = "gateway_with_entries";
        size = p4_size();
    } else if (!p4_params_list.empty()) {
        stage_tbl_type = "hash_action";
        size = p4_size();
    }
    json::map &match_attributes = tbl["match_attributes"];
    json::vector &stage_tables = match_attributes["stage_tables"];
    json::map &stage_tbl = *add_stage_tbl_cfg(match_attributes, stage_tbl_type.data(), size);
    stage_tbl["memory_resource_allocation"] = nullptr;
    if (!match_attributes.count("match_type"))
        match_attributes["match_type"] = stage_tbl_type.data();
    // This is a only a glass required field, as it is only required when no default action
    // is specified, which is impossible for Brig through p4-16
    stage_tbl["default_next_table"] = Stage::end_of_pipe();
    add_pack_format(stage_tbl, 0, 0, hash_dist.empty() ? 1 : 0);
    add_result_physical_buses(stage_tbl);
    if (actions) {
        actions->gen_tbl_cfg(tbl["actions"]);
        actions->add_action_format(this, stage_tbl);
    } else if (action && action->actions) {
        action->actions->gen_tbl_cfg(tbl["actions"]);
        action->actions->add_action_format(this, stage_tbl);
    }
    common_tbl_cfg(tbl);
    if (stage_tbl_type == "hash_action" && !p4_params_list.empty()) add_hash_functions(stage_tbl);
    if (idletime)
        idletime->gen_stage_tbl_cfg(stage_tbl);
    else if (options.match_compiler)
        stage_tbl["stage_idletime_table"] = nullptr;
    add_all_reference_tables(tbl);
    gen_idletime_tbl_cfg(stage_tbl);
    merge_context_json(tbl, stage_tbl);
}

DEFINE_TABLE_TYPE(HashActionTable)
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void HashActionTable::write_merge_regs,
                      (mau_regs & regs, int type, int bus),
                      { write_merge_regs_vt(regs, type, bus); })
