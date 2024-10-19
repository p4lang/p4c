/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/mau/attached_output.h"

#include "bf-p4c/mau/action_format.h"
#include "bf-p4c/mau/table_layout.h"

namespace MeterALU {

/**
 * The purpose of this file is to determine the requirements of any LPF, WRED, or Stateful ALU
 * that is writing some value back to the data plane. Unlike meter color, which is processed
 * through the immediate pathway, these type of output come from the ALU on that particular
 * row and output onto the home row action bus.  Thus, the requirements are similar to that
 * of an action data table.  The following describes their location in the uArch:
 *
 * MAU Figure 6-73: LPF meter block diagram
 *
 * The 32-bit Vnew is passed to the RED logic (when enabled) to generate the 8b drop indicator.  If
 * RED is enabled, then the drop indicator is output to the action data bus.  Otherwise the 32b Vnew
 * (optionally scaled by the LPF action scale field of the meter RAM word) value is sent.
 *
 * Note:
 *
 * The output format is encoded as a bitvec of 4 bit. Each bit represents the use of 1 byte on
 * action data bus. The encoded value is computed from 'read.size()' and 'container_bit'.
 * The 'container_bit' gives the offset into 32 bit block. The 'read.size()' gives the size
 * of used bits in the 32 bit block.
 *
 *
 * MAU Figure 6-74: Stateful ALU block diagram
 *
 * The alu-hi/lo outputs are also sent to the output ALU where they can be passed to the action data
 * bus.  If the output ALU is predicated off, then its output is gated to 0.  The exception to this
 * is when the 4b predication output from cmp-hi/lo is being sent directly to the action data bus.
 *
 * Note:
 *
 * Same as meter alu above, the output format is encoded as a bitvec of 4 bit.
 *
 * MAU Sec 6.2.12.7: ALU output
 *
 * Multiple stateful ALU outputs can be OR’ed together onto the action data bus by configuring the
 * shift values the same across each stateful ALU.
 */

bool Format::Use::contains_adb_slot(ActionData::SlotType_t type, int start_byte) const {
    int bytes = ActionData::slot_type_to_bits(type) / 8;
    bitvec adb_range = table_bus_inputs[type].getslice(start_byte, bytes);
    BUG_CHECK(adb_range.empty() || (adb_range.is_contiguous() && adb_range.popcount() == bytes),
              "Meter ALU action bus was improperly set up");
    return !adb_range.empty();
}

void Format::create_meter_alu(ActionData::ALUOperation &alu, ActionAnalysis::ActionParam &read,
                              le_bitrange container_bits) {
    auto ao = read.unsliced_expr()->to<IR::MAU::AttachedOutput>();
    BUG_CHECK(ao != nullptr, "Cannot create meter alu");
    ActionData::MeterALU *ma = new ActionData::MeterALU(ao->attached->name, read.range());
    ActionData::ALUParameter ap(ma, container_bits);
    alu.add_param(ap);
}

/**
 * Tofino can support at most one meter_alu or stateful alu in each action, because the they share
 * the same address bus to meter ram or stateful ram.
 *
 * Tofino can support at most one meter address user per logical table, i.e. stateful alu,
 * selector, or meter.  Currently, in the compiler each IR::MAU::Table object corresponds to
 * a single logical table (unless split into multiple stages), and cannot yet by itself
 * be split into multiple logical tables with two different addresses that get extracted.
 *
 * This function uses the container information from action analysis to generate the meter_alu
 * output format on action data bus.  Note that the output from meter_alu is usually 32-bit, and
 * PHV allocation determines how many container(s) are needed to operate on the 32-bit output.
 * The PHV allocation may allocate four 8bit container, or two 16bit container, or other
 * configurations.
 *
 * The ops_per_action is a single ALU operation that contains stateful ALU data.
 */
void Format::create_alu_ops_for_action(ActionAnalysis::ContainerActionsMap &ca_map,
                                       cstring action_name, OperationsPerAction &ops_per_action) {
    LOG2("   Creating action data alus for " << action_name);

    for (auto &container_action_info : ca_map) {
        auto container = container_action_info.first;
        auto &cont_action = container_action_info.second;
        ActionData::ALUOPConstraint_t alu_cons = ActionData::DEPOSIT_FIELD;
        if (!cont_action.specialities().getbit(ActionAnalysis::ActionParam::METER_ALU)) continue;

        if (cont_action.convert_instr_to_bitmasked_set)
            BUG("Bitmasked set cannot be used on attached output in %1% : %2%",
                container.toString(), cont_action.to_string());
        if (cont_action.action_data_isolated() || container.is(PHV::Kind::mocha))
            alu_cons = ActionData::ISOLATED;

        auto *alu = new ActionData::ALUOperation(container, alu_cons);
        bool created_action_data = false;

        for (auto &field_action : cont_action.field_actions) {
            le_bitrange bits;
            auto *write_field = phv.field(field_action.write.expr, &bits);
            le_bitrange container_bits;
            int write_count = 0;
            PHV::FieldUse use(PHV::FieldUse::WRITE);
            write_field->foreach_alloc(
                bits, cont_action.table_context, &use, [&](const PHV::AllocSlice &alloc) {
                    write_count++;
                    container_bits = alloc.container_slice();
                    BUG_CHECK(container_bits.lo >= 0, "Invalid negative container bit");
                    if (!alloc.container())
                        LOG1("ERROR: Phv field " << write_field->name << " written in action "
                                                 << action_name << " is not allocated?");
                });
            if (write_count > 1) BUG("Splitting of writes handled incorrectly");

            for (auto &read : field_action.reads) {
                if (read.type == ActionAnalysis::ActionParam::PHV) continue;
                if ((read.type == ActionAnalysis::ActionParam::ACTIONDATA &&
                     read.speciality != ActionAnalysis::ActionParam::METER_ALU) ||
                    read.type == ActionAnalysis::ActionParam::CONSTANT) {
                    BUG("Illegal instruction with both meter alu action data and non meter alu "
                        "data 1% : %2%",
                        container.toString(), cont_action.to_string());
                }
                create_meter_alu(*alu, read, container_bits);
                created_action_data = true;
            }
        }
        if (created_action_data) {
            ops_per_action[action_name].emplace_back(alu);
        }
    }
}

bool Format::preorder(const IR::MAU::Table *tbl) {
    auto &ops_per_action = operations_per_table[tbl];

    ActionAnalysis::ContainerActionsMap container_actions_map;
    for (auto action : Values(tbl->actions)) {
        container_actions_map.clear();
        ActionAnalysis aa(phv, true, false, tbl, layout_choices.red_info);
        aa.set_container_actions_map(&container_actions_map);
        action->apply(aa);
        create_alu_ops_for_action(container_actions_map, action->name.name, ops_per_action);
    }

    bool found_attached_meter_or_stateful_alu = false;
    for (auto ba : tbl->attached) {
        if (!ba->attached->is<IR::MAU::Meter>() && !ba->attached->is<IR::MAU::StatefulAlu>())
            continue;
        attached_to_table_map[ba->attached].insert(tbl);
        found_attached_meter_or_stateful_alu = true;
    }

    if (!found_attached_meter_or_stateful_alu) {
        for (auto action_ops : ops_per_action) {
            BUG_CHECK(action_ops.second.empty(),
                      "Operations on meter alu users in action %1% "
                      "in table %2% with no object attached",
                      action_ops.first, tbl->externalName());
        }
    }
    return true;
}

/**
 * Bit 0 of the meter address user is output to bit 0 of the home row action bus, and
 * corresponds directly to bit 0 of an action data RAM.  Thus each bit of the slice of
 * AttachedOutput corresponds to that bit on the home row action bus, which is then
 * coordinated to the adb slot itself.
 */
void Format::build_use(OperationsPerAction &ops_per_action, Use *use) {
    for (auto &entry : ops_per_action) {
        auto &alu_positions = use->alu_positions[entry.first];
        for (auto *init_alu_op : entry.second) {
            auto parameter_positions = init_alu_op->parameter_positions();
            int byte_offset = -1;
            for (auto param_pos : parameter_positions) {
                auto ma = param_pos.second->to<ActionData::MeterALU>();
                BUG_CHECK(ma != nullptr, "All parameters must be from meter ALUs currently");
                int bit_offset = (ma->range().lo / init_alu_op->size()) * init_alu_op->size();
                byte_offset = bit_offset / 8;
                break;
            }
            BUG_CHECK(byte_offset >= 0, "no parameter from meter ALU");
            auto alu_op = init_alu_op->add_right_shift(init_alu_op->hw_right_shift(), nullptr);
            alu_positions.emplace_back(alu_op, ActionData::METER_ALU, byte_offset);
            use->table_bus_inputs[alu_op->index()].setrange(byte_offset, alu_op->size() / 8);
        }
    }
    for (size_t i = 0; i < use->table_bus_inputs.size(); i++) {
        LOG4("\tADB inputs for table of type " << i << " " << use->table_bus_inputs[i]);
    }
}

/**
 * Each use has two ActionData::BusInputs to track.  As an AttachedMemory can be connected to
 * multiple stateful ALU objects, the StatefulALU will have action data bus requirements from
 * all tables.  However, when printing the action bus outputs, which are printed per table, not
 * per stateful ALU, the action bus inputs need to reflect the table only inputs.
 *
 * The stateful ALU action bus requirements are the OR of all individual table requirements.
 */
void Format::end_apply() {
    for (auto &entry : attached_to_table_map) {
        LOG1("  Building use for attached memory " << entry.first);
        safe_vector<Use *> uses;
        for (auto tbl : entry.second) {
            auto use = &layout_choices.total_meter_output_format[tbl->name];
            LOG3("      Building use for " << tbl->name);
            build_use(operations_per_table[tbl], use);
            uses.push_back(use);
        }
        ActionData::BusInputs total_inputs;
        for (auto use : uses) {
            for (size_t i = 0; i < total_inputs.size(); i++) {
                total_inputs[i] |= use->table_bus_inputs[i];
            }
        }
        for (size_t i = 0; i < total_inputs.size(); i++) {
            LOG2("    ADB inputs for attached memory of type " << i << " " << total_inputs[i]);
        }

        for (auto use : uses) {
            use->meter_alu_bus_inputs = total_inputs;
        }
    }
}

/**
 *
 * Meter ALU output is often saved to a PHV container to be used in next
 * stage. The path to the PHV goes through action data bus and VLIW ALU. A PHV
 * container is 'shared' between meter ALUs if multiple meter ALUs outputs to
 * the same PHV container.
 *
 *  1. Different actions in the same table invoke the same meter ALU, and the
 *  results are written to the same PHV container.
 *
 *  action act_1 { f = meter.execute(1); }
 *  action act_2 { f = meter.execute(2); }
 *  action act_3 { f = meter.execute(3); }
 *  table t { actions = { act_1; act_2; act_3; } }
 *
 *  2. Different actions in different tables invoke the same meter ALU, and the
 *  results are written to the same PHV container.
 *
 *  action act_1 { f = meter.execute(1); }
 *  action act_2 { f = meter.execute(2); }
 *  action act_3 { f = meter.execute(3); }
 *  table t_1 { actions = { act_1; } }
 *  table t_2 { actions = { act_2; } }
 *  table t_3 { actions = { act_3; } }
 *
 *  3. Different actions in different tables invoke different meter ALU and the
 *  results are written to the same PHV container.
 *
 *  action act_1 { f = meter_1.execute(); }
 *  action act_2 { f = meter_2.execute(); }
 *  action act_3 { f = meter_3.execute(); }
 *  table t1 { actions = { act_1; } }
 *  table t2 { actions = { act_2; } }
 *  table t3 { actions = { act_3; } }
 */

}  // namespace MeterALU
