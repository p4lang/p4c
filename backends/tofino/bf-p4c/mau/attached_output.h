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

#ifndef BF_P4C_MAU_ATTACHED_OUTPUT_H_
#define BF_P4C_MAU_ATTACHED_OUTPUT_H_

#include "bf-p4c/mau/action_format.h"

class LayoutChoices;

namespace MeterALU {

using namespace P4;
using OperationsPerAction = std::map<cstring, safe_vector<ActionData::ALUOperation *>>;
using OperationsPerTable = ordered_map<const IR::MAU::Table *, OperationsPerAction>;
using AttachedToTableMap =
    ordered_map<const IR::MAU::AttachedMemory *, ordered_set<const IR::MAU::Table *>>;

class Format : public MauInspector {
    const PhvInfo &phv;
    AttachedToTableMap attached_to_table_map;
    OperationsPerTable operations_per_table;
    LayoutChoices &layout_choices;

 public:
    // Per table
    struct Use {
        ///> ALU Operations to be use in InstructionAdjustment
        std::map<cstring, safe_vector<ActionData::ALUPosition>> alu_positions;
        ///> ADB inputs on home row from this table's operations`
        ActionData::BusInputs table_bus_inputs = {{bitvec(), bitvec(), bitvec()}};
        ///> ADB inputs on home row for all tables that share this associated attached memory
        ActionData::BusInputs meter_alu_bus_inputs = {{bitvec(), bitvec(), bitvec()}};

        safe_vector<const ActionData::ALUPosition *> all_alu_positions() const {
            safe_vector<const ActionData::ALUPosition *> rv;
            for (auto &act : alu_positions)
                for (auto &pos : act.second) rv.push_back(&pos);
            return rv;
        }
        void clear() {
            alu_positions.clear();
            table_bus_inputs = {{bitvec(), bitvec(), bitvec()}};
            meter_alu_bus_inputs = {{bitvec(), bitvec(), bitvec()}};
        }

        bool contains_adb_slot(ActionData::SlotType_t type, int start_byte) const;
    };

 private:
    profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        operations_per_table.clear();
        attached_to_table_map.clear();
        return rv;
    }

    void create_meter_alu(ActionData::ALUOperation &alu, ActionAnalysis::ActionParam &read,
                          le_bitrange container_bits);
    void create_alu_ops_for_action(ActionAnalysis::ContainerActionsMap &ca_map, cstring action_name,
                                   OperationsPerAction &ops_per_action);
    bool preorder(const IR::MAU::Table *tbl) override;
    void build_use(OperationsPerAction &ops_per_action, Use *use);
    void end_apply() override;

 public:
    Format(const PhvInfo &p, LayoutChoices &l) : phv(p), layout_choices(l) {}
};

}  // namespace MeterALU

#endif /* BF_P4C_MAU_ATTACHED_OUTPUT_H_ */
