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

#include <config.h>

#include <unordered_map>

#include "action_bus.h"
#include "lib/algorithm.h"
#include "input_xbar.h"
#include "instruction.h"
#include "misc.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"

Table::Format *MatchTable::get_format() const {
    if (!format && gateway) return gateway->get_format();
    return format.get();
}

Table::Format::Field *MatchTable::lookup_field(const std::string &n, const std::string &act) const {
    auto *rv = format ? format->field(n) : nullptr;
    if (!rv && gateway) rv = gateway->lookup_field(n, act);
    return rv;
}

void MatchTable::common_init_setup(const VECTOR(pair_t) & data, bool ternary,
                                   P4Table::type p4type) {
    Table::common_init_setup(data, ternary, p4type);
    setup_logical_id();
    if (Target::DYNAMIC_CONFIG())
        if (auto *dconfig = get(data, "dynamic_config"))
            if (CHECKTYPESIZE(*dconfig, tMAP))
                for (auto &kv : dconfig->map) dynamic_config.emplace_back(this, kv);
    for (auto &kv : data)
        if (kv.key == "input_xbar" && CHECKTYPESIZE(kv.value, tMAP))
            input_xbar.emplace_back(InputXbar::create(this, ternary, kv.key, kv.value.map));
}

bool MatchTable::common_setup(pair_t &kv, const VECTOR(pair_t) & data, P4Table::type p4type) {
    if (Table::common_setup(kv, data, p4type)) {
        return true;
    }
    if (kv.key == "input_xbar" || kv.key == "hash_dist") {
        /* done in common_init_setup */
        return true;
    }
    if (kv.key == "dynamic_config" && Target::DYNAMIC_CONFIG()) {
        /* done in common_init_setup */
        return true;
    }
    if (kv.key == "always_run") {
        if ((always_run = get_bool(kv.value)) && !Target::SUPPORT_ALWAYS_RUN())
            error(kv.key.lineno, "always_run not supported on %s", Target::name());
        return true;
    }
    if (kv.key == "gateway") {
        if (CHECKTYPE(kv.value, tMAP)) {
            gateway = GatewayTable::create(kv.key.lineno, name_ + " gateway", gress, stage, -1,
                                           kv.value.map);
            gateway->set_match_table(this, false);
        }
        return true;
    }
    if (kv.key == "idletime") {
        if (CHECKTYPE(kv.value, tMAP)) {
            idletime = IdletimeTable::create(kv.key.lineno, name_ + " idletime", gress, stage, -1,
                                             kv.value.map);
            idletime->set_match_table(this, false);
        }
        return true;
    }
    if (kv.key == "selector") {
        attached.selector.setup(kv.value, this);
        return true;
    }
    if (kv.key == "selector_length") {
        attached.selector_length.setup(kv.value, this);
        return true;
    }
    if (kv.key == "meter_color") {
        attached.meter_color.setup(kv.value, this);
        return true;
    }
    if (kv.key == "stats") {
        if (kv.value.type == tVEC)
            for (auto &v : kv.value.vec) attached.stats.emplace_back(v, this);
        else
            attached.stats.emplace_back(kv.value, this);
        return true;
    }
    if (kv.key == "meter") {
        if (kv.value.type == tVEC)
            for (auto &v : kv.value.vec) attached.meters.emplace_back(v, this);
        else
            attached.meters.emplace_back(kv.value, this);
        return true;
    }
    if (kv.key == "stateful") {
        if (kv.value.type == tVEC)
            for (auto &v : kv.value.vec) attached.statefuls.emplace_back(v, this);
        else
            attached.statefuls.emplace_back(kv.value, this);
        return true;
    }
    if (kv.key == "table_counter") {
        if (kv.value == "table_miss")
            table_counter = TABLE_MISS;
        else if (kv.value == "table_hit")
            table_counter = TABLE_HIT;
        else if (kv.value == "gateway_miss")
            table_counter = GATEWAY_MISS;
        else if (kv.value == "gateway_hit")
            table_counter = GATEWAY_HIT;
        else if (kv.value == "gateway_inhibit")
            table_counter = GATEWAY_INHIBIT;
        else if (kv.value == "disabled")
            table_counter = DISABLED;
        else
            error(kv.value.lineno, "Invalid table counter %s", value_desc(kv.value));
        return true;
    }
    return false;
}

bool MatchTable::is_attached(const Table *tbl) const {
    return tbl && (tbl == gateway || tbl == idletime || get_attached()->is_attached(tbl));
}

bitvec MatchTable::compute_reachable_tables() {
    Table::compute_reachable_tables();
    if (gateway) reachable_tables_ |= gateway->reachable_tables();
    if (idletime) reachable_tables_ |= idletime->reachable_tables();
    reachable_tables_ |= attached.compute_reachable_tables();
    return reachable_tables_;
}

/**
 * Return the first default found meter type of a stateful/meter call.  If the meter type
 * is considered to be default, then all of the meter types would be identical
 */
METER_ACCESS_TYPE MatchTable::default_meter_access_type(bool for_stateful) {
    METER_ACCESS_TYPE rv = NOP;
    auto actions = get_actions();
    if (actions == nullptr) return rv;
    for (auto it = actions->begin(); it != actions->end(); it++) {
        if (it->default_only) continue;
        for (auto &call : it->attached) {
            auto type = call->table_type();
            if (!((type == Table::METER && !for_stateful) ||
                  (type == Table::STATEFUL && for_stateful)))
                continue;
            // Currently the first argument is the meter type
            if (call.args[0].type == Table::Call::Arg::Const) {
                return static_cast<METER_ACCESS_TYPE>(call.args[0].value());
            } else if (auto n = call.args[0].name()) {
                if (auto *st = call->to<StatefulTable>()) {
                    if (auto *act = st->actions->action(call.args[0].name())) {
                        return static_cast<METER_ACCESS_TYPE>((act->code << 1) | 1);
                    }
                }
            }
        }
    }
    return rv;
}

std::vector<Table::Call> MatchTable::get_calls() const {
    std::vector<Call> rv = Table::get_calls();
    if (attached.selector) rv.emplace_back(attached.selector);
    if (attached.selector_length) rv.emplace_back(attached.selector_length);
    for (auto &c : attached.stats)
        if (c) rv.emplace_back(c);
    for (auto &c : attached.meters)
        if (c) rv.emplace_back(c);
    for (auto &c : attached.statefuls)
        if (c) rv.emplace_back(c);
    if (attached.meter_color) rv.emplace_back(attached.meter_color);
    return rv;
}

void MatchTable::pass0() {
    LOG1("### match table " << name() << " pass0 " << loc());
#if 0
    // redundant with (and supercedes) choose_logical_id in pass2.  That function is much
    // better, taking dependencies into account, so logical_id should not be allocated here
    alloc_id("logical", logical_id, stage->pass1_logical_id,
             LOGICAL_TABLES_PER_STAGE, true, stage->logical_id_use);
#endif
    if (logical_id >= 0) {
        if (stage->logical_id_use[logical_id] && stage->logical_id_use[logical_id] != this) {
            error(lineno, "Duplicate logical id %d use", logical_id);
            error(stage->logical_id_use[logical_id]->lineno, "previous use here");
        }
        stage->logical_id_use[logical_id] = this;
    }
    for (auto physid : physical_ids) {
        if (stage->physical_id_use[physid] && stage->physical_id_use[physid] != this) {
            error(lineno, "Duplicate physical id %d use", physid);
            error(stage->physical_id_use[physid]->lineno, "previous use here");
        }
        stage->physical_id_use[physid] = this;
    }
    if (action.check() && action->set_match_table(this, !action.is_direct_call()) != ACTION)
        error(action.lineno, "%s is not an action table", action->name());
    attached.pass0(this);
}

void MatchTable::pass1() {
    if (gateway) {
        // needs to happen before Actions::pass1 so that extra_next_lut is setup
        gateway->setup_map_indexing(this);
    }
    Table::pass1();
    if (!p4_table)
        p4_table = P4Table::alloc(P4Table::MatchEntry, this);
    else
        p4_table->check(this);
    // Set up default action. This will look up action and/or tind for default
    // action if the match_table doesnt have one specified
    if (default_action.empty()) default_action = get_default_action();
    if (table_counter >= GATEWAY_MISS && !gateway)
        error(lineno, "Can't count gateway events on table %s as it doesn't have a gateway",
              name());
    if (!p4_params_list.empty()) {
        for (auto &p : p4_params_list) {
            // bit_width_full should be generated in assembly as 'full_size' in
            // the 'p4_param_order'. This is the full size of the field as used
            // in p4 program.
            if (!p.bit_width_full) p.bit_width_full = p.bit_width;

            std::size_t found = p.name.find(".$valid");
            if (found != std::string::npos) p.is_valid = true;
        }
    }
    if (idletime) {
        idletime->logical_id = logical_id;
        idletime->pass1();
    }
    for (auto &ixb : input_xbar) ixb->pass1();
    for (auto &hd : hash_dist) hd.pass1(this, HashDistribution::OTHER, false);
    if (gateway) {
        gateway->logical_id = logical_id;
        gateway->pass1();
    }
}

void Table::allocate_physical_ids(unsigned usable) {
    if (physical_ids) {
        auto unusable = physical_ids - bitvec(usable);
        BUG_CHECK(unusable.empty(), "table %s using physical id %d which appears to be invalid",
                  name(), *unusable.begin());
        return;
    }
    if (!Target::MATCH_REQUIRES_PHYSID()) return;
    for (int i = 0; i < PHYSICAL_TABLES_PER_STAGE; ++i) {
        if (!((usable >> i) & 1)) continue;
        if (stage->physical_id_use[i]) continue;
        physical_ids[i] = 1;
        stage->physical_id_use[i] = this;
        return;
    }
    error(lineno, "No physical id available for table %s", name());
}

void MatchTable::pass3() {
    if (gateway) {
        gateway->pass3();
    }
}

void MatchTable::gen_idletime_tbl_cfg(json::map &stage_tbl) const {
    if (idletime) idletime->gen_stage_tbl_cfg(stage_tbl);
}

#include "tofino/match_table.cpp"  // NOLINT(build/include)
#if HAVE_JBAY
#include "jbay/match_table.cpp"  // NOLINT(build/include)
#endif                           /* HAVE_JBAY */

template <class TARGET>
void MatchTable::write_common_regs(typename TARGET::mau_regs &regs, int type, Table *result) {
    /* this follows the order and behavior in stage_match_entry_table.py
     * it can be reorganized to be clearer */

    /*------------------------
     * data path
     *-----------------------*/
    if (gress == EGRESS) regs.dp.imem_table_addr_egress |= 1 << logical_id;

    /*------------------------
     * Match Merge
     *-----------------------*/
    auto &merge = regs.rams.match.merge;
    auto &adrdist = regs.rams.match.adrdist;
    if (gress != GHOST) merge.predication_ctl[gress].table_thread |= 1 << logical_id;
    if (gress == INGRESS || gress == GHOST) {
        merge.logical_table_thread[0].logical_table_thread_ingress |= 1 << logical_id;
        merge.logical_table_thread[1].logical_table_thread_ingress |= 1 << logical_id;
        merge.logical_table_thread[2].logical_table_thread_ingress |= 1 << logical_id;
    } else if (gress == EGRESS) {
        merge.logical_table_thread[0].logical_table_thread_egress |= 1 << logical_id;
        merge.logical_table_thread[1].logical_table_thread_egress |= 1 << logical_id;
        merge.logical_table_thread[2].logical_table_thread_egress |= 1 << logical_id;
    }
    adrdist.adr_dist_table_thread[timing_thread(gress)][0] |= 1 << logical_id;
    adrdist.adr_dist_table_thread[timing_thread(gress)][1] |= 1 << logical_id;

    Actions *actions = action && action->actions ? action->actions.get() : this->actions.get();

    std::set<int> result_buses;
    if (result) {
        actions = result->action && result->action->actions ? result->action->actions.get()
                                                            : result->actions.get();
        for (auto &row : result->layout) {
            int r_bus = row.row * 2;
            if (row.bus.count(Layout::RESULT_BUS))
                r_bus += row.bus.at(Layout::RESULT_BUS) & 1;
            else if (row.bus.count(Layout::TIND_BUS))
                r_bus += row.bus.at(Layout::TIND_BUS);
            else
                continue;
            result_buses.insert(r_bus);
        }
    } else {
        /* ternary match with no indirection table */
        auto tern_table = this->to<TernaryMatchTable>();
        BUG_CHECK(tern_table != nullptr);
        if (tern_table->indirect_bus >= 0) result_buses.insert(tern_table->indirect_bus);
        result = this;
    }

    for (auto r_bus : result_buses) {
        auto &shift_en = merge.mau_payload_shifter_enable[type][r_bus];
        setup_muxctl(merge.match_to_logical_table_ixbar_outputmap[type][r_bus], logical_id);
        setup_muxctl(merge.match_to_logical_table_ixbar_outputmap[type + 2][r_bus], logical_id);

        int default_action = 0;
        unsigned adr_mask = 0;
        unsigned adr_default = 0;
        unsigned adr_per_entry_en = 0;

        /**
         * This section of code determines the registers required to determine the
         * instruction code to run for this particular table.  This uses the information
         * provided by the instruction code.
         *
         * The address is built of two parts, the instruction code and the per flow enable
         * bit.  These can either come from overhead, or from the default register.
         * The keyword $DEFAULT indicates that the value comes from the default
         * register
         */
        auto instr_call = instruction_call();
        // FIXME: Workaround until a format is provided on the gateway to find the
        // action bit section.  This will be a quick add on.
        if (instr_call.args[0] == "$DEFAULT") {
            for (auto it = actions->begin(); it != actions->end(); it++) {
                if (it->code != -1) {
                    adr_default |= it->addr;
                    break;
                }
            }
        } else if (auto field = instr_call.args[0].field()) {
            adr_mask |= (1U << field->size) - 1;
        }

        if (instr_call.args[1] == "$DEFAULT") {
            adr_default |= ACTION_INSTRUCTION_ADR_ENABLE;
        } else if (auto field = instr_call.args[1].field()) {
            if (auto addr_field = instr_call.args[0].field()) {
                adr_per_entry_en = field->bit(0) - addr_field->bit(0);
            } else {
                adr_per_entry_en = 0;
            }
        }
        shift_en.action_instruction_adr_payload_shifter_en = 1;
        merge.mau_action_instruction_adr_mask[type][r_bus] = adr_mask;
        merge.mau_action_instruction_adr_default[type][r_bus] = adr_default;
        merge.mau_action_instruction_adr_per_entry_en_mux_ctl[type][r_bus] = adr_per_entry_en;

        if (idletime) idletime->write_merge_regs(regs, type, r_bus);
        if (result->action) {
            if (auto adt = result->action->to<ActionTable>()) {
                merge.mau_actiondata_adr_default[type][r_bus] =
                    adt->determine_default(result->action);
            }
            shift_en.actiondata_adr_payload_shifter_en = 1;
        }
        if (!get_attached()->stats.empty()) shift_en.stats_adr_payload_shifter_en = 1;
        if (!get_attached()->meters.empty() || !get_attached()->statefuls.empty())
            shift_en.meter_adr_payload_shifter_en = 1;

        result->write_merge_regs(regs, type, r_bus);
    }

    /*------------------------
     * Action instruction Address
     *-----------------------*/
    int max_code = actions->max_code;
    if (options.match_compiler)
        if (auto *action_format = lookup_field("action"))
            max_code = (1 << (action_format->size - (gateway ? 1 : 0))) - 1;
    /**
     * The action map can be used if the choices for the instruction are < 8.  The map data
     * table will be used if the number of choices are between 2 and 8, and references
     * the instruction call to determine whether the instruction comes from the map
     * data table or the default register.
     */
    auto instr_call = instruction_call();
    bool use_action_map =
        instr_call.args[0].field() && max_code < ACTION_INSTRUCTION_SUCCESSOR_TABLE_DEPTH;
    // FIXME: Workaround until a format is provided on the gateway to find the
    // action bit section.  This will be a quick add on.

    if (use_action_map) {
        merge.mau_action_instruction_adr_map_en[type] |= (1U << logical_id);
        for (auto &act : *actions)
            if ((act.name != result->default_action) || !result->default_only_action) {
                merge.mau_action_instruction_adr_map_data[type][logical_id][act.code / 4]
                    .set_subfield(act.addr + ACTION_INSTRUCTION_ADR_ENABLE,
                                  (act.code % 4) * TARGET::ACTION_INSTRUCTION_MAP_WIDTH,
                                  TARGET::ACTION_INSTRUCTION_MAP_WIDTH);
            }
    }

    /**
     * This register is now the responsiblity of the driver for all tables, as the driver
     * will initialize this value from the initial default action.  If we ever want to
     * move some of this responsibility back to the compiler, then this code can be used
     * for this, but it is currently incorrect for tables that have been split across
     * multiple stages for non noop default actions.
    if (this->to<HashActionTable>()) {
        merge.mau_action_instruction_adr_miss_value[logical_id] = 0;
    } else if (!default_action.empty()) {
        auto *act = actions->action(default_action);
        merge.mau_action_instruction_adr_miss_value[logical_id] =
            ACTION_INSTRUCTION_ADR_ENABLE + act->addr;
    } else if (!result->default_action.empty()) {
        auto *act = actions->action(result->default_action);
        merge.mau_action_instruction_adr_miss_value[logical_id] =
            ACTION_INSTRUCTION_ADR_ENABLE + act->addr; }
    */

    /**
     * No direct call for a next table, like instruction.  The next table can be determined
     * from other parameters.  If there is a next parameter in the format, then this is the
     * field to be used as an extractor.
     *
     * If there is no next field, but there is more than one possible entry in the hitmap,
     * then the action instruction is being used as the index.
     *
     * If necessary, i.e. something becomes more complex, then perhaps a call needs to be
     * added.
     *
     * Also, a quick note that though the match_next_table_adr_default is not necessary to set,
     * the diagram in 6.4.3.3. Next Table Processing, the default register is after the mask.
     * However, in hardware, the default register is before the mask.
     */
    int next_field_size = result->get_format_field_size("next");
    int action_field_size = result->get_format_field_size("action");

    if (next_field_size > 0) {
        next_table_adr_mask = ((1U << next_field_size) - 1);
    } else if (result->get_hit_next().size() > 1) {
        next_table_adr_mask = ((1U << action_field_size) - 1);
    }
    write_next_table_regs(regs, result);

    /*------------------------
     * Immediate data found in overhead
     *-----------------------*/
    if (result->format) {
        for (auto &row : result->layout) {
            int r_bus = row.row * 2;
            if (row.bus.count(Layout::RESULT_BUS))
                r_bus += row.bus.at(Layout::RESULT_BUS) & 1;
            else if (row.bus.count(Layout::TIND_BUS))
                r_bus += row.bus.at(Layout::TIND_BUS);
            else
                continue;
            merge.mau_immediate_data_mask[type][r_bus] = bitMask(result->format->immed_size);
            if (result->format->immed_size > 0)
                merge.mau_payload_shifter_enable[type][r_bus].immediate_data_payload_shifter_en = 1;
        }
    }
    if (result->action_bus) {
        result->action_bus->write_immed_regs(regs, result);
        for (auto &mtab : get_attached()->meters) {
            // if the meter table outputs something on the action-bus of the meter
            // home row, need to set up the action hv xbar properly
            result->action_bus->write_action_regs(regs, result, mtab->home_row(), 0);
        }
        for (auto &stab : get_attached()->statefuls) {
            // if the stateful table outputs something on the action-bus of the meter
            // home row, need to set up the action hv xbar properly
            result->action_bus->write_action_regs(regs, result, stab->home_row(), 0);
        }
    }

    // FIXME:
    // The action parameters that are stored as immediates in the match
    // overhead need to be properly packed into this register. We had been
    // previously assuming that the compiler would do that for us, specifying
    // the bits needed here as the argument to the action call; eg assembly
    // code like:
    //         default_action: actname(0x100)
    // for the default action being actname with the value 0x100 for its
    // parameters stored as immediates (which might actually be several
    // parameters in the P4 source code.) To get this from the
    // default_action_parameters map, we need to look up those argument names
    // in the match table format and action aliases and figure out which ones
    // correspond to match immediates, and pack the values appropriately.
    // Doable but non-trivial, probably requiring a small helper function. Need
    // to deal with both exact match and ternary indirect.
    //
    // For now, most miss configuration registers are only written by the driver
    // (since the user API says what miss behavior to perform). The compiler
    // (glass) relies on the driver to write them but this could change in
    // future. This particular register would only be set if the compiler chose
    // to allocate action parameters in match overhead.
    //
    // if (default_action_parameters.size() > 0)
    //     merge.mau_immediate_data_miss_value[logical_id] = default_action_parameters[0];
    // else if (result->default_action_parameters.size() > 0)
    //     merge.mau_immediate_data_miss_value[logical_id] = result->default_action_parameters[0];

    for (auto &ixb : input_xbar) ixb->write_regs(regs);
    /* DANGER -- you might think we should call write_regs on other related things here
     * (actions, hash_dist, idletime, gateway) rather than just input_xbar, but those are
     * all called by the various callers of this method.  Not clear why input_xbar is
     * different */

    if (gress == EGRESS) regs.cfg_regs.mau_cfg_lt_thread |= 1U << logical_id;
    if (options.match_compiler && dynamic_cast<HashActionTable *>(this)) return;  // skip the rest

    if (table_counter) {
        merge.mau_table_counter_ctl[logical_id / 8U].set_subfield(table_counter,
                                                                  3 * (logical_id % 8U), 3);
    } else {  // Set to TABLE_HIT by default
        merge.mau_table_counter_ctl[logical_id / 8U].set_subfield(TABLE_HIT, 3 * (logical_id % 8U),
                                                                  3);
    }
}

int MatchTable::get_address_mau_actiondata_adr_default(unsigned log2size, bool per_flow_enable) {
    int huffman_ones = log2size > 2 ? log2size - 3 : 0;
    BUG_CHECK(huffman_ones < 7);
    int rv = (1 << huffman_ones) - 1;
    rv = ((rv << 10) & 0xf8000) | (rv & 0x1f);
    if (!per_flow_enable) rv |= 1 << 22;
    return rv;
}

/**
 * Generates the hash_bits node for a single hash_function node in the JSON.
 *
 * Will add the impact of a single hash_table (64 bit section of the input xbar) to the hash
 * bits.  If the table requires multiple hash_tables, then the previous hash table value will
 * be looked up and added.  FIXME: At some point refactor this function to not keep
 * doing this rewrite.
 *
 * The JSON for each hash bit has the following:
 *     hash_bit - The hash bit in which this is output on the Galois matrix.  (Really whatever
 *         this bit position is just has to coordinate across the other driver structures, but
 *         those are also based on the Galois matrix position).
 *     seed - the bit that is xored in at the end of the calcuation
 *     bits_to_xor - The field bits from the P4 API that will determine the value of this bit,
 *         and must be XORed for this bit.  This is a vector of fields with 4 values.
 *         - field_bit - The p4 field bit to be XORed
 *         - field_name - The p4 field name to be XORed
 *         The next two parameters are only needed for dynamic_key_masks, as they indicate
 *         to the driver which bit to turn off
 *         - hash_match_group - Which 128 bit input xbar group this bit is appearing in (0-7)
 *         - hash_match_group_bit - The bit offset within the 128 bit input xbar group.
 */
void MatchTable::gen_hash_bits(const std::map<int, HashCol> &hash_table,
                               InputXbar::HashTable hash_table_id, json::vector &hash_bits,
                               unsigned hash_group_no, bitvec hash_bits_used) const {
    for (auto &col : hash_table) {
        if (!hash_bits_used.getbit(col.first)) continue;
        json::map hash_bit;
        bool hash_bit_added = false;
        json::vector *bits_to_xor = nullptr;
        // FIXME: This function has a lot of unnecessary copying and moving around.
        for (auto &hb : hash_bits) {
            if (hb->to<json::map>()["hash_bit"]->to<json::number>() == json::number(col.first)) {
                bits_to_xor = &(hb->to<json::map>()["bits_to_xor"]->to<json::vector>());
                hash_bit_added = true;
            }
        }
        if (!hash_bit_added) bits_to_xor = &(hash_bit["bits_to_xor"] = json::vector());
        hash_bit["hash_bit"] = col.first;
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        hash_bit["seed"] = input_xbar[0]->get_seed_bit(hash_group_no, col.first);
        for (const auto &bit : col.second.data) {
            if (auto ref = input_xbar[0]->get_hashtable_bit(hash_table_id, bit)) {
                std::string field_name, global_name;
                field_name = ref.name();

                auto field_bit = remove_name_tail_range(field_name) + ref.lobit();
                global_name = field_name;

                // Look up this field in the param list to get a custom key
                // name, if present.
                auto p = find_p4_param(field_name);
                if (!p && !p4_params_list.empty()) {
                    warning(col.second.lineno,
                            "Cannot find field name %s in p4_param_order "
                            "for table %s",
                            field_name.c_str(), name());
                } else if (p && !p->key_name.empty()) {
                    field_name = p->key_name;
                }
                auto group = input_xbar[0]->hashtable_input_group(hash_table_id);
                int group_bit = bit;
                // FIXME -- this adjustment is a hack for tofino1/2.  Should have a virtual
                // method on InputXbar?  or something in Target?
                if (group.index != hash_table_id.index && (hash_table_id.index & 1))
                    group_bit += 64;
                bits_to_xor->push_back(
                    json::map{{"field_bit", json::number(field_bit)},
                              {"field_name", json::string(field_name)},
                              {"global_name", json::string(global_name)},
                              {"hash_match_group", json::number(group.index)},
                              {"hash_match_group_bit", json::number(group_bit)}});
            }
        }
        if (!hash_bit_added) hash_bits.push_back(std::move(hash_bit));
    }
}

void MatchTable::add_hash_functions(json::map &stage_tbl) const {
    json::vector &hash_functions = stage_tbl["hash_functions"] = json::vector();
    // TODO: Hash functions are not generated for ALPM atcams as the
    // partition index bits used in hash which is a compiler generated field and
    // should not be in 'match_key_fields'. The tests in p4factory are written
    // with match_spec to not include the partition index field. Glass also
    // generates an empty 'hash_functions' node
    if (is_alpm()) return;
    // Emit hash info only if p4_param_order (match_key_fields) are present
    // FIXME: This input_xbar is populated if its a part of the hash_action
    // table or the hash_distribution which is incorrect. This should move
    // inside the hash_dist so this condition does not occur in the
    // hash_action table
    bitvec hash_matrix_req;
    hash_matrix_req.setrange(0, EXACT_HASH_GROUP_SIZE);
    if (!p4_params_list.empty() && !input_xbar.empty()) {
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        auto ht = input_xbar[0]->get_hash_tables();
        if (ht.size() > 0) {
            // Merge all bits to xor across multiple hash ways in single
            // json::vector for each hash bit
            for (const auto hash_table : ht) {
                json::map hash_function;
                json::vector &hash_bits = hash_function["hash_bits"] = json::vector();
                hash_function["hash_function_number"] = hash_table.first.uid();
                gen_hash_bits(hash_table.second, hash_table.first, hash_bits,
                              hash_table.first.uid(), hash_matrix_req);
                hash_functions.push_back(std::move(hash_function));
            }
        }
    }
}

void MatchTable::add_all_reference_tables(json::map &tbl, Table *match_table) const {
    auto mt = (!match_table) ? this : match_table;
    json::vector &action_data_table_refs = tbl["action_data_table_refs"];
    json::vector &selection_table_refs = tbl["selection_table_refs"];
    json::vector &meter_table_refs = tbl["meter_table_refs"];
    json::vector &statistics_table_refs = tbl["statistics_table_refs"];
    json::vector &stateful_table_refs = tbl["stateful_table_refs"];
    add_reference_table(action_data_table_refs, mt->action);
    if (auto a = mt->get_attached()) {
        if (a->selector) {
            unsigned sel_mask = (1U << METER_TYPE_START_BIT) - 1;
            sel_mask &= ~((1U << SELECTOR_LOWER_HUFFMAN_BITS) - 1);
            add_reference_table(selection_table_refs, a->selector);
        }
        for (auto &m : a->meters) {
            add_reference_table(meter_table_refs, m);
        }
        for (auto &s : a->stats) {
            add_reference_table(statistics_table_refs, s);
        }
        for (auto &s : a->statefuls) {
            add_reference_table(stateful_table_refs, s);
        }
    }
}
