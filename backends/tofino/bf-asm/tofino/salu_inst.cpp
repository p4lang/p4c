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

/* Tofino template specializations for instructions #included in salu_inst.cpp
 * WARNING -- this is included in an anonymous namespace, as these SaluInstruction
 * subclasses are all defined in that anonymous namespace */

template <>
void AluOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_state_alu[act->code][slot - ALU2LO];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    salu.salu_op = opc->opcode & 0xf;
    salu.salu_arith = opc->opcode >> 4;
    salu.salu_pred = predication_encode & Target::Tofino::STATEFUL_PRED_MASK;
    const int alu_const_min = Target::STATEFUL_ALU_CONST_MIN();
    const int alu_const_max = Target::STATEFUL_ALU_CONST_MAX();
    if (srca) {
        if (auto m = srca.to<operand::Memory>()) {
            salu.salu_asrc_memory = 1;
            salu.salu_asrc_memory_index = m->field->bit(0) > 0;
        } else if (auto k = srca.to<operand::Const>()) {
            salu.salu_asrc_memory = 0;
            if (k->value >= alu_const_min && k->value <= alu_const_max) {
                salu.salu_const_src = k->value & Target::STATEFUL_ALU_CONST_MASK();
                salu.salu_regfile_const = 0;
            } else {
                salu.salu_const_src = tbl->get_const(k->lineno, k->value);
                salu.salu_regfile_const = 1;
            }
        } else if (auto r = srca.to<operand::Regfile>()) {
            salu.salu_asrc_memory = 0;
            salu.salu_const_src = r->index;
            salu.salu_regfile_const = 1;
        } else {
            BUG("unknown operand");
        }
    }
    if (srcb) {
        if (auto f = srcb.to<operand::Phv>()) {
            salu.salu_bsrc_phv = 1;
            salu.salu_bsrc_phv_index = f->phv_index(tbl);
        } else if (auto m = srcb.to<operand::MathFn>()) {
            salu_instr_common.salu_alu2_lo_bsrc_math = 1;
            if (auto b = m->of.to<operand::Phv>()) {
                salu_instr_common.salu_alu2_lo_math_src = b->phv_index(tbl);
            } else if (auto b = m->of.to<operand::Memory>()) {
                salu_instr_common.salu_alu2_lo_math_src = b->field->bit(0) > 0 ? 3 : 2;
            } else {
                BUG("unknown operand");
            }
        } else if (auto k = srcb.to<operand::Const>()) {
            salu.salu_bsrc_phv = 0;
            if (k->value >= alu_const_min && k->value <= alu_const_max) {
                salu.salu_const_src = k->value & Target::STATEFUL_ALU_CONST_MASK();
                salu.salu_regfile_const = 0;
            } else {
                salu.salu_const_src = tbl->get_const(k->lineno, k->value);
                salu.salu_regfile_const = 1;
            }
        } else if (auto r = srcb.to<operand::Regfile>()) {
            salu.salu_bsrc_phv = 0;
            salu.salu_const_src = r->index;
            salu.salu_regfile_const = 1;
        } else {
            BUG("unknown operand");
        }
    }
}
void AluOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::Tofino::mau_regs>(regs, tbl, act);
}

template <>
void BitOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    LOG2(this);
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_state_alu[act->code][slot - ALU2LO];
    salu.salu_op = opc->opcode & 0xf;
    salu.salu_pred = predication_encode & Target::Tofino::STATEFUL_PRED_MASK;
    // 1b instructions are from mem-lo to alu1-lo
    salu.salu_asrc_memory = 1;
    salu.salu_asrc_memory_index = 0;
}
void BitOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::Tofino::mau_regs>(regs, tbl, act);
}

template <>
void CmpOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_cmp_alu[act->code][slot];
    if (srca) {
        salu.salu_cmp_asrc_input = srca->field->bit(0) > 0;
        salu.salu_cmp_asrc_sign = srca_neg;
        salu.salu_cmp_asrc_enable = 1;
    }
    if (srcb) {
        salu.salu_cmp_bsrc_input = srcb->phv_index(tbl);
        salu.salu_cmp_bsrc_sign = srcb_neg;
        salu.salu_cmp_bsrc_enable = 1;
    }
    if (srcc) {
        if (auto k = dynamic_cast<const operand::Const *>(srcc)) {
            const int cmp_const_min = Target::STATEFUL_CMP_CONST_MIN();
            const int cmp_const_max = Target::STATEFUL_CMP_CONST_MAX();
            if (k->value >= cmp_const_min && k->value <= cmp_const_max) {
                salu.salu_cmp_const_src = k->value & Target::STATEFUL_CMP_CONST_MASK();
                salu.salu_cmp_regfile_const = 0;
            } else {
                salu.salu_cmp_const_src = tbl->get_const(srcc->lineno, k->value);
                salu.salu_cmp_regfile_const = 1;
            }
        } else if (auto r = dynamic_cast<const operand::Regfile *>(srcc)) {
            salu.salu_cmp_const_src = r->index;
            salu.salu_cmp_regfile_const = 1;
        }
    } else {
        salu.salu_cmp_const_src = 0;
        salu.salu_cmp_regfile_const = 0;
    }
    salu.salu_cmp_opcode = opc->opcode | (type << 2);
}
void CmpOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::Tofino::mau_regs>(regs, tbl, act);
}

void TMatchOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    BUG("Unreachable state.");
}

void OutOP::decode_output_mux(Target::Tofino, Table *tbl, value_t &op) {
    static const std::map<std::string, int> ops_mux_lookup = {
        {"mem_hi", 0},     {"mem_lo", 1},     {"memory_hi", 0}, {"memory_lo", 1},
        {"phv_hi", 2},     {"phv_lo", 3},     {"alu_hi", 4},    {"alu_lo", 5},
        {"alu_hi_out", 4}, {"alu_lo_out", 5}, {"predicate", 6}};
    if (op.type == tCMD && ops_mux_lookup.count(op[0].s))
        output_mux = ops_mux_lookup.at(op[0].s);
    else if (op.type == tSTR && ops_mux_lookup.count(op.s))
        output_mux = ops_mux_lookup.at(op.s);
    else
        output_mux = -1;
    if (src) {
        int tmp = output_mux;
        if (auto *phv = src.to<operand::Phv>())
            output_mux = 3 - phv->phv_index(tbl->to<StatefulTable>());
        else if (auto *mem = src.to<operand::Memory>())
            output_mux = mem->field->bit(0) > 0 ? 0 : 1;
        BUG_CHECK(tmp < 0 || tmp == output_mux, "inconsistent output mux decode");
    }
}
int OutOP::decode_output_option(Target::Tofino, value_t &op) { return -1; }

template <>
void OutOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_output_alu[act->code];
    if (predication_encode) {
        salu.salu_output_cmpfn = predication_encode & Target::Tofino::STATEFUL_PRED_MASK;
    } else {
        salu.salu_output_cmpfn = STATEFUL_PREDICATION_ENCODE_UNCOND;
    }
    salu.salu_output_asrc = output_mux;
}
void OutOP::write_regs(Target::Tofino::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::Tofino::mau_regs>(regs, tbl, act);
}
