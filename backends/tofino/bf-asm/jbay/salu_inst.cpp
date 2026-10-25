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

/* JBay template specializations for instructions #included in salu_inst.cpp
 * WARNING -- this is included in an anonymous namespace, as these SaluInstruction
 * subclasses are all defined in that anonymous namespace */

struct DivMod : public AluOP {
    struct Decode : public AluOP::Decode {
        Decode(const char *name, target_t targ, int opc) : AluOP::Decode(name, targ, opc) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override {
            auto *rv = new DivMod(this, op[0].lineno);
            if (op.size != 3) error(op[0].lineno, "divmod must have exactly 2 operands");
            if (op.size > 1) rv->srca = operand(tbl, act, op[1]);
            if (op.size > 2) rv->srcb = operand(tbl, act, op[2]);
            rv->dest = AluOP::HI;
            rv->slot = ALU2HI;
            return rv;
        }
    };
    DivMod(const Decode *op, int l) : AluOP(op, l) {}

    Instruction *pass1(Table *tbl, Table::Actions::Action *act) override {
        tbl->stage->table_use[timing_thread(tbl->gress)] |= Stage::USE_STATEFUL_DIVIDE;
        BUG_CHECK(tbl->to<StatefulTable>(), "stateful instruction on non-stateful table?");
        tbl->to<StatefulTable>()->divmod_used = true;
        return AluOP::pass1(tbl, act);
    }
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

// setz op, so can OR with alu1hi to get that result
DivMod::Decode opDIVMOD("divmod", JBAY, 0x00);

void DivMod::write_regs(Target::Tofino::mau_regs &, Table *, Table::Actions::Action *) {
    BUG("unsupported");
}
void DivMod::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    AluOP::write_regs(regs, tbl, act);
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    salu_instr_common.salu_divide_enable |= 1;
}

struct MinMax : public SaluInstruction {
    const struct Decode : public Instruction::Decode {
        std::string name;
        unsigned opcode;
        Decode(const char *name, target_t targ, int op)
            : Instruction::Decode(name, targ, STATEFUL_ALU), name(name), opcode(op) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    } *opc;
    bool phv = false;  // source is mem or phv
    operand mask, postmod;
    // constants for mask and postmod packed together
    boost::optional<unsigned> constval = boost::none;
    MinMax(const Decode *op, int l) : SaluInstruction(l), opc(op) {}
    std::string name() override { return opc->name; };
    Instruction *pass1(Table *tbl, Table::Actions::Action *) override;
    void pass2(Table *tbl, Table::Actions::Action *) override;
    bool salu_alu() const override { return true; }
    bool equiv(Instruction *a_) override {
        if (auto *a = dynamic_cast<MinMax *>(a_))
            return opc == a->opc && phv == a->phv && mask == a->mask && postmod == a->postmod;
        return false;
    }
    bool phvRead(std::function<void(const ::Phv::Slice &sl)>) override { return phv; }
    void dbprint(std::ostream &out) const override {
        out << "INSTR: " << opc->name << (phv ? "phv, " : "mem, ") << mask;
        if (postmod) out << ", " << postmod;
    }
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

MinMax::Decode opMIN8("min8", JBAY, 0), opMAX8("max8", JBAY, 1), opMIN16("min16", JBAY, 2),
    opMAX16("max16", JBAY, 3);

Instruction *MinMax::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                    const VECTOR(value_t) & op) const {
    auto *rv = new MinMax(this, op[0].lineno);
    if (op.size > 2) {
        if (op[1] == "phv")
            rv->phv = true;
        else if (op[1] != "mem")
            error(op[1].lineno, "%s source must be 'mem' or 'phv'", op[0].s);
        rv->mask = operand(tbl, act, op[2]);
        if (!rv->mask.to<operand::Phv>() && !rv->mask.to<operand::Const>())
            error(op[1].lineno, "%s mask must be constant or from phv or hash_dist", op[0].s);
    } else {
        error(op[0].lineno, "%s must have a single mask operand", op[0].s);
    }
    if (op.size == 4) {
        rv->postmod = operand(tbl, act, op[3]);
    } else if (op.size > 4) {
        error(op[0].lineno, "too many operands for %s", op[0].s);
    }
    rv->slot = MINMAX;
    return rv;
}
Instruction *MinMax::pass1(Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int mask_size = (opc->opcode & 2) ? 8 : 16;
    constval = boost::none;
    mask->pass1(tbl);
    act->minmax_use = true;
    if (auto k = mask.to<operand::Const>()) {
        if (k->value < 0 || k->value >= (1U << mask_size) || mask.neg)
            error(k->lineno, "%s mask value out of range", name().c_str());
        constval = k->value & ((1U << mask_size) - 1);
    } else if (auto p = mask.to<operand::Phv>()) {
        if (p->phv_index(tbl))
            error(lineno, "%s phv mask must come from the lower half input", name().c_str());
    } else {
        error(mask->lineno, "%s invalid mask", name().c_str());
    }
    if (postmod) {
        if (auto k = postmod.to<operand::Const>()) {
            if (k->value < 0) {
                k->value = -k->value;
                postmod.neg = !postmod.neg;
            }
            if (k->value > 255) error(lineno, "%s post mod too large", name().c_str());
            constval = constval.get_value_or(0) | (k->value & 0xff) << mask_size;
        } else if (auto p = postmod.to<operand::Phv>()) {
            if (!p->phv_index(tbl))
                error(lineno, "%s phv post mod must come from the upper half input",
                      name().c_str());
        } else {
            error(postmod->lineno, "%s invalid post mod", name().c_str());
        }
    }
    // We allocate the value here in order to report an error in pass1 if the capacity
    // of the register file is exceeded. The next call in write_regs with the same value
    // will return already allocated register file row index.
    if (constval) tbl->get_const(lineno, *constval);
    return this;
}
void MinMax::pass2(Table *tbl, Table::Actions::Action *act) {
    if (act->slot_use.intersects(bitvec(ALU2LO, 4)))
        error(lineno, "min/max requires all 4 stateful alu slots be unused");
}
void MinMax::write_regs(Target::JBay::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    if (auto k = mask.to<operand::Const>()) {
        salu_instr_common.salu_minmax_mask_ctl = 1;
    } else {
        salu_instr_common.salu_minmax_mask_ctl = 0;
    }
    salu_instr_common.salu_minmax_ctl = opc->opcode;
    salu_instr_common.salu_minmax_enable = 1;
    if (postmod) {
        if (auto k = postmod.to<operand::Const>()) {
            salu_instr_common.salu_minmax_postmod_value_ctl = 0;
        } else {
            salu_instr_common.salu_minmax_postmod_value_ctl = 1;
        }
        if (postmod.neg)
            salu_instr_common.salu_minmax_postdec_enable = 1;
        else
            salu_instr_common.salu_minmax_postinc_enable = 1;
    }
    if (constval) {
        auto &salu_instr_cmp = meter_group.stateful.salu_instr_cmp_alu[act->code][3];
        salu_instr_cmp.salu_cmp_regfile_adr = tbl->get_const(lineno, *constval);
    }
    // salu_instr_common.salu_minmax_src_sel = phv;  -- FIXME -- specify PHV source?
    for (auto &salu : meter_group.stateful.salu_instr_state_alu[act->code]) {
        salu.salu_op = 0xd;
        salu.salu_arith = 1;
        salu.salu_pred = 0xffff;
    }
}
void MinMax::write_regs(Target::Tofino::mau_regs &, Table *, Table::Actions::Action *) {
    BUG("unsupported");
}

template <>
void AluOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_state_alu[act->code][slot - ALU2LO];
    auto &salu_ext = meter_group.stateful.salu_instr2_state_alu[act->code][slot - ALU2LO];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    auto &salu_instr_output_alu = meter_group.stateful.salu_instr_output_alu[act->code];
    salu.salu_op = opc->opcode & 0xf;
    salu.salu_arith = opc->opcode >> 4;
    salu.salu_pred = predication_encode;
    bool need_flyover = (tbl->format->size >> tbl->is_dual_mode()) > 32;
    const int alu_const_min = Target::STATEFUL_ALU_CONST_MIN();
    const int alu_const_max = Target::STATEFUL_ALU_CONST_MAX();
    if (srca) {
        if (auto m = srca.to<operand::Memory>()) {
            salu.salu_asrc_input = m->field->bit(0) > 0 ? 1 : 0;
            if (need_flyover) {
                salu_ext.salu_flyover_src_sel = 1;
                need_flyover = false;
            }
        } else if (auto f = srca.to<operand::Phv>()) {
            salu.salu_asrc_input = f->phv_index(tbl) ? 3 : 2;
            if (need_flyover) {
                salu_ext.salu_flyover_src_sel = 1;
                need_flyover = false;
            }
        } else if (auto k = srca.to<operand::Const>()) {
            salu.salu_asrc_input = 4;
            if (k->value >= alu_const_min && k->value <= alu_const_max) {
                salu.salu_const_src = k->value & Target::STATEFUL_ALU_CONST_MASK();
                salu.salu_regfile_const = 0;
            } else {
                salu.salu_const_src = tbl->get_const(k->lineno, k->value);
                salu.salu_regfile_const = 1;
            }
        } else if (auto r = srca.to<operand::Regfile>()) {
            salu.salu_asrc_input = 4;
            salu.salu_const_src = r->index;
            salu.salu_regfile_const = 1;
        } else {
            BUG("invalid srca");
        }
    }
    if (srcb) {
        if (auto m = srcb.to<operand::Memory>()) {
            salu.salu_bsrc_input = m->field->bit(0) > 0 ? 3 : 2;
            if (need_flyover) {
                salu_ext.salu_flyover_src_sel = 0;
                need_flyover = false;
            }
        } else if (auto f = srcb.to<operand::Phv>()) {
            salu.salu_bsrc_input = f->phv_index(tbl) ? 1 : 0;
            if (need_flyover) {
                salu_ext.salu_flyover_src_sel = 0;
                need_flyover = false;
            }
        } else if (auto m = srcb.to<operand::MathFn>()) {
            salu_instr_common.salu_alu2_lo_bsrc_math = 1;
            if (auto b = m->of.to<operand::Phv>()) {
                salu_instr_common.salu_alu2_lo_math_src = b->phv_index(tbl);
            } else if (auto b = m->of.to<operand::Memory>()) {
                salu_instr_common.salu_alu2_lo_math_src = b->field->bit(0) > 0 ? 3 : 2;
            } else {
                BUG("invalid mathfn operand");
            }
        } else if (auto k = srcb.to<operand::Const>()) {
            salu.salu_bsrc_input = 4;
            if (k->value >= alu_const_min && k->value <= alu_const_max) {
                salu.salu_const_src = k->value & Target::STATEFUL_ALU_CONST_MASK();
                salu.salu_regfile_const = 0;
            } else {
                salu.salu_const_src = tbl->get_const(k->lineno, k->value);
                salu.salu_regfile_const = 1;
            }
        } else if (auto r = srcb.to<operand::Regfile>()) {
            salu.salu_bsrc_input = 4;
            salu.salu_const_src = r->index;
            salu.salu_regfile_const = 1;
        } else {
            BUG("invalid srcb");
        }
    }
}
void AluOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::JBay::mau_regs>(regs, tbl, act);
}

template <>
void BitOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    LOG2(this);
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_state_alu[act->code][slot - ALU2LO];
    salu.salu_op = opc->opcode & 0xf;
    salu.salu_pred = predication_encode;
    // 1b instructions are from mem-lo to alu1-lo
    salu.salu_asrc_input = 0;
}
void BitOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::JBay::mau_regs>(regs, tbl, act);
}

static int sbus_mask(int alu, const std::vector<Table::Ref> &tbls) {
    int rv = 0;
    for (auto &tbl : tbls) {
        int bit = tbl->layout[0].row / 4U;
        if (bit > alu) --bit;
        rv |= 1 << bit;
    }
    return rv;
}

template <>
void CmpOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_cmp_alu[act->code][slot];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    if (srca) {
        salu.salu_cmp_asrc_input = srca->field->bit(0) > 0;
        salu.salu_cmp_asrc_sign = srca_neg;
        salu.salu_cmp_asrc_enable = 1;
        if (maska != uint32_t(-1)) {
            salu.salu_cmp_asrc_mask_enable = 1;
            auto cval = 0;
            if (auto k = dynamic_cast<const operand::Const *>(srcc))
                cval = k->value;
            else if (auto r = dynamic_cast<const operand::Regfile *>(srcc))
                cval = tbl->get_const_val(r->index);
            int64_t min = Target::STATEFUL_CMP_CONST_MIN();
            int64_t max = Target::STATEFUL_CMP_CONST_MAX();
            bool maska_outside = (maska < uint32_t(min) && maska > max);
            bool maska_equal_inside = (uint32_t(cval) != maska && cval >= min && cval <= max);
            if (!maska_outside && !maska_equal_inside) {
                salu.salu_cmp_const_src = maska & Target::STATEFUL_CMP_CONST_MASK();
                salu.salu_cmp_mask_input = 0;
            } else {
                salu.salu_cmp_regfile_adr = tbl->get_const(srca->lineno, maska);
                salu.salu_cmp_mask_input = 1;
            }
        }
    }
    if (srcb) {
        salu.salu_cmp_bsrc_input = srcb->phv_index(tbl);
        salu.salu_cmp_bsrc_sign = srcb_neg;
        salu.salu_cmp_bsrc_enable = 1;
        if (maskb != uint32_t(-1)) {
            // uarch 6.2.12.6.1, masks for operand a/b are sourced from the
            // same regfile slot.
            if (salu.salu_cmp_asrc_mask_enable && salu.salu_cmp_mask_input && maskb != maska)
                error(lineno, "inconsistent operand mask %d and %d in salu compare operation",
                      maska, maskb);
            salu.salu_cmp_bsrc_mask_enable = 1;
            salu.salu_cmp_regfile_adr = tbl->get_const(srcb->lineno, maskb);
        }
    }
    if (srcc) {
        if (auto k = dynamic_cast<const operand::Const *>(srcc)) {
            const int cmp_const_min = Target::STATEFUL_CMP_CONST_MIN();
            const int cmp_const_max = Target::STATEFUL_CMP_CONST_MAX();
            if (k->value >= cmp_const_min && k->value <= cmp_const_max) {
                salu.salu_cmp_const_src = k->value & Target::STATEFUL_CMP_CONST_MASK();
                salu.salu_cmp_regfile_const = 0;
            } else {
                // uarch 6.2.12.6.1, masks for operand a/b are sourced from the
                // same regfile slot as operand c if c is a constant.
                if (salu.salu_cmp_asrc_mask_enable && salu.salu_cmp_mask_input &&
                    maska != uint32_t(k->value))
                    error(lineno, "inconsistent operand mask %d and %d in salu compare operation",
                          maska, uint32_t(k->value));
                if (salu.salu_cmp_bsrc_mask_enable && salu.salu_cmp_mask_input &&
                    maskb != uint32_t(k->value))
                    error(lineno, "inconsistent operand mask %d and %d in salu compare operation",
                          maskb, uint32_t(k->value));
                salu.salu_cmp_regfile_adr = tbl->get_const(srcc->lineno, k->value);
                salu.salu_cmp_regfile_const = 1;
            }
        } else if (auto r = dynamic_cast<const operand::Regfile *>(srcc)) {
            salu.salu_cmp_regfile_adr = r->index;
            salu.salu_cmp_regfile_const = 1;
        }
    } else {
        salu.salu_cmp_const_src = 0;
        salu.salu_cmp_regfile_const = 0;
    }
    salu.salu_cmp_opcode = opc->opcode | (type << 2);
    auto lmask = sbus_mask(logical_home_row / 4U, tbl->sbus_learn);
    auto mmask = sbus_mask(logical_home_row / 4U, tbl->sbus_match);
    salu_instr_common.salu_lmatch_sbus_listen = lmask;
    salu_instr_common.salu_match_sbus_listen = mmask;
    salu_instr_common.salu_sbus_in_comb = tbl->sbus_comb;
    if (lmask || mmask) {
        // if lmask and mmask are both zero, these registers don't matter, but the model
        // will assert if they are non-zero)
        salu.salu_cmp_sbus_or = 0;
        salu.salu_cmp_sbus_and = learn ? 1 : 0;
        salu.salu_cmp_sbus_invert = learn_not ? 1 : 0;
    }
}
void CmpOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::JBay::mau_regs>(regs, tbl, act);
}

template <>
void TMatchOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_cmp_alu[act->code][slot];
    auto &salu_tmatch = meter_group.stateful.salu_instr_tmatch_alu[act->code][slot];
    auto &salu_instr_common = meter_group.stateful.salu_instr_common[act->code];
    salu.salu_cmp_tmatch_enable = 1;
    salu.salu_cmp_asrc_enable = 1;
    salu.salu_cmp_bsrc_enable = 1;
    meter_group.stateful.tmatch_mask[slot][0] = ~mask & 0xffffffffU;
    meter_group.stateful.tmatch_mask[slot][1] = ~mask >> 32;
    salu.salu_cmp_opcode = 2;
    salu.salu_cmp_asrc_input = srca->field->bit(0) > 0;
    salu.salu_cmp_bsrc_input = srcb->phv_index(tbl);
    if (auto lmask = sbus_mask(logical_home_row / 4U, tbl->sbus_learn))
        salu_instr_common.salu_lmatch_sbus_listen = lmask;
    if (auto mmask = sbus_mask(logical_home_row / 4U, tbl->sbus_match))
        salu_instr_common.salu_match_sbus_listen = mmask;
    salu.salu_cmp_sbus_or = 0;
    salu.salu_cmp_sbus_and = learn ? 1 : 0;
    salu.salu_cmp_sbus_invert = learn_not ? 1 : 0;
    // we set the learn output unconditionally if there's a tmatch -- should it be controllable?
    salu_tmatch.salu_tmatch_vld_ctl = 1;
    // salu_tmatch.salu_tmatch_invert = 0;  -- when can this be useful?
}

void TMatchOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::JBay::mau_regs>(regs, tbl, act);
}

void OutOP::decode_output_mux(Target::JBay, Table *tbl, value_t &op) {
    static const std::map<std::string, int> ops_mux_lookup = {
        {"mem_hi", 1},    {"mem_lo", 0},  {"memory_hi", 1}, {"memory_lo", 0},    {"phv_hi", 3},
        {"phv_lo", 2},    {"alu_hi", 5},  {"alu_lo", 4},    {"minmax_index", 5}, {"minmax_post", 4},
        {"predicate", 6}, {"address", 7}, {"div", 8},       {"mod", 9},          {"minmax", 10}};
    if (op.type == tCMD && ops_mux_lookup.count(op[0].s))
        output_mux = ops_mux_lookup.at(op[0].s);
    else if (op.type == tSTR && ops_mux_lookup.count(op.s))
        output_mux = ops_mux_lookup.at(op.s);
    else
        output_mux = -1;
    if (src) {
        int tmp = output_mux;
        if (auto *phv = src.to<operand::Phv>())
            output_mux = 2 + phv->phv_index(tbl->to<StatefulTable>());
        else if (auto *mem = src.to<operand::Memory>())
            output_mux = mem->field->bit(0) > 0 ? 1 : 0;
        BUG_CHECK(tmp < 0 || tmp == output_mux, "inconsistent output mux decode");
    }
}
int OutOP::decode_output_option(Target::JBay, value_t &op) {
    if (op == "lmatch") {
        lmatch = true;
        if (op.type == tCMD)
            lmatch_pred = decode_predicate(op[1]);
        else
            lmatch_pred = STATEFUL_PREDICATION_ENCODE_UNCOND;
    } else {
        return -1;
    }
    return 0;
}

template <>
void OutOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl_, Table::Actions::Action *act) {
    LOG2(this);
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    int logical_home_row = tbl->layout[0].row;
    auto &meter_group = regs.rams.map_alu.meter_group[logical_home_row / 4U];
    auto &salu = meter_group.stateful.salu_instr_output_alu[act->code][slot - ALUOUT0];
    if (predication_encode) {
        salu.salu_output_cmpfn = predication_encode;
    } else {
        salu.salu_output_cmpfn = STATEFUL_PREDICATION_ENCODE_UNCOND;
    }
    salu.salu_output_asrc = output_mux;
    if ((salu.salu_lmatch_adr_bit_enable = lmatch))
        meter_group.stateful.salu_mathtable[0] = lmatch_pred;
    if (output_mux == STATEFUL_PREDICATION_OUTPUT)
        meter_group.stateful.stateful_ctl.salu_output_pred_sel = slot - ALUOUT0;
}
void OutOP::write_regs(Target::JBay::mau_regs &regs, Table *tbl, Table::Actions::Action *act) {
    write_regs<Target::JBay::mau_regs>(regs, tbl, act);
}
