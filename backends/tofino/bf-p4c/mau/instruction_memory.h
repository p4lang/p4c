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

#ifndef BF_P4C_MAU_INSTRUCTION_MEMORY_H_
#define BF_P4C_MAU_INSTRUCTION_MEMORY_H_

/* clang-format off */
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/common/alloc.h"

using namespace P4;

class GenerateVLIWInstructions {
    PhvInfo &phv;
    ActionData::FormatType_t format_type;
    SplitAttachedInfo &split_attached;
    const IR::MAU::Table* tbl;

    void generate_for_action(const IR::MAU::Action *);
    ordered_map<const IR::MAU::Action *, bitvec> table_instrs;
    ordered_map<const IR::MAU::Action *, bool> table_instrs_has_unalloc_tempvar;

 public:
    bitvec get_instr(const IR::MAU::Action *act) {
        return table_instrs[act];
    }
    bool instr_has_unalloc_tempvar(const IR::MAU::Action *act) {
        return table_instrs_has_unalloc_tempvar[act];
    }
    GenerateVLIWInstructions(PhvInfo &p, ActionData::FormatType_t ft,
            SplitAttachedInfo &sai, const IR::MAU::Table* tbl);
};

/** Algorithms for the allocation of the Instruction Memory.  The Instruction Memory is defined
 *  in the uArch section 6.1.10.3 Action Instruction Memory.
 *
 *  The instruction memory is a 32 row x (PHV ALUs) per gress memory.  Each action in
 *  P4 corresponds to a single RAM row in the IMEM.  Each slot of a row corresponds to the
 *  instruction for that particular ALU.  A noop instruction for an ALU is an all 0 encoded
 *  instruction.  Thus an action in the P4 code coordinates to a single line in the
 *  instruction memory.  For all PHVs that a particular action writes, the slots on that row
 *  will have the corresponding action, and for all PHVs that the particular action doesn't
 *  write, a noop, encoded as all 0s will let the PHV pass directly through.  These instructions
 *  are called VLIW instructions, as one RAM line coordinates to hundreds of individual
 *  ALU instructions.
 *
 *  When a stage decides which actions to run, the RAM lines containing these actions are
 *  all ORed together to make a single instruction to be run by the ALUs.  This is the root
 *  of action dependency.  If two actions in the same packet operate on the same ALU,
 *  then their corresponding instruction opcodes will be ORed together, causing the instruction
 *  to be garbled and incorrect.  (N.B.  There is a possibility of removing action dependencies,
 *  if it is known that this ORing won't have any semantic change to the instruction)
 *
 *  Each instruction memory line has two colors.  Two actions can be stored per RAM line as
 *  the actions on that RAM line can have a different color.  This will only work if the
 *  intersection of the PHV writes for these actions are an empty set, as each individual
 *  ALU instruction can be marked a certain color.
 *
 *  Furthermore, actions can be shared across multiple tables, as long as the action opcodes
 *  are identical.  The main corner case for this is a noop action, which usually appears
 *  in several tables.  In both gresses, currently, the algorithm always reserves the first
 *  imem slot with the 0 color to be noop.  Gateways that inhibit can potentially run an
 *  action as well, if the pfe for the imem is always defaulted on.  Thus when the payload
 *  is all 0s, the instruction just runs a noop
 */
struct InstructionMemory {
    static constexpr int NOOP_ROW = 0;
    static constexpr int NOOP_COLOR = 0;
    const IMemSpec &spec;

 protected:
    explicit InstructionMemory(const IMemSpec &s) : spec(s) {}

    InstructionMemory(const InstructionMemory &) = delete;
    InstructionMemory &operator=(const InstructionMemory &) = delete;

 public:
    virtual ~InstructionMemory() = default;

    std::set<cstring> atcam_updates;

    /** Instruction Memory requires two things:
     *    1. The RAM line position/color of a word
     *    2. The code for running this instruction that is written on the RAM line.
     *
     *  Each match saves with it an action to run.  Rather than store a full address
     *  per match key, which would be 6 bits apiece, if the total number of possible
     *  hit actions are <= 8, then the address just needs to be a unique code between
     *  0 and ceil_log2(hit_actions).
     *
     *  The row and color correspond to the instruction memory row and color, while
     *  the mem_code is used by the context JSON to know what to write on the SRAM for
     *  running this particular action.
     */
    struct Use {
        struct VLIW_Instruction {
            bitvec non_noop_instructions;
            int row;
            int color;
            int mem_code = -1;

            // The address for the RAM line is in this format
            unsigned gen_addr() const {
                return color | row << Device::imemSpec().color_bits();
            }
            VLIW_Instruction(bitvec nni, int r, int c)
                : non_noop_instructions(nni), row(r), color(c) {}

            bool operator==(const VLIW_Instruction &v) const {
                return (non_noop_instructions == v.non_noop_instructions) && \
                       (row == v.row) && \
                       (color == v.color) && \
                       (mem_code == v.mem_code);
            }

            friend std::ostream & operator<<(std::ostream &out, const VLIW_Instruction &i);
        };
        std::map<cstring, VLIW_Instruction> all_instrs;

        void clear() {
            all_instrs.clear();
        }
        void merge(const Use &alloc);

        bool operator==(const Use &u) const {
            return all_instrs == u.all_instrs;
        }

        bool operator!=(const Use &u) const {
            return all_instrs != u.all_instrs;
        }

        friend std::ostream & operator<<(std::ostream &out, const Use &u);
    };

    std::map<const IR::MAU::ActionData *, const Use *> shared_action_profiles;
        // std::map<cstring, InstructionMemory::Use::VLIW_Instruction>> shared_action_profiles;

    virtual BFN::Alloc2Dbase<cstring> &imem_use(gress_t gress) = 0;
    const BFN::Alloc2Dbase<cstring> &imem_use(gress_t gress) const {
        return const_cast<InstructionMemory *>(this)->imem_use(gress); }
    virtual BFN::Alloc2Dbase<bitvec> &imem_slot_inuse(gress_t gress) = 0;
    const BFN::Alloc2Dbase<bitvec> &imem_slot_inuse(gress_t gress) const {
        return const_cast<InstructionMemory *>(this)->imem_slot_inuse(gress); }

    bool is_noop_slot(int row, int color);
    bool find_row_and_color(bitvec current_bv, gress_t gress, int &row, int &color,
                            bool &first_noop, bool has_unalloc_temp = false);
    bool shared_instr(const IR::MAU::Table *tbl, Use &alloc, bool gw_linked);
    bool alloc_always_run_instr(const IR::MAU::Table *tbl, Use &alloc, bitvec current_bv);

 public:
    bool allocate_imem(const IR::MAU::Table *tbl, Use &alloc, PhvInfo &phv, bool gw_linked,
        ActionData::FormatType_t format_type, SplitAttachedInfo &sai);
    void update_always_run(const Use &alloc, gress_t gress);
    void update(cstring name, const Use &alloc, gress_t gress);
    void update(cstring name, const TableResourceAlloc *alloc, gress_t gress);
    void update(cstring name, const TableResourceAlloc *alloc, const IR::MAU::Table *tbl);
    void update(const IR::MAU::Table *tbl);

    static InstructionMemory *create();
};

/* clang-format on */

#endif /* BF_P4C_MAU_INSTRUCTION_MEMORY_H_ */
