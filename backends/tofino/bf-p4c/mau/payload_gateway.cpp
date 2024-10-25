/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/mau/payload_gateway.h"

#include <boost/range/combine.hpp>

#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/phv/phv_fields.h"

/**
 * A gateway in Tofino2 and future generations can be used to implement a small match table.
 * For some context, a gateway is a 5 entry TCAM that has been used to implement conditionals.
 * Examine the following example:
 *
 * if (condition) {
 *     x.apply();
 * } else {
 *     y.apply();
 * }
 * z.apply();
 *
 * The gateway as a logical table can set the next table based on the evaluation of the condition.
 * The gateway rows can be thought of as:
 *
 *  ____match____|__next_table__
 *    condition  |      x
 *      miss     |      y
 *
 * A gateway can be linked with a match table as a single logical table.  The gateway will verify
 * its condition, and if the condition is false, can override the match table.  This process
 * is known as inhibiting.  When the gateway inhibits, the next table comes from the gateway.
 * If the gateway does not inhibit, the next table comes from the match table. Let's say, for
 * example, that table x and the condition were merged into a single logical table.  The gateway
 * rows can be thought of as:
 *
 *  ____match____|__inhibit__|__next_table__
 *    condition  |   false   |    N/A (comes from x's execution, in this case z)
 *      miss     |   true    |     y
 *
 * Now, what actually happens on an inhibit.  In match central, there are two pathways for table
 * execution, a hit pathway and a miss pathway.  When the gateway inhibits, the gateway will
 * automatically force the logical table to enter the hit pathway (even if the underlying
 * match_table missed).  The 83 bit result bus, the input to the hit pathway, is overwritten
 * by the gateway with a 83 bit value.  This value is called the payload.
 *
 * For the standard example, the gateway payload is an all 0 value.  Match central is then
 * programmed to interpret the all 0 payload as a noop.  Let's extend the table
 *
 *  ____match____|__inhibit__|__next_table__|__payload__
 *    condition  |   false   |     N/A      |    N/A
 *      miss     |   true    |      y       |    0x0
 *
 * Now, Tofino has corner cases that require the payload to be a non-zero value.  Say I have
 * the following P4 code:
 *
 *     field = register_action.execute(hash) // hash based index
 *     b.apply();
 *
 *
 * In this program, if the condition is true, then an action that runs a register action
 * addressed by hash is run.  Now this action will be translated in our midend to a single
 * table:
 *
 * action compiler_generated_action() {
 *     field = register_action.execute(hash);
 * }
 *
 * table compiler_generated_table {
 *     actions = { compiler_generated_action; }
 *     default_action = compiler_generated_action;
 * }
 *
 * This is a keyless table, and thus will always miss.  Thus if this table ever runs, the miss
 * action will have to set up this register action's execution.  The major problem is that any
 * information from hash is only available on the hit pathway.  Hash Distribution is only
 * accessible on hit.  This is a conflict, as a table that can only run the miss pathway must
 * access something that is only accessible on hit.
 *
 * The hardware workaround is to use a gateway.  The gateway can be used to generate a "hit",
 * which then is used to access the hardware pathway.  This is basically the gateway running
 * an action.  The table in this case would be:
 *
 *  ____match____|__inhibit__|__next_table__|__payload__
 *      true     |    yes    |      b       |    0x0
 *
 * In this case, the table is executed by the gateway inhibiting.  Inhibit in this case runs
 * a table.  The gateway must always match, as we unconditionally want to run the table.  The
 * all 0 payload is then intepreted by match central to execute the compiler_generated_action,
 * rather than run a noop on the previous example.
 *
 * Now let's extend the example, to further understanding:
 *
 * if (condition) {
 *     field = register_action.execute(hash) // hash based index
 *     b.apply();
 * } else {
 *     c.apply();
 * }
 *
 * The gateway rows become:
 *
 *  ____match____|__inhibit__|__next_table__|__payload__
 *    condition  |    yes    |      b       |    0x3
 *      miss     |    no     |     N/A      |    N/A
 *
 * gateway format : { action(0) : 0..0, meter_pfe(0) : 1..1 }
 *
 * Now, when the condition is true, we want the register action to run.  We run by inhibiting the
 * table.  When the condition is false, we want the register action to not run.  The table will be
 * run, and then the miss pathway will set the payload.  The gateway payload is not necessary for
 * this case, as on the miss the hit pathway will not run, and the input to the table will never be
 * all 0.  However, in order to match what is done for all other gateway examples (i.e. linking a
 * table with a gateway preserves handling the all 0 input being a noop), the payload value was
 * added.
 *
 * This feature has now been extended in Tofino2.  The major difference between Tofino and Tofino2
 * is that in Tofino, there is only a single entry allowed in the gateway format.  In Tofino2, up
 * to 5 entries are allowed in the format.  This allows for separate behavior per entry.
 *
 * Say for instance, I had the following program:
 *
 * if (field == 3)
 *     field1 = register_action1.execute(hash);  -> action compiler_generated_act1;
 * else if (field == 7)
 *     field2 = register_action2.execute(hash);  -> action compiler_generated_act2;
 * b.apply();
 *
 * where each of the register actions correspond to different stateful instructions on the same
 * stateful ALU.  This program requires two gateways in Tofino, where it could be done in a single
 * gateway in Tofino2.  Each stateful ALU execution requires a separate meter_type.  Thus if I
 * wanted to do this in a gateway:
 *
 *
 *  ____match____|__inhibit__|__next_table__|__payload__
 *    field == 3 |    yes    |      b       |    payload1
 *    field == 7 |    yes    |      b       |    payload2
 *      miss     |    no     |     N/A      |    N/A
 *
 * gateway_format = { action(0) : 0..1, meter_pfe(0) : 2..2, meter_type(0) : 3..5,
 *                     action(1) : 6..7, meter_pfe(1) : 8..8, meter_type(1) : 9..11 }
 *
 * Because the action that would be run (setting field1 or field2 would be different), as well as
 * the register action's stateful instruction differing, both of the entries require a different
 * payload.  However, on Tofino, only one entry is allowed for the gateway payload
 *
 * On Tofino2, however, multiple entries are allowed in the gateway format.  This is because the
 * gateway has a separate shift per entry.  The register .*_exact_shiftcount has up to 5 entries
 * per exact match table.  A gateway has 5 entries, so in Tofino2, the exact_shiftcount was also
 * made accessible to the gateway.  Thus now a gateway can support different execution per
 * gateway match.
 *
 * The purpose of this pass is to find tables that can be converted to this type of gateway
 * and convert them if table placement wishes to do so.
 */
void FindPayloadCandidates::add_option(const IR::MAU::Table *tbl, LayoutChoices &lc) {
    if (tbl->getAnnotation("no_gateway_conversion"_cs)) return;

    CollectMatchFieldsAsGateway collect(phv);
    tbl->apply(collect);
    if (!collect || !collect.compute_offsets()) return;

    auto action_formats = lc.get_action_formats(tbl);
    auto last_af = action_formats.back();
    // Currently we don't allow Action Data in a table, though in theory this could
    // be allowed in the future.  We'd have to program a RAM line, which is much more difficult
    // in the assembler than programming a register
    if (last_af.bytes_per_loc.at(ActionData::ACTION_DATA_TABLE) > 0) return;

    auto table_layouts = lc.get_layout_options(tbl);
    const LayoutOption *single_lo = nullptr;
    for (auto &lo : table_layouts) {
        if (lo.action_format_index != static_cast<int>(action_formats.size()) - 1) continue;
        if (lo.way.match_groups != 1 && !lo.layout.ternary) continue;
        single_lo = &lo;
        break;
    }

    if (single_lo == nullptr) return;
    // Guarantee that we fit all of the overhead
    if (single_lo->layout.overhead_bits * (tbl->entries_list->entries.size()) >
        TableFormat::OVERHEAD_BITS)
        return;

    candidates.insert(tbl->name);
    lc.add_payload_gw_layout(tbl, *single_lo);
}

IR::MAU::Table *FindPayloadCandidates::convert_to_gateway(const IR::MAU::Table *tbl) {
    if (candidates.count(tbl->name) == 0) return nullptr;

    // see FindPayloadCandidates::add_option()
    // for the check that ensures the conversion doesn't happen
    BUG_CHECK(!tbl->match_table->getAnnotation("no_gateway_conversion"_cs),
              "attempt to perform a disabled optimisation");

    IR::MAU::Table *gw_tbl = new IR::MAU::Table(*tbl);

    // We iterate over the list of entries.
    int entry_index = 0;
    for (auto &entry : tbl->entries_list->entries) {
        // for each entry line we create a single gw_expr, combining the key values.
        IR::Expression *gw_expr = nullptr;

        // Iterate over tbl->match_key & entry->keys->components together.
        BUG_CHECK(entry->keys->size() == tbl->match_key.size(),
                  "entry keys size != match_key size");
        for (auto tup : boost::combine(tbl->match_key, entry->keys->components)) {
            const IR::MAU::TableKey *tbl_key;
            const IR::Expression *value;
            boost::tie(tbl_key, value) = tup;  // Oh for C++17 structured binding.
            auto key = tbl_key->expr;

            if (value->is<IR::DefaultExpression>()) continue;  // Always true.
            if (auto mask = value->to<IR::Mask>()) {
                // Move the masking onto the key itself.
                key = new IR::BAnd(key, mask->right);
                value = mask->left;
            }
            // Combine into a single `key1==value1 && key2==value2` expression.
            auto equ = new IR::Equ(key, value);
            if (gw_expr == nullptr)
                gw_expr = equ;
            else
                gw_expr = new IR::LAnd(equ, gw_expr);
        }

        cstring entry_name = "$entry"_cs + std::to_string(entry_index++);
        gw_tbl->gateway_rows.push_back(std::make_pair(gw_expr, entry_name));
        cstring action_name;

        std::pair<cstring, std::vector<const IR::Constant *>> payload_row;
        payload_row.second = convert_entry_to_payload_args(tbl, entry, &action_name);
        payload_row.first = action_name;
        gw_tbl->gateway_payload.emplace(entry_name, payload_row);
    }
    // No match key, all on the gateway rows
    gw_tbl->gateway_constant_entries_key = std::move(gw_tbl->match_key);
    gw_tbl->layout.gateway = gw_tbl->layout.gateway_match = true;

    // This is the miss entry.  On miss, similar to a hash action table, we want to
    // run_table, and let the miss_entry of the table set up the next table.
    if (gw_tbl->gateway_rows.back().first != nullptr)
        gw_tbl->gateway_rows.push_back(std::make_pair(nullptr, cstring()));

    /**
     * A table may originally have next table propagation information in the next table map
     * that now has to be updated for this program.
     *
     * Essentially each gateway row (in order to have an entry in to the gateway_payload map),
     * will have a unique tag: $entry#.  The miss entry will currently have a $miss entry.
     *
     * If the table is a hit or miss, then all entries that run the gateway payload will tie
     * to the hit pathway, while the entry for the miss pathway goes through $miss.  With an
     * action_chain, because the actions of each entry is known ahead of time, each gateway
     * entry can reflect that in the code.  If no control flow from this table exists, just
     * create empty entries.
     */
    gw_tbl->next.clear();
    if (tbl->hit_miss_p4()) {
        const IR::MAU::TableSeq *hit_seq =
            tbl->next.count("$hit"_cs) == 0 ? new IR::MAU::TableSeq() : tbl->next.at("$hit"_cs);
        const IR::MAU::TableSeq *miss_seq =
            tbl->next.count("$miss"_cs) == 0 ? new IR::MAU::TableSeq() : tbl->next.at("$miss"_cs);
        for (auto entry : gw_tbl->gateway_rows) {
            if (entry.second.isNull()) continue;
            gw_tbl->next[entry.second] = hit_seq;
        }
        gw_tbl->next["$miss"_cs] = miss_seq;
    } else if (tbl->action_chain() || tbl->has_default_path()) {
        for (auto &entry : gw_tbl->gateway_payload) {
            const IR::MAU::TableSeq *act_seq = nullptr;
            auto act_name = entry.second.first;
            if (tbl->next.count(act_name))
                act_seq = tbl->next.at(act_name);
            else if (tbl->next.count("$default"_cs))
                act_seq = tbl->next.at("$default"_cs);
            else
                act_seq = new IR::MAU::TableSeq();
            gw_tbl->next[entry.first] = act_seq;
        }

        for (auto &act : tbl->actions) {
            if (!act.second->init_default) continue;
            const IR::MAU::TableSeq *act_seq = nullptr;
            auto act_name = act.first;
            if (tbl->next.count(act_name))
                act_seq = tbl->next.at(act_name);
            else if (tbl->next.count("$default"_cs))
                act_seq = tbl->next.at("$default"_cs);
            else
                act_seq = new IR::MAU::TableSeq();
            gw_tbl->next["$miss"_cs] = act_seq;
        }
    } else {
        auto next_seq = new IR::MAU::TableSeq();
        for (auto entry : gw_tbl->gateway_payload) {
            gw_tbl->next[entry.first] = next_seq;
        }
        gw_tbl->next["$miss"_cs] = next_seq;
    }
    // No static entries anymore, these are encoded into the gateway
    gw_tbl->entries_list = nullptr;
    return gw_tbl;
}

/**
 * Given a static entry, which has an action and parameters, this converts these parameters
 * to a list of IR::Constants, which will either be saved on the gateway_row or used in
 * an entry.
 */
FindPayloadCandidates::PayloadArguments FindPayloadCandidates::convert_entry_to_payload_args(
    const IR::MAU::Table *tbl, const IR::Entry *entry, cstring *act_name) {
    std::vector<const IR::Constant *> rv;
    auto mc = entry->action->to<IR::MethodCallExpression>();
    auto pe = mc->method->to<IR::PathExpression>();
    if (act_name) {
        cstring action_name = pe->path->name;
        for (auto act_it : tbl->actions) {
            if (act_it.second->name != action_name) continue;
            *act_name = act_it.first;
            break;
        }
    }

    for (size_t i = 0; i < mc->arguments->size(); i++) {
        auto expr = mc->arguments->at(i)->expression;
        if (auto constant = expr->to<IR::Constant>()) {
            rv.push_back(constant);
        } else if (auto bool_lit = expr->to<IR::BoolLiteral>()) {
            if (bool_lit->value == true)
                rv.push_back(new IR::Constant(IR::Type::Bits::get(1), 1));
            else
                rv.push_back(new IR::Constant(IR::Type::Bits::get(1), 0));
        } else {
            BUG("Unknown expression in a static list");
        }
    }
    return rv;
}

/**
 * Determines the bits to write as the bits for the instruction address.  This comes from
 * the VLIW Instruction allocation, which must be done before this value is calculated
 */
bitvec FindPayloadCandidates::determine_instr_address_payload(const IR::MAU::Action *act,
                                                              const TableResourceAlloc *alloc) {
    auto &instr_mem = alloc->instr_mem;
    auto vliw_instr = instr_mem.all_instrs.at(act->name);
    if (vliw_instr.mem_code < 0) BUG("A gateway cannot have a miss action");
    return bitvec(vliw_instr.mem_code);
}

/**
 * Determines the value of the immediate data based on the PayloadArguments
 */
bitvec FindPayloadCandidates::determine_immediate_payload(const IR::MAU::Action *act,
                                                          PayloadArguments &payload_args,
                                                          const TableResourceAlloc *alloc) {
    auto &af = alloc->action_format;
    auto &alu_positions = af.alu_positions.at(act->name);
    if (act->args.size() != payload_args.size())
        warning("using gateway payload for %s, which driver can't currently reprogram", act);
    unsigned param_index = 0;
    bitvec payload;
    for (auto param : act->args) {
        bitvec param_value;
        ActionData::Argument ad_arg(param->name, {0, param->type->width_bits() - 1});
        if (param_index < payload_args.size()) {
            auto constant = payload_args.at(param_index);
            param_value = bitvec(constant->asUnsigned());
        }

        for (auto &alu_pos : alu_positions) {
            BUG_CHECK(alu_pos.loc == ActionData::IMMEDIATE,
                      "Only can have immediate values "
                      "on gateway payloads currently");
            auto alu_op = alu_pos.alu_op;
            bitvec param_payload = alu_op->static_entry_of_arg(&ad_arg, param_value);
            payload |= param_payload << (alu_pos.start_byte * 8);
        }
        param_index++;
    }

    for (auto &alu_pos : alu_positions) {
        auto alu_op = alu_pos.alu_op;
        bitvec constant_payload = alu_op->static_entry_of_constants();
        payload |= constant_payload << (alu_pos.start_byte * 8);
    }
    return payload;
}

/**
 * The indirect address is similar to the immediate in that the value may come from the
 * payload arguments.
 */
bitvec FindPayloadCandidates::determine_indirect_addr_payload(const IR::MAU::Action *act,
                                                              PayloadArguments &payload_args,
                                                              const IR::MAU::AttachedMemory *at) {
    const IR::MAU::StatefulCall *call = nullptr;
    for (auto sc : act->stateful_calls) {
        if (sc->attached_callee->clone_id != at->clone_id) continue;
        call = sc;
        break;
    }
    bitvec rv;

    if (call == nullptr) {
        BUG_CHECK(act->per_flow_enables.count(at->unique_id()) == 0,
                  "Attached memory enabled "
                  "with no call?");
        return rv;
    }

    // No index implies call is direct
    if (call->index == nullptr) return rv;

    if (auto aa = call->index->to<IR::MAU::ActionArg>()) {
        BUG_CHECK(act->args.size() == payload_args.size(),
                  "Cannot have an action argument "
                  "without a payload argument");
        int param_index = 0;
        for (auto param : act->args) {
            if (param->equiv(*aa)) break;
            param_index++;
        }
        auto expr = payload_args.at(param_index);
        if (auto c = expr->to<IR::Constant>()) {
            rv = bitvec(c->asUnsigned());
        } else if (auto b = expr->to<IR::BoolLiteral>()) {
            if (b->value) rv.setbit(0);
        } else {
            BUG("Unhandled type in the static entry parameter list");
        }
    } else if (auto c = call->index->to<IR::Constant>()) {
        rv = bitvec(c->asUnsigned());
    }
    return rv;
}

/**
 * Returns a bool if that attached memory runs in that action
 */
bitvec FindPayloadCandidates::determine_indirect_pfe_payload(const IR::MAU::Action *act,
                                                             const IR::MAU::AttachedMemory *at) {
    bitvec rv;
    if (act->per_flow_enables.count(at->unique_id())) rv.setbit(0);
    return rv;
}

/**
 * The meter type value is returned from the meter type of that action
 */
bitvec FindPayloadCandidates::determine_meter_type_payload(const IR::MAU::Action *act,
                                                           const IR::MAU::AttachedMemory *at) {
    if (act->meter_types.count(at->unique_id()) == 0) {
        BUG_CHECK(act->per_flow_enables.count(at->unique_id()) == 0,
                  "Per flow enable "
                  "on but missing meter type");
        return bitvec();
    }
    BUG_CHECK(act->per_flow_enables.count(at->unique_id()),
              "Per flow enable "
              "off but has a meter type");
    return bitvec(static_cast<int>(act->meter_types.at(at->unique_id())));
}

/**
 * Determines the payload value of a single entry in the table_format.  Given an entry with
 * a particular action, and set of parameters from a static entry for that action, this
 * will return the payload value for that entry.
 */
bitvec FindPayloadCandidates::determine_match_group_payload(
    const IR::MAU::Table *tbl, const TableResourceAlloc *alloc, const IR::MAU::Action *act,
    std::vector<const IR::Constant *> arguments, int entry_idx) {
    bitvec rv;
    const IR::MAU::AttachedMemory *stats_alu_user = nullptr;
    const IR::MAU::AttachedMemory *meter_alu_user = nullptr;
    for (auto attached : tbl->attached) {
        if (attached->attached->to<IR::MAU::Counter>()) stats_alu_user = attached->attached;
        if (attached->attached->to<IR::MAU::Meter>() ||
            attached->attached->to<IR::MAU::StatefulAlu>())
            meter_alu_user = attached->attached;
    }

    auto &tf = alloc->table_format;
    auto match_group = tf.match_groups.at(entry_idx);
    for (int i = TableFormat::NEXT; i < TableFormat::ENTRY_TYPES; i++) {
        bitvec current_value;
        if (match_group.mask[i].empty()) continue;
        switch (i) {
            case TableFormat::ACTION:
                current_value = determine_instr_address_payload(act, alloc);
                break;
            case TableFormat::IMMEDIATE:
                current_value = determine_immediate_payload(act, arguments, alloc);
                break;
            case TableFormat::COUNTER:
                BUG_CHECK(stats_alu_user != nullptr,
                          "Must have a counter to have a counter "
                          "address");
                current_value = determine_indirect_addr_payload(act, arguments, stats_alu_user);
                break;
            case TableFormat::COUNTER_PFE:
                BUG_CHECK(stats_alu_user != nullptr,
                          "Must have a counter to have a counter "
                          "address");
                current_value = determine_indirect_pfe_payload(act, stats_alu_user);
                break;
            case TableFormat::METER:
                BUG_CHECK(meter_alu_user != nullptr,
                          "Must have a meter to have a meter "
                          "address");
                current_value = determine_indirect_addr_payload(act, arguments, meter_alu_user);
                break;
            case TableFormat::METER_PFE:
                BUG_CHECK(meter_alu_user != nullptr,
                          "Must have a meter to have a meter "
                          "address");
                current_value = determine_indirect_pfe_payload(act, meter_alu_user);
                break;
            case TableFormat::METER_TYPE:
                BUG_CHECK(meter_alu_user != nullptr,
                          "Must have a meter to have a meter "
                          "address");
                current_value = determine_meter_type_payload(act, meter_alu_user);
                break;
            case TableFormat::VALID:
                current_value = bitvec(1);
                break;
            default:
                BUG("No way that this can be a gateway payload");
        }
        rv |= current_value << match_group.mask[i].min().index();
    }
    return rv;
}

/**
 * This is to determine the value that goes onto the payload_value of a gateway.  Can be used
 * for both Tofino and Tofino2 gateways, both pre and post table placement.  This is currently
 * done in the memories allocation, as this is when we write the Memories::Use object.
 *
 * Will allow gateway variables such as:
 *
 *    - action
 *    - immediate
 *    - stats_addr
 *    - stats_pfe
 *    - meter_addr
 *    - meter_pfe
 *    - meter_type
 */
bitvec FindPayloadCandidates::determine_payload(const IR::MAU::Table *tbl,
                                                const TableResourceAlloc *alloc,
                                                const IR::MAU::Table::Layout *layout) {
    bitvec payload;
    auto &tf = alloc->table_format;
    if (tbl->entries_list && tbl->entries_list->entries.size() == tf.match_groups.size()) {
        int entry_idx = 0;
        for (auto entry : tbl->entries_list->entries) {
            cstring act_name;
            auto payload_args = convert_entry_to_payload_args(tbl, entry, &act_name);
            auto act = tbl->actions.at(act_name);
            payload |= determine_match_group_payload(tbl, alloc, act, payload_args, entry_idx);
            entry_idx++;
        }
        return payload;
    } else if (!tbl->gateway_payload.empty() &&
               tbl->gateway_payload.size() == tf.match_groups.size()) {
        int entry_idx = 0;
        for (auto entry : tbl->gateway_rows) {
            if (!entry.second) continue;
            auto payload_row = tbl->gateway_payload.at(entry.second);
            payload |= determine_match_group_payload(tbl, alloc, tbl->actions.at(payload_row.first),
                                                     payload_row.second, entry_idx);
            entry_idx++;
        }
        return payload;
    } else if (layout->hash_action) {
        BUG_CHECK(tf.match_groups.size() == 1 && tbl->hit_actions() == 1,
                  "Not a no match hit "
                  "table");
        std::vector<const IR::Constant *> payload_args;
        const IR::MAU::Action *hit_act = nullptr;
        for (auto act : Values(tbl->actions)) {
            if (act->miss_only()) continue;
            hit_act = act;
            break;
        }
        return determine_match_group_payload(tbl, alloc, hit_act, payload_args, 0);
    } else if (tbl->entries_list) {
        BUG_CHECK(tbl->entries_list->entries.size() == tf.match_groups.size(),
                  "Not every payload "
                  "is accounted for");
    } else if (!tbl->gateway_payload.empty()) {
        BUG_CHECK(tbl->gateway_payload.size() == tf.match_groups.size(),
                  "Not every payload is "
                  "accounted for");
    }
    BUG("Unreachable");
    return bitvec();
}
