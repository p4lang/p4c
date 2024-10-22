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

/* clang-format off */

#include "bf-p4c/mau/instruction_memory.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/device.h"

#include "bf-p4c/mau/tofino/instruction_memory.h"

GenerateVLIWInstructions::GenerateVLIWInstructions(PhvInfo &p, ActionData::FormatType_t ft,
                                                   SplitAttachedInfo &sai,
                                                   const IR::MAU::Table *tbl)
    : phv(p), format_type(ft), split_attached(sai), tbl(tbl) {
    LOG3("GenerateVLIWInstructions for table " << tbl->name);
    for (const auto &act : tbl->actions) {
        generate_for_action(act.second);
    }
}

void GenerateVLIWInstructions::generate_for_action(const IR::MAU::Action *act) {
    LOG3("GenerateVLIWInstructions for action " << act->name);
    bitvec current_vliw;
    bool current_has_unalloc_tempvar = false;
    // Need to capture the instructions that will be created during the splitting of the tables
    auto *act_to_visit = split_attached.get_split_action(act, tbl, format_type);
    if (act_to_visit == nullptr) return;
    BUG_CHECK(act_to_visit, "Somehow have a nullptr action for %1%", format_type);

    LOG4("\tSplit action found " << act_to_visit);
    for (auto *prim : act_to_visit->action) {
        auto instr = prim->to<IR::MAU::Instruction>();
        BUG_CHECK(instr, "Unexpected IR::Primitive, expected IR::MAU::Instruction");
        auto output = instr->getOutput();
        if (output) {
            LOG3("\tGenerateVLIWInstructions for expression " << output);
            le_bitrange bits;
            auto field = phv.field(output, &bits);
            BUG_CHECK(field != nullptr, "Instruction writing to a non-phv allocated object");
            auto isTempVar = phv.isTempVar(field);
            LOG2("\t\tExpression is write for field " << field << " tempvar " << isTempVar
                                                      << " alloc size " << field->alloc_size());
            if (field->alloc_size() == 0 && isTempVar) {
                current_has_unalloc_tempvar = true;
                return;
            }
            // Mark which containers are to have non noop instructions
            PHV::FieldUse use(PHV::FieldUse::WRITE);
            field->foreach_alloc(bits, tbl, &use, [&](const PHV::AllocSlice &alloc) {
                if (!alloc.container()) return;
                current_vliw.setbit(Device::phvSpec().containerToId(alloc.container()));
            });
        }
    }
    table_instrs[act] = current_vliw;
    table_instrs_has_unalloc_tempvar[act] = current_has_unalloc_tempvar;
    LOG4("\t Action vliw bits :" << current_vliw);
}

void InstructionMemory::Use::merge(const Use &alloc) {
    BUG_CHECK(all_instrs.size() == 1 && alloc.all_instrs.size() == 1,
              "Only always run "
        "tables can be merged");
    cstring key = all_instrs.begin()->first;
    VLIW_Instruction a_instr = all_instrs.begin()->second;
    VLIW_Instruction b_instr = alloc.all_instrs.begin()->second;

    BUG_CHECK(static_cast<int>(a_instr.gen_addr()) == Device::alwaysRunIMemAddr() &&
              static_cast<int>(b_instr.gen_addr()) == Device::alwaysRunIMemAddr() &&
                  key == "$always_run",
              "Only always run tables can be merged");

    VLIW_Instruction m_instr(a_instr.non_noop_instructions | b_instr.non_noop_instructions,
                             a_instr.row, a_instr.color);
    all_instrs.emplace(key, m_instr);
    ;
}

std::ostream &operator<<(std::ostream &out, const InstructionMemory::Use &u) {
    out << "Instruction Memory: [" << std::endl;
    for (auto &instr : u.all_instrs) out << instr.first << ": " << instr.second << std::endl;
    out << " ]" << std::endl;
    return out;
}

std::ostream &operator<<(std::ostream &out, const InstructionMemory::Use::VLIW_Instruction &i) {
    out << "non_noop_instructions=" << i.non_noop_instructions;
    out << " row=" << i.row;
    out << " color=" << i.color;
    out << " mem_code=" << i.mem_code;
    return out;
}

bool InstructionMemory::is_noop_slot(int row, int color) {
    return row == NOOP_ROW && color == NOOP_COLOR;
}

bool InstructionMemory::find_row_and_color(bitvec current_bv, gress_t gress, int &row, int &color,
                                           bool &first_noop, bool has_unalloc_temp) {
    LOG3("Finding row and color for " << current_bv << " in gress " << gress << " first noop "
                                      << first_noop << " has unallocated tempvar "
                                      << has_unalloc_temp);
    auto &use = imem_use(gress);
    auto &slot_in_use = imem_slot_inuse(gress);

    // Always reserve one noop on the first line. Subsequent noops will be
    // reserved on different lines to allow driver to associate each with a
    // different action handle during entry reads from hardware.
    if (current_bv.empty() && first_noop) {
        // If an action has an instruction which writes to a temp variable it
        // cannot be a noop. This condition needs to be checked since tempvars
        // do not have a phv allocation until after table placement. Thus we
        // have a current_bv which is empty but will eventually have a container
        // being used to write to.
        if (!has_unalloc_temp) {
            row = NOOP_ROW;
            color = NOOP_COLOR;
            int addr = (row << spec.color_bits()) | color;
            LOG2("\tFixing row " << row << ", color " << color << ", addr " << addr
                                 << " for first noop");
            first_noop = false;
            return true;
        }
    }

    for (int i = 0; i < spec.rows(); i++) {
        bitvec current_row;
        bool occupied = true;
        for (int j = 0; j < spec.colors(); j++) {
            if (is_noop_slot(i, j)) continue;

            if (use[i][j].isNull()) occupied = false;
            current_row |= slot_in_use[i][j];
        }
        if (occupied) continue;

        // Make sure that no color collision exists
        if (!(current_row & current_bv).empty()) continue;

        for (int j = 0; j < spec.colors(); j++) {
            if (is_noop_slot(i, j)) continue;
            if (!use[i][j].isNull()) continue;

            row = i;
            color = j;
            int addr = (row << spec.color_bits()) | color;
            LOG2("\tFixing row " << i << ", color " << j << ", addr " << addr);
            if (Device::hasAlwaysRunInstr() && Device::alwaysRunIMemAddr() == addr) continue;
            return true;
        }
        BUG("Unreachable section of the instruction memory allocation");
    }
    return false;
}

/**
 * If two tables share an action profile, then the instruction memory location can be the
 * exact same across both tables.  However, the mem code for the instructions may need to
 * differ.
 *
 * Say for example, we had two tables {t1, t2} share an action profile that had the same 4
 * actions, {a1, a2, a3, a4 }.  Four actions would require two bits of RAM line per entry.  Let's
 * say however, that one of the tables, with an action profile, i.e t2, is linked with a gateway.
 * A gateway requires an action_instruction_adr_map_data entry as well, as the gateway goes
 * through the hit pathway.
 *
 * Thus in our example, t2 would require 5 instruction entries, and thus 3 bits of indirection,
 * while t1 would only require 2 bits.  Furthermore, if the gateway goes to row 0, then
 * the an action encoding in t2 wouldn't fit within the 2 bits.
 *
 * This function is to share the instruction memory line, while determining the correct
 * memcode per action.
 */
bool InstructionMemory::shared_instr(const IR::MAU::Table *tbl, Use &alloc, bool gw_linked) {
    const Use *cached_use = nullptr;
    const IR::MAU::ActionData *ad = nullptr;
    const IR::MAU::ActionData *ad_use = nullptr;
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        ad = at->to<IR::MAU::ActionData>();
        if (ad == nullptr) continue;
        if (shared_action_profiles.count(ad)) {
            LOG2("Already shared through the action profile");
            cached_use = shared_action_profiles.at(ad);
            ad_use = ad;
        }
    }

    if (ad_use == nullptr || cached_use == nullptr) return false;

    int hit_actions = tbl->hit_actions();
    if (gw_linked) hit_actions += 1;

    bool can_use_hitmap = hit_actions <= TableFormat::IMEM_MAP_TABLE_ENTRIES;
    int hit_action_index = gw_linked ? 1 : 0;
    for (auto action : Values(tbl->actions)) {
        auto instr_pos = cached_use->all_instrs.find(action->name);
        BUG_CHECK(instr_pos != cached_use->all_instrs.end(),
                  "%s: Error when programming action "
                  "%s on shared action profile %s and table %s",
                  ad_use->srcInfo, ad_use->name, tbl->name);

        Use::VLIW_Instruction single_instr = instr_pos->second;
        single_instr.mem_code = -1;
        if (!action->miss_only()) {
            if (can_use_hitmap)
                single_instr.mem_code = hit_action_index;
            else
                single_instr.mem_code = single_instr.gen_addr();
            hit_action_index++;
            LOG2("Action " << action->name << " Mem code " << single_instr.mem_code);
        }
        alloc.all_instrs.emplace(action->name, single_instr);
    }
    return true;
}

bool InstructionMemory::alloc_always_run_instr(const IR::MAU::Table *tbl, Use &alloc,
        bitvec current_bv) {
    auto &use = imem_use(tbl->gress);
    auto &slot_in_use = imem_slot_inuse(tbl->gress);

    // Eventual goal is to run ActionAnalysis on the merged actions with all other
    // AlwaysRun actions (in case two tables interact with the same container).  But
    // at this point, no container conflicts
    int row = Device::alwaysRunIMemAddr() / 2;
    bitvec reserved;
    for (int color = 0; color < 2; color++) reserved |= slot_in_use[row][color];
    if (!(reserved & current_bv).empty()) return false;
    int ar_color = Device::alwaysRunIMemAddr() % 2;
    use[row][ar_color] = "$always_run_action"_cs;
    slot_in_use[row][ar_color] |= current_bv;
    Use::VLIW_Instruction single_instr(current_bv, row, ar_color);
    alloc.all_instrs.emplace("$always_run_action"_cs, single_instr);
    return true;
}

bool InstructionMemory::allocate_imem(const IR::MAU::Table *tbl, Use &alloc, PhvInfo &phv,
                                      bool gw_linked, ActionData::FormatType_t format_type,
                                      SplitAttachedInfo &sai) {
    BUG_CHECK(format_type.valid(), "invalid format type in InstructionMemory::allocate_imem");
    LOG1("Allocating instruction memory for " << tbl->name << " " << format_type);

    // Action Profiles always have the same instructions for every table
    if (shared_instr(tbl, alloc, gw_linked)) return true;
    gw_linked |= !format_type.matchThisStage();

    GenerateVLIWInstructions gen_vliw(phv, format_type, sai, tbl);
    if (tbl->is_always_run_action()) {
        auto act_it = Values(tbl->actions).begin();
        auto current_bv = gen_vliw.get_instr((*act_it));
        return alloc_always_run_instr(tbl, alloc, current_bv);
    }
    auto &use = imem_use(tbl->gress);
    auto &slot_in_use = imem_slot_inuse(tbl->gress);

    // TODO: potentially sharing mem_codes between identical actions
    int hit_actions = tbl->hit_actions();
    if (gw_linked) hit_actions += 1;

    bool can_use_hitmap = hit_actions <= TableFormat::IMEM_MAP_TABLE_ENTRIES;

    int hit_action_index = gw_linked ? 1 : 0;
    bool first_noop = true;
    for (auto action : Values(tbl->actions)) {
        if (sai.get_split_action(action, tbl, format_type) == nullptr) {
            LOG2("  Not generating instruction for "
                 << action->name << " as it is not necessary post attached split");
            continue;
        }

        auto current_bv = gen_vliw.get_instr(action);
        bool action_has_unalloc_tempvar = gen_vliw.instr_has_unalloc_tempvar(action);
        int row = -1;
        int color = -1;
        if (!find_row_and_color(current_bv, tbl->gress, row, color, first_noop,
                                action_has_unalloc_tempvar))
            return false;
        Use::VLIW_Instruction single_instr(current_bv, row, color);
        if (!action->miss_only()) {
            if (can_use_hitmap)
                single_instr.mem_code = hit_action_index;
            else
                single_instr.mem_code = single_instr.gen_addr();
            hit_action_index++;
        }
        alloc.all_instrs.emplace(action->name, single_instr);
        use[row][color] = tbl->name + "$" + action->name.originalName;
        slot_in_use[row][color] = current_bv;
        LOG2("\tAllocated Row " << row << " color " << color << " mem code "
                                << single_instr.mem_code << " for action " << use[row][color]);
    }

    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        auto ad = at->to<IR::MAU::ActionData>();
        if (ad) {
            shared_action_profiles.emplace(ad, &alloc);
            break;
        }
    }

    return true;
}

void InstructionMemory::update(cstring name, const Use &alloc, gress_t gress) {
    auto &use = imem_use(gress);
    auto &slot_in_use = imem_slot_inuse(gress);

    for (auto &entry : alloc.all_instrs) {
        auto a_name = name + "$" + entry.first;
        auto instr = entry.second;
        int row = instr.row;
        int color = instr.color;
        if (is_noop_slot(row, color)) {
            BUG_CHECK(instr.non_noop_instructions.empty(),
                      "Allocating %s, which has ALU "
                      "operations, to the noop slot in the instruction memory",
                      a_name);

            if (!use[row][color].isNull()) use[row][color] = a_name;

        } else {
            if (!use[row][color].isNull() && use[row][color] != a_name) {
                BUG("Instructions %s and %s are assigned to the same slot.", use[row][color],
                    a_name);
            }
            use[row][color] = a_name;
        }

        int opposite_color = color == 1 ? 0 : 1;
        if (!(slot_in_use[row][opposite_color] & instr.non_noop_instructions).empty())
            BUG("Colliding instructions on row %d for action %s and action %s", row,
                use[row][opposite_color], a_name);
        slot_in_use[row][color] = instr.non_noop_instructions;
    }
}

void InstructionMemory::update_always_run(const Use &alloc, gress_t gress) {
    auto &use = imem_use(gress);
    auto &slot_in_use = imem_slot_inuse(gress);

    BUG_CHECK(alloc.all_instrs.size() == 1, "Always run instruction can only have one instruction");
    for (auto &entry : alloc.all_instrs) {
        auto a_name = "$always_run_action"_cs;
        auto instr = entry.second;
        BUG_CHECK(static_cast<int>(instr.gen_addr()) == Device::alwaysRunIMemAddr(),
                  "Always Run Table Misassigned");
        int row = instr.row;
        int color = instr.color;
        use[row][color] = a_name;
        int opposite_color = color == 1 ? 0 : 1;
        if (!(slot_in_use[row][opposite_color] & instr.non_noop_instructions).empty())
            BUG("Colliding instructions on row %d for action %s and action %s", row,
                use[row][opposite_color], a_name);
        // This is an OR
        slot_in_use[row][color] |= instr.non_noop_instructions;
    }
}

void InstructionMemory::update(cstring name, const TableResourceAlloc *alloc, gress_t gress) {
    update(name, alloc->instr_mem, gress);
}

void InstructionMemory::update(cstring name, const TableResourceAlloc *alloc,
        const IR::MAU::Table *tbl) {
    if (tbl->is_always_run_action()) {
        update_always_run(alloc->instr_mem, tbl->gress);
        return;
    }
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        auto ad = at->to<IR::MAU::ActionData>();
        if (ad == nullptr) continue;
        if (shared_action_profiles.count(ad)) return;
        shared_action_profiles.emplace(ad, &alloc->instr_mem);
    }
    update(name, alloc, tbl->gress);
}

void InstructionMemory::update(const IR::MAU::Table *tbl) {
    if (tbl->layout.atcam && tbl->is_placed()) {
        auto orig_name = tbl->name.before(tbl->name.findlast('$'));
        if (atcam_updates.count(orig_name)) return;
        atcam_updates.emplace(orig_name);
    }
    update(tbl->name, tbl->resources, tbl);
}

InstructionMemory *InstructionMemory::create() { return new Tofino::InstructionMemory(); }

/* clang-format on */
