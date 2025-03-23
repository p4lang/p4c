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

#include <cstring>

#include <boost/optional.hpp>

#include "backends/tofino/bf-asm/config.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "instruction.h"
#include "lib/hex.h"
#include "phv.h"

namespace StatefulAlu {

struct operand : public IHasDbPrint {
    struct Base : public IHasDbPrint {
        int lineno;
        explicit Base(int line) : lineno(line) {}
        Base(const Base &a) : lineno(a.lineno) {}
        virtual ~Base() {}
        virtual Base *clone() const = 0;
        virtual void dbprint(std::ostream &) const = 0;
        virtual bool equiv(const Base *) const = 0;
        virtual const char *kind() const = 0;
        virtual Base *lookup(Base *&) { return this; }
        virtual bool phvRead(std::function<void(const ::Phv::Slice &sl)>) { return false; }
        virtual void pass1(StatefulTable *) {}
    } *op;
    struct Const : public Base {
        int64_t value;
        Const *clone() const override { return new Const(*this); }
        Const(int line, int64_t v) : Base(line), value(v) {}
        void dbprint(std::ostream &out) const override { out << value; }
        bool equiv(const Base *a_) const override {
            if (auto *a = dynamic_cast<const Const *>(a_)) {
                return value == a->value;
            } else {
                return false;
            }
        }
        const char *kind() const override { return "constant"; }
    };
    // Operand representing a constant stored in the register file
    struct Regfile : public Base {
        int index = -1;
        Regfile *clone() const override { return new Regfile(*this); }
        Regfile(int line, int index) : Base(line), index(index) {}
        Regfile(int line, const value_t &n) : Base(line) {
            if (PCHECKTYPE2M(n.vec.size == 2, n[1], tINT, tBIGINT, "SALU regfile row reference"))
                index = get_int64(n[1], sizeof(index) / 8, "regfile row index out of bounds");
        }
        void dbprint(std::ostream &out) const override { out << index; }
        bool equiv(const Base *a_) const override {
            if (auto *a = dynamic_cast<const Regfile *>(a_)) {
                return index == a->index;
            } else {
                return false;
            }
        }
        const char *kind() const override { return "register file constant"; }
    };
    struct Phv : public Base {
        virtual Phv *clone() const = 0;
        explicit Phv(int lineno) : Base(lineno) {}
        virtual int phv_index(StatefulTable *tbl) = 0;
    };
    struct PhvReg : public Phv {
        ::Phv::Ref reg;
        PhvReg *clone() const override { return new PhvReg(*this); }
        PhvReg(gress_t gress, int stage, const value_t &v) : Phv(v.lineno), reg(gress, stage, v) {}
        void dbprint(std::ostream &out) const override { out << reg; }
        bool equiv(const Base *a_) const override {
            if (auto *a = dynamic_cast<const PhvReg *>(a_)) {
                return reg == a->reg;
            } else {
                return false;
            }
        }
        const char *kind() const override { return "phv_reg"; }
        void pass1(StatefulTable *tbl) override {
            if (!reg.check()) return;
            int size = tbl->format->begin()->second.size / 8;
            if (tbl->input_xbar.empty()) {
                error(lineno, "No input xbar for salu instruction operand for phv");
                return;
            }
            BUG_CHECK(tbl->input_xbar.size() == 1, "%s does not have one input xbar", tbl->name());
            int byte = tbl->find_on_ixbar(*reg, tbl->input_xbar[0]->match_group());
            int base = options.target == TOFINO ? 8 : 0;
            if (byte < 0)
                error(lineno, "Can't find %s on the input xbar", reg.name());
            else if (byte != base && byte != base + size)
                error(lineno, "%s must be at %d or %d on ixbar to be used in stateful table %s",
                      reg.desc().c_str(), base * 8, (base + size) * 8, tbl->name());
            else if (int(reg->size()) > size * 8)
                error(lineno, "%s is too big for stateful table %s", reg.desc().c_str(),
                      tbl->name());
            else
                tbl->phv_byte_mask |= ((1U << (reg->size() + 7) / 8U) - 1) << (byte - base);
        }
        int phv_index(StatefulTable *tbl) override {
            int base = options.target == TOFINO ? 8 : 0;
            return tbl->find_on_ixbar(*reg, tbl->input_xbar[0]->match_group()) > base;
        }
        bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override {
            fn(*reg);
            return true;
        }
    };
    // Operand which directly accesses phv(hi/lo) from Input Xbar
    struct PhvRaw : public Phv {
        int pi = -1;
        unsigned mask = ~0U;
        PhvRaw *clone() const override { return new PhvRaw(*this); }
        PhvRaw(gress_t gress, const value_t &v) : Phv(v.lineno) {
            if (v == "phv_lo")
                pi = 0;
            else if (v == "phv_hi")
                pi = 1;
            else
                BUG("Unknown phv operand %s", v.s);
            if (v.type == tCMD && PCHECKTYPE(v.vec.size == 2, v[1], tRANGE)) {
                if ((v[1].range.lo & 7) || ((v[1].range.hi + 1) & 7))
                    error(lineno, "only byte slices allowed on %s", v[0].s);
                mask = (1U << (v[1].range.hi + 1) / 8U) - (1U << (v[1].range.lo / 8U));
            }
        }
        void dbprint(std::ostream &out) const override { out << (pi ? "phv_hi" : "phv_lo"); }
        bool equiv(const Base *a_) const override {
            if (auto *a = dynamic_cast<const PhvRaw *>(a_)) {
                return pi == a->pi;
            } else {
                return false;
            }
        }
        const char *kind() const override { return "phv_ixb"; }
        void pass1(StatefulTable *tbl) override {
            int size = tbl->format->begin()->second.size / 8U;
            if (mask == ~0U)
                mask = (1U << size) - 1;
            else if (mask & ~((1U << size) - 1))
                error(lineno, "slice out of range for %d byte value", size);
            tbl->phv_byte_mask |= mask << (size * pi);
        }
        int phv_index(StatefulTable *tbl) override { return pi; }
        bool phvRead(std::function<void(const ::Phv::Slice &sl)>) override { return true; }
    };
    struct Memory : public Base {
        Table *tbl;
        Table::Format::Field *field;
        Memory *clone() const override { return new Memory(*this); }
        Memory(int line, Table *t, Table::Format::Field *f) : Base(line), tbl(t), field(f) {}
        void dbprint(std::ostream &out) const override { out << tbl->format->find_field(field); }
        bool equiv(const Base *a_) const override {
            if (auto *a = dynamic_cast<const Memory *>(a_)) {
                return field == a->field;
            } else {
                return false;
            }
        }
        const char *kind() const override { return "memory"; }
    };
    struct MathFn;
    bool neg = false;
    uint64_t mask = uint32_t(-1);
    operand() : op(0) {}
    operand(const operand &a) : op(a.op ? a.op->clone() : 0) {}
    operand(operand &&a) : op(a.op) { a.op = 0; }
    operand &operator=(const operand &a) {
        if (&a != this) {
            delete op;
            op = a.op ? a.op->clone() : 0;
        }
        return *this;
    }
    operand &operator=(operand &&a) {
        if (&a != this) {
            delete op;
            op = a.op;
            a.op = 0;
        }
        return *this;
    }
    ~operand() { delete op; }
    operand(Table *tbl, const Table::Actions::Action *act, const value_t &v, bool can_mask = false);
    bool valid() const { return op != 0; }
    explicit operator bool() const { return op != 0; }
    bool operator==(operand &a) {
        return op == a.op || (op && a.op && op->lookup(op)->equiv(a.op->lookup(a.op)));
    }
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) {
        return op ? op->lookup(op)->phvRead(fn) : false;
    }
    void dbprint(std::ostream &out) const {
        if (neg) out << '-';
        if (op)
            op->dbprint(out);
        else
            out << "(null)";
    }
    Base *operator->() { return op->lookup(op); }
    template <class T>
    T *to() {
        return dynamic_cast<T *>(op);
    }
};

struct operand::MathFn : public Base {
    operand of;
    MathFn *clone() const override { return new MathFn(*this); }
    MathFn(int line, operand of) : Base(line), of(of) {}
    void dbprint(std::ostream &out) const override {
        out << "math(" << of << ")";
        ;
    }
    bool equiv(const Base *a_) const override {
        if (auto *a = dynamic_cast<const MathFn *>(a_)) {
            return of.op == a->of.op;
        } else {
            return false;
        }
    }
    const char *kind() const override { return "math fn"; }
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) { return of->phvRead(fn); }
    void pass1(StatefulTable *tbl) override { of->pass1(tbl); }
};

operand::operand(Table *tbl, const Table::Actions::Action *act, const value_t &v_, bool can_mask)
    : op(nullptr) {
    const value_t *v = &v_;
    if (options.target == TOFINO) can_mask = false;
    if (can_mask && v->type == tCMD && *v == "&" && v->vec.size == 3) {
        if (v->vec[2].type == tINT || v->vec[2].type == tBIGINT) {
            mask = get_int64(v->vec[2], 64, "mask too large");
            v = &v->vec[1];
        } else if (v->vec[1].type == tINT || v->vec[1].type == tBIGINT) {
            mask = get_int64(v->vec[1], 64, "mask too large");
            v = &v->vec[2];
        } else {
            error(v->lineno, "mask must be a constant");
        }
    }
    if (v->type == tCMD && *v == "-") {
        neg = true;
        v = &v->vec[1];
    }
    if (v->type == tINT || v->type == tBIGINT) {
        auto i = get_int64(*v, 64, "Integer too large");
        op = new Const(v->lineno, i);
        return;
    }
    if (v->type == tCMD && *v == "register_param") {
        op = new Regfile(v->lineno, *v);
        return;
    }
    if (v->type == tSTR) {
        if (auto f = tbl->format->field(v->s)) {
            op = new Memory(v->lineno, tbl, f);
            return;
        }
    }
    if (v->type == tCMD) {
        BUG_CHECK(v->vec.size > 0 && v->vec[0].type == tSTR, "bad operand");
        if (auto f = tbl->format->field(v->vec[0].s)) {
            if (v->vec.size > 1 && CHECKTYPE(v->vec[1], tRANGE) && v->vec[1].range.lo != 0)
                error(v->vec[1].lineno, "Can't slice memory field %s in stateful action",
                      v->vec[0].s);
            op = new Memory(v->lineno, tbl, f);
            return;
        }
    }
    if ((v->type == tCMD) && (v->vec[0] == "math_table")) {
        // operand *opP = new operand(tbl, act, v->vec[1]);
        op = new MathFn(v->lineno, operand(tbl, act, v->vec[1]));
        return;
    }
    if (*v == "phv_lo" || *v == "phv_hi") {
        op = new PhvRaw(tbl->gress, *v);
        return;
    }
    if (::Phv::Ref(tbl->gress, tbl->stage->stageno, *v).check(false))
        op = new PhvReg(tbl->gress, tbl->stage->stageno, *v);
}

enum salu_slot_use {
    CMP0,
    CMP1,
    CMP2,
    CMP3,
    ALU2LO,
    ALU1LO,
    ALU2HI,
    ALU1HI,
    ALUOUT0,
    ALUOUT1,
    ALUOUT2,
    ALUOUT3,
    MINMAX,
    // aliases
    CMPLO = CMP0,
    CMPHI = CMP1,
    ALUOUT = ALUOUT0,
};

// Abstract interface class for SALU Instructions
// SALU Instructions - AluOP, BitOP, CmpOP, OutOP
struct SaluInstruction : public Instruction {
    explicit SaluInstruction(int lineno) : Instruction(lineno) {}
    // Stateful ALU's dont access PHV's directly
    static int decode_predicate(const value_t &exp);
};

int SaluInstruction::decode_predicate(const value_t &exp) {
    if (exp == "cmplo") return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMPLO;
    if (exp == "cmphi") return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMPHI;
    if (exp == "cmp0") return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMP0;
    if (Target::STATEFUL_CMP_UNITS() > 1 && exp == "cmp1")
        return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMP1;
    if (Target::STATEFUL_CMP_UNITS() > 2 && exp == "cmp2")
        return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMP2;
    if (Target::STATEFUL_CMP_UNITS() > 3 && exp == "cmp3")
        return Target::STATEFUL_PRED_MASK() & STATEFUL_PREDICATION_ENCODE_CMP3;
    if (exp == "!") return Target::STATEFUL_PRED_MASK() ^ decode_predicate(exp[1]);
    if (exp == "&") {
        auto rv = decode_predicate(exp[1]);
        for (int i = 2; i < exp.vec.size; ++i) rv &= decode_predicate(exp[i]);
        return rv;
    }
    if (exp == "|") {
        auto rv = decode_predicate(exp[1]);
        for (int i = 2; i < exp.vec.size; ++i) rv |= decode_predicate(exp[i]);
        return rv;
    }
    if (exp == "^") {
        auto rv = decode_predicate(exp[1]);
        for (int i = 2; i < exp.vec.size; ++i) rv ^= decode_predicate(exp[i]);
        return rv;
    }
    if (exp.type == tINT && exp.i >= 0 && exp.i <= Target::STATEFUL_PRED_MASK()) return exp.i;
    error(exp.lineno, "Unexpected expression %s in predicate", value_desc(&exp));
    return -1;
}

struct AluOP : public SaluInstruction {
    const struct Decode : public Instruction::Decode {
        std::string name;
        unsigned opcode;
        enum operands_t { NONE, A, B, AandB } operands = AandB;
        const Decode *swap_args;
        Decode(const char *n, int opc, bool assoc = false, const char *alias_name = 0)
            : Instruction::Decode(n, STATEFUL_ALU),
              name(n),
              opcode(opc),
              swap_args(assoc ? this : 0) {
            if (alias_name) alias(alias_name, STATEFUL_ALU);
        }
        Decode(const char *n, int opc, Decode *sw, const char *alias_name = 0,
               operands_t use = AandB)
            : Instruction::Decode(n, STATEFUL_ALU),
              name(n),
              opcode(opc),
              operands(use),
              swap_args(sw) {
            if (sw && !sw->swap_args) sw->swap_args = this;
            if (alias_name) alias(alias_name, STATEFUL_ALU);
        }
        Decode(const char *n, int opc, const char *alias_name)
            : Instruction::Decode(n, STATEFUL_ALU), name(n), opcode(opc), swap_args(0) {
            if (alias_name) alias(alias_name, STATEFUL_ALU);
        }
        Decode(const char *n, int opc, bool assoc, operands_t use)
            : Instruction::Decode(n, STATEFUL_ALU),
              name(n),
              opcode(opc),
              operands(use),
              swap_args(assoc ? this : 0) {}
        Decode(const char *n, int opc, const char *alias_name, operands_t use)
            : Instruction::Decode(n, STATEFUL_ALU),
              name(n),
              opcode(opc),
              operands(use),
              swap_args(0) {
            if (alias_name) alias(alias_name, STATEFUL_ALU);
        }
        Decode(const char *n, int opc, Decode *sw, operands_t use)
            : Instruction::Decode(n, STATEFUL_ALU),
              name(n),
              opcode(opc),
              operands(use),
              swap_args(sw) {
            if (sw && !sw->swap_args) sw->swap_args = this;
        }
        Decode(const char *n, target_t targ, int opc)
            : Instruction::Decode(n, targ, STATEFUL_ALU), name(n), opcode(opc), swap_args(0) {}

        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    } *opc;
    int predication_encode = STATEFUL_PREDICATION_ENCODE_UNCOND;
    enum dest_t { LO, HI };
    dest_t dest = LO;
    operand srca, srcb;
    AluOP(const Decode *op, int l) : SaluInstruction(l), opc(op) {}
    std::string name() override { return opc->name; };
    Instruction *pass1(Table *tbl, Table::Actions::Action *) override;
    void pass2(Table *tbl, Table::Actions::Action *) override {}
    bool salu_alu() const override { return true; }
    bool equiv(Instruction *a_) override;
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override {
        return srca.phvRead(fn) | srcb.phvRead(fn);
    }
    void dbprint(std::ostream &out) const override {
        out << "INSTR: " << opc->name << " pred=0x" << hex(predication_encode) << " "
            << (dest ? "hi" : "lo") << ", " << srca << ", " << srcb;
    }
    template <class REGS>
    void write_regs(REGS &regs, Table *tbl, Table::Actions::Action *act);
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

static AluOP::Decode opADD("add", 0x1c, true), opSUB("sub", 0x1e), opSADDU("saddu", 0x10, true),
    opSADDS("sadds", 0x11, true), opSSUBU("ssubu", 0x12), opSSUBS("ssubs", 0x13),
    opMINU("minu", 0x14, true), opMINS("mins", 0x15, true), opMAXU("maxu", 0x16, true),
    opMAXS("maxs", 0x17, true), opNOP("nop", 0x18, true, AluOP::Decode::NONE),
    opSUBR("subr", 0x1f, &opSUB), opSSUBRU("ssubru", 0x1a, &opSSUBU),
    opSSUBRS("ssubrs", 0x1b, &opSSUBS),

    opSETZ("setz", 0x00, true, AluOP::Decode::NONE), opNOR("nor", 0x01, true),
    opANDCA("andca", 0x02), opNOTA("nota", 0x03, "not", AluOP::Decode::A),
    opANDCB("andcb", 0x04, &opANDCA), opNOTB("notb", 0x05, &opNOTA, AluOP::Decode::B),
    opXOR("xor", 0x06, true), opNAND("nand", 0x07, true), opAND("and", 0x08, true),
    opXNOR("xnor", 0x09, true), opB("alu_b", 0x0a, "b", AluOP::Decode::B), opORCA("orca", 0x0b),
    opA("alu_a", 0x0c, &opB, "a", AluOP::Decode::A), opORCB("orcb", 0x0d, &opORCA),
    opOR("or", 0x0e, true), opSETHI("sethi", 0x0f, true, AluOP::Decode::NONE);

Instruction *AluOP::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                   const VECTOR(value_t) & op) const {
    AluOP *rv = new AluOP(this, op[0].lineno);
    auto operands = this->operands;
    int idx = 1;
    // Check optional predicate operand
    if (idx < op.size) {
        if (op[idx].type == tINT) {
            // Predicate is an integer. no warning for odd values
            rv->predication_encode = op[idx++].i;
        } else if (op[idx].startsWith("cmp") || op[idx] == "!" || op[idx] == "&" ||
                   op[idx] == "|" || op[idx] == "^") {
            // Predicate is an expression
            rv->predication_encode = decode_predicate(op[idx++]);
            if (rv->predication_encode == STATEFUL_PREDICATION_ENCODE_NOOP)
                warning(op[idx - 1].lineno, "Instruction predicate is always false");
            else if (rv->predication_encode == STATEFUL_PREDICATION_ENCODE_UNCOND)
                warning(op[idx - 1].lineno, "Instruction predicate is always true");
        }
    }
    if (idx < op.size && op[idx] == "lo") {
        rv->dest = LO;
        idx++;
    } else if (idx < op.size && op[idx] == "hi") {
        rv->dest = HI;
        idx++;
    } else if (idx == op.size && name == "nop") {
        // allow nop without even a destination -- assume lo
        rv->dest = LO;
    } else {
        error(rv->lineno, "invalid destination for %s instruction", op[0].s);
    }
    if (operands == NONE) {
        if (idx < op.size) error(rv->lineno, "too many operands for %s instruction", op[0].s);
        return rv;
    }
    if (idx < op.size && operands != B) rv->srca = operand(tbl, act, op[idx++]);
    if (idx < op.size && operands != A) rv->srcb = operand(tbl, act, op[idx++]);
    if (swap_args && (rv->srca.to<operand::Phv>() || rv->srca.to<operand::MathFn>() ||
                      (rv->srcb.to<operand::Memory>() &&
                       (rv->srca.to<operand::Const>() || rv->srca.to<operand::Regfile>())))) {
        operands = (rv->opc = swap_args)->operands;
        std::swap(rv->srca, rv->srcb);
    }
    if (idx < op.size)
        error(rv->lineno, "too many operands for %s instruction", op[0].s);
    else if ((!rv->srca && operands != B) || (!rv->srcb && operands != A))
        error(rv->lineno, "not enough operands for %s instruction", op[0].s);
    if (auto mf = rv->srca.to<operand::MathFn>()) {
        error(rv->lineno, "Can't reference math table in %soperand of %s instruction",
              operands != A ? "first " : "", op[0].s);
        if (!mf->of.to<operand::Phv>() && !mf->of.to<operand::Memory>())
            error(rv->lineno, "Math table input must come from Phv or memory");
    }
    if (rv->srca.to<operand::Phv>())
        error(rv->lineno, "Can't reference phv in %soperand of %s instruction",
              operands != A ? "first " : "", op[0].s);
    if (rv->srcb.to<operand::Memory>())
        error(rv->lineno, "Can't reference memory in %soperand of %s instruction",
              operands != A ? "first " : "", op[0].s);
    if (auto mf = rv->srcb.to<operand::MathFn>()) {
        rv->slot = ALU2LO;
        if (rv->dest != LO) error(rv->lineno, "Can't reference math table in alu-hi");
        if (!mf->of.to<operand::Phv>() && !mf->of.to<operand::Memory>())
            error(rv->lineno, "Math table input must come from Phv or memory");
    }
    if (rv->srca.neg) {
        if (auto k = rv->srca.to<operand::Const>())
            k->value = -k->value;
        else
            error(rv->lineno, "Can't negate operand of %s instruction", op[0].s);
    }
    if (rv->srcb.neg) {
        if (auto k = rv->srcb.to<operand::Const>())
            k->value = -k->value;
        else
            error(rv->lineno, "Can't negate operand of %s instruction", op[0].s);
    }
    return rv;
}

bool AluOP::equiv(Instruction *a_) {
    if (auto *a = dynamic_cast<AluOP *>(a_))
        return opc == a->opc && predication_encode == a->predication_encode && dest == a->dest &&
               srca == a->srca && srcb == a->srcb;
    return false;
}

Instruction *AluOP::pass1(Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "expected stateful table");
    if (slot < 0 && act->slot_use[slot = (dest ? ALU1HI : ALU1LO)]) slot = dest ? ALU2HI : ALU2LO;
    auto k1 = srca.to<operand::Const>();
    auto k2 = srcb.to<operand::Const>();
    // Check cases when both constants would be stored in the register file on different rows
    // Two constants that do not fit as immediate constants
    if (k1 && k2 && !k1->equiv(k2))
        error(lineno, "can only have one constant in an SALU instruction");
    if (!k1) k1 = k2;
    if (k1 && (k1->value < Target::STATEFUL_ALU_CONST_MIN() ||
               k1->value > Target::STATEFUL_ALU_CONST_MAX())) {
        auto min_value = -(INT64_C(1) << (tbl->alu_size() - 1));
        auto max_value = (INT64_C(1) << tbl->alu_size()) - 1;
        if (k1->value < min_value || k1->value > max_value) {
            error(lineno,
                  "value %" PRIi64
                  " of the constant operand"
                  " out of range for %d bit stateful ALU",
                  k1->value, tbl->alu_size());
        } else if (k1->value >= (INT64_C(1) << (Target::STATEFUL_REGFILE_CONST_WIDTH() - 1))) {
            // constants have a limited width, and are always signed, so need to make
            // sure they wrap properly
            k1->value -= INT64_C(1) << Target::STATEFUL_REGFILE_CONST_WIDTH();
            if (k2 && k2 != k1) k2->value = k1->value;
        }
    }
    auto r1 = srca.to<operand::Regfile>();
    auto r2 = srcb.to<operand::Regfile>();
    if (r1 && r2 && !r1->equiv(r2))
        error(lineno, "can only have one register file reference in an SALU instruction");
    if (!r1) r1 = r2;
    if (r1) {
        int64_t v1 = tbl->get_const_val(r1->index);
        auto min_value = -(INT64_C(1) << (tbl->alu_size() - 1));
        auto max_value = (INT64_C(1) << tbl->alu_size()) - 1;
        if (v1 < min_value || v1 > max_value) {
            error(lineno,
                  "initial value %" PRIi64
                  " of the register file operand"
                  " out of range for %d bit stateful ALU",
                  v1, tbl->alu_size());
        }
    }
    if (k1 && r1)
        error(lineno,
              "can have either a constant or a register file reference"
              " in an SALU instruction");
    if (srca) srca->pass1(tbl);
    if (srcb) srcb->pass1(tbl);
    return this;
}

Instruction *genNoop(StatefulTable *tbl, Table::Actions::Action *act) {
    VECTOR(value_t) args = EMPTY_VECTOR_INIT;
    BUG_CHECK(tbl->format->begin() != tbl->format->end(), "No tbl->format!");
    args.add("or").add("lo").add(0).add(tbl->format->begin()->first.c_str());
    auto *rv = Instruction::decode(tbl, act, args);
    VECTOR_fini(args);
    return rv;
}

struct BitOP : public SaluInstruction {
    const struct Decode : public Instruction::Decode {
        std::string name;
        unsigned opcode;
        Decode(const char *n, unsigned opc)
            : Instruction::Decode(n, STATEFUL_ALU), name(n), opcode(opc) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    } *opc;
    int predication_encode = STATEFUL_PREDICATION_ENCODE_UNCOND;
    BitOP(const Decode *op, int lineno) : SaluInstruction(lineno), opc(op) {}
    std::string name() override { return opc->name; };
    Instruction *pass1(Table *, Table::Actions::Action *) override {
        slot = ALU1LO;
        return this;
    }
    void pass2(Table *, Table::Actions::Action *) override {}
    bool salu_alu() const override { return true; }
    bool equiv(Instruction *a_) override;
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override { return false; }
    void dbprint(std::ostream &out) const override { out << "INSTR: " << opc->name; }
    template <class REGS>
    void write_regs(REGS &regs, Table *tbl, Table::Actions::Action *act);
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

static BitOP::Decode opSET_BIT("set_bit", 0x0), opSET_BITC("set_bitc", 0x1),
    opCLR_BIT("clr_bit", 0x2), opCLR_BITC("clr_bitc", 0x3), opREAD_BIT("read_bit", 0x4),
    opREAD_BITC("read_bitc", 0x5), opSET_BIT_AT("set_bit_at", 0x6),
    opSET_BITC_AT("set_bitc_at", 0x7), opCLR_BIT_AT("clr_bit_at", 0x8),
    opCLR_BITC_AT("clr_bitc_at", 0x9);

Instruction *BitOP::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                   const VECTOR(value_t) & op) const {
    BitOP *rv = new BitOP(this, op[0].lineno);
    if (op.size > 1) error(rv->lineno, "too many operands for %s instruction", op[0].s);
    return rv;
}

bool BitOP::equiv(Instruction *a_) {
    if (auto *a = dynamic_cast<BitOP *>(a_)) return opc == a->opc;
    return false;
}

struct CmpOP : public SaluInstruction {
    const struct Decode : public Instruction::Decode {
        std::string name;
        unsigned opcode;
        Decode(const char *n, unsigned opc, bool type)
            : Instruction::Decode(n, STATEFUL_ALU, type), name(n), opcode(opc) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    } *opc;
    int type = 0;
    operand::Memory *srca = 0;
    uint32_t maska = 0xffffffffU;
    operand::Phv *srcb = 0;
    uint32_t maskb = 0xffffffffU;
    operand::Base *srcc = 0;  // operand::Const or operand::Regfile
    bool srca_neg = false, srcb_neg = false;
    bool learn = false, learn_not = false;
    CmpOP(const Decode *op, int lineno) : SaluInstruction(lineno), opc(op) {}
    std::string name() override { return opc->name; };
    Instruction *pass1(Table *tbl, Table::Actions::Action *) override;
    void pass2(Table *tbl, Table::Actions::Action *) override {}
    bool equiv(Instruction *a_) override;
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override {
        bool rv = false;
        if (srca) rv |= srca->phvRead(fn);
        if (srcb) rv |= srcb->phvRead(fn);
        if (srcc) rv |= srcc->phvRead(fn);
        return rv;
    }
    void dbprint(std::ostream &out) const override {
        out << "INSTR: " << opc->name << " cmp" << slot;
        if (srca) {
            out << ", " << (srca_neg ? "-" : "") << *srca;
            if (maska != 0xffffffffU) out << " & 0x" << hex(maska);
        }
        if (srcb) {
            out << ", " << (srcb_neg ? "-" : "") << *srcb;
            if (maskb != 0xffffffffU) out << " & 0x" << hex(maskb);
        }
        if (srcc) out << ", " << *srcc;
        if (learn) out << ", learn";
        if (learn_not) out << ", learn_not";
    }
    template <class REGS>
    void write_regs(REGS &regs, Table *tbl, Table::Actions::Action *act);
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

static CmpOP::Decode opEQU("equ", 0, false), opNEQ("neq", 1, false), opGRT("grt", 0, true),
    opLEQ("leq", 1, true), opGEQ("geq", 2, true), opLSS("lss", 3, true);

Instruction *CmpOP::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                   const VECTOR(value_t) & op) const {
    auto rv = new CmpOP(this, op[0].lineno);
    if (auto *p = strchr(op[0].s, '.')) {
        if (type_suffix && !strcmp(p, ".s"))
            rv->type = 1;
        else if (type_suffix && !strcmp(p, ".u"))
            rv->type = 2;
        else if (type_suffix && !strcmp(p, ".uus"))
            rv->type = 3;
        else
            error(rv->lineno, "Invalid type %s for %s instruction", p + 1, name.c_str());
    } else if (type_suffix) {
        error(rv->lineno, "Missing type for %s instruction", name.c_str());
    }
    if (op.size < 1 || op[1].type != tSTR) {
        error(rv->lineno, "invalid destination for %s instruction", op[0].s);
        return rv;
    }
    unsigned unit;
    int len;
    if (op[1] == "lo") {
        rv->slot = CMPLO;
    } else if (op[1] == "hi") {
        rv->slot = CMPHI;
    } else if ((sscanf(op[1].s, "p%u%n", &unit, &len) >= 1 ||
                sscanf(op[1].s, "cmp%u%n", &unit, &len) >= 1) &&
               unit < Target::STATEFUL_CMP_UNITS() && op[1].s[len] == 0) {
        rv->slot = CMP0 + unit;
    } else {
        error(rv->lineno, "invalid destination for %s instruction", op[0].s);
    }
    for (int idx = 2; idx < op.size; ++idx) {
        if (!rv->learn) {
            if (op[idx] == "learn") {
                rv->learn = true;
                continue;
            }
            if (op[idx] == "!" && op[idx].type == tCMD && op[idx].vec.size == 2 &&
                op[idx][1] == "learn") {
                rv->learn = rv->learn_not = true;
                continue;
            }
        }
        operand src(tbl, act, op[idx], true);
        if (!rv->srca && (rv->srca = src.to<operand::Memory>())) {
            rv->srca_neg = src.neg;
            rv->maska = src.mask;
            src.op = nullptr;
        } else if (!rv->srcb && (rv->srcb = src.to<operand::Phv>())) {
            rv->srcb_neg = src.neg;
            rv->maskb = src.mask;
            src.op = nullptr;
        } else if (!rv->srcc && (rv->srcc = src.to<operand::Const>())) {
            auto *srcc = src.to<operand::Const>();
            if (src.neg) srcc->value = -srcc->value;
            if (src.mask != ~0U) srcc->value &= src.mask;
            src.op = nullptr;
        } else if (!rv->srcc && (rv->srcc = src.to<operand::Regfile>())) {
            if (src.neg || src.mask != ~0U)
                error(src->lineno, "Register file operand cannot be negated or masked");
            src.op = nullptr;
        } else if (src) {
            error(src->lineno, "Can't have more than one %s operand to an SALU compare",
                  src->kind());
        }
    }
    return rv;
}

bool CmpOP::equiv(Instruction *a_) {
    if (auto *a = dynamic_cast<CmpOP *>(a_))
        return opc == a->opc && slot == a->slot && srca == a->srca && maska == a->maska &&
               srcb == a->srcb && maskb == a->maskb && srcc == a->srcc && learn == a->learn &&
               learn_not == a->learn_not;
    return false;
}

Instruction *CmpOP::pass1(Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "Expected stateful table");
    if (srca) srca->pass1(tbl);
    if (srcb) srcb->pass1(tbl);
    if (srcc) srcc->pass1(tbl);
    return this;
}

struct TMatchOP : public SaluInstruction {
    const struct Decode : public Instruction::Decode {
        std::string name;
        Decode(const char *n, target_t target)
            : Instruction::Decode(n, target, STATEFUL_ALU), name(n) {}
        Decode(const char *n, std::set<target_t> target)
            : Instruction::Decode(n, target, STATEFUL_ALU), name(n) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    } *opc;
    operand::Memory *srca = 0;
    uint64_t mask = 0;
    operand::Phv *srcb = 0;
    bool learn = false, learn_not = false;
    TMatchOP(const Decode *op, int lineno) : SaluInstruction(lineno), opc(op) {}
    std::string name() override { return opc->name; };
    Instruction *pass1(Table *tbl, Table::Actions::Action *) override;
    void pass2(Table *tbl, Table::Actions::Action *) override {}
    bool equiv(Instruction *a_) override;
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override {
        return srcb ? srcb->phvRead(fn) : false;
    }
    void dbprint(std::ostream &out) const override {
        out << "INSTR: " << opc->name << " cmp" << slot;
        if (srca) out << ", " << *srca;
        if (mask) out << ", 0x" << hex(mask);
        if (srcb) out << ", " << *srcb;
        if (learn) out << ", learn";
        if (learn_not) out << ", learn_not";
    }
    template <class REGS>
    void write_regs(REGS &regs, Table *tbl, Table::Actions::Action *act);
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

static TMatchOP::Decode opTMatch("tmatch", {
                                               JBAY,
                                           });

Instruction *TMatchOP::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                      const VECTOR(value_t) & op) const {
    auto rv = new TMatchOP(this, op[0].lineno);
    if (op.size < 1 || op[1].type != tSTR) {
        error(rv->lineno, "invalid destination for %s instruction", op[0].s);
        return rv;
    }
    unsigned unit;
    int len;
    if ((sscanf(op[1].s, "p%u%n", &unit, &len) >= 1 ||
         sscanf(op[1].s, "cmp%u%n", &unit, &len) >= 1) &&
        unit < Target::STATEFUL_TMATCH_UNITS() && op[1].s[len] == 0) {
        rv->slot = CMP0 + unit;
    } else {
        error(rv->lineno, "invalid destination for %s instruction", op[0].s);
    }
    for (int idx = 2; idx < op.size; ++idx) {
        if (!rv->learn) {
            if (op[idx] == "learn") {
                rv->learn = true;
                continue;
            }
            if (op[idx] == "!" && op[idx].type == tCMD && op[idx].vec.size == 2 &&
                op[idx][1] == "learn") {
                rv->learn = rv->learn_not = true;
                continue;
            }
        }
        if (op[idx].type == tINT || op[idx].type == tBIGINT) {
            if (rv->mask)
                error(op[idx].lineno, "Can't have more than one mask operand to an SALU tmatch");
            rv->mask = get_int64(op[idx], 64, "Integer too large");
        } else if (op[idx].type == tSTR) {
            if (auto f = tbl->format->field(op[idx].s)) {
                if (rv->srca) {
                    error(op[idx].lineno,
                          "Can't have more than one memory operand to an "
                          "SALU tmatch");
                    delete rv->srca;
                }
                rv->srca = new operand::Memory(op[idx].lineno, tbl, f);
            } else if (rv->srcb) {
                error(op[idx].lineno, "Can't have more than one phv operand to an SALU tmatch");
            } else if (op[idx] == "phv_lo" || op[idx] == "phv_hi") {
                rv->srcb = new operand::PhvRaw(tbl->gress, op[idx]);
            } else {
                rv->srcb = new operand::PhvReg(tbl->gress, tbl->stage->stageno, op[idx]);
            }
        }
    }
    if (!rv->srca || !rv->srcb || !rv->mask)
        error(rv->lineno, "Not enough operands to SALU tmatch");
    return rv;
}

bool TMatchOP::equiv(Instruction *a_) {
    if (auto *a = dynamic_cast<TMatchOP *>(a_))
        return opc == a->opc && slot == a->slot && srca == a->srca && srcb == a->srcb &&
               mask == a->mask && learn == a->learn && learn_not == a->learn_not;
    return false;
}

Instruction *TMatchOP::pass1(Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "Expected stateful table");
    if (srca) srca->pass1(tbl);
    if (srcb) srcb->pass1(tbl);
    if (tbl->tmatch_use[slot].op) {
        if (mask != tbl->tmatch_use[slot].op->mask) {
            error(lineno, "Incompatable tmatch masks in stateful actions %s and %s",
                  tbl->tmatch_use[slot].act->name.c_str(), act->name.c_str());
            error(tbl->tmatch_use[slot].op->lineno, "previous use");
        }
    } else {
        tbl->tmatch_use[slot].act = act;
        tbl->tmatch_use[slot].op = this;
    }
    return this;
}

// Output ALU instruction
struct OutOP : public SaluInstruction {
    struct Decode : public Instruction::Decode {
        explicit Decode(const char *n) : Instruction::Decode(n, STATEFUL_ALU) {}
        Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                            const VECTOR(value_t) & op) const override;
    };
    int predication_encode = STATEFUL_PREDICATION_ENCODE_UNCOND;
    operand src;
    int output_mux = -1;
    bool lmatch = false;
    int lmatch_pred = 0;
    FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void decode_output_mux,
                          (register_type, Table *tbl, value_t &op))
    void decode_output_mux(Table *tbl, value_t &op) {
        SWITCH_FOREACH_TARGET(options.target, decode_output_mux(TARGET(), tbl, op););
    }
    FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, int decode_output_option, (register_type, value_t &op))
    int decode_output_option(value_t &op) {
        SWITCH_FOREACH_TARGET(options.target, return decode_output_option(TARGET(), op););
    }
    OutOP(const Decode *op, int lineno) : SaluInstruction(lineno) {}
    std::string name() override { return "output"; };
    Instruction *pass1(Table *tbl, Table::Actions::Action *) override;
    void pass2(Table *tbl, Table::Actions::Action *) override {}
    bool salu_output() const override { return true; }
    bool equiv(Instruction *a_) override;
    bool phvRead(std::function<void(const ::Phv::Slice &sl)> fn) override {
        return src ? src->phvRead(fn) : false;
    }
    void dbprint(std::ostream &out) const override {
        out << "INSTR: output " << "pred=0x" << hex(predication_encode) << " word"
            << (slot - ALUOUT0) << " mux=" << output_mux;
    }
    template <class REGS>
    void write_regs(REGS &regs, Table *tbl, Table::Actions::Action *act);
    FOR_ALL_REGISTER_SETS(DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS)
};

static OutOP::Decode opOUTPUT("output");

bool OutOP::equiv(Instruction *a_) {
    if (auto *a = dynamic_cast<OutOP *>(a_))
        return predication_encode == a->predication_encode && slot == a->slot &&
               output_mux == a->output_mux;
    return false;
}

Instruction *OutOP::Decode::decode(Table *tbl, const Table::Actions::Action *act,
                                   const VECTOR(value_t) & op) const {
    OutOP *rv = new OutOP(this, op[0].lineno);
    int idx = 1;
    // Check optional predicate operand
    if (idx < op.size) {
        // Predicate is an integer
        if (op[idx].type == tINT) {
            rv->predication_encode = op[idx++].i;
            // Predicate is an expression
        } else if (op[idx].startsWith("cmp") || op[idx] == "!" || op[idx] == "&" ||
                   op[idx] == "|" || op[idx] == "^") {
            rv->predication_encode = decode_predicate(op[idx++]);
            if (rv->predication_encode == STATEFUL_PREDICATION_ENCODE_NOOP)
                warning(op[idx - 1].lineno, "Instruction predicate is always false");
            else if (rv->predication_encode == STATEFUL_PREDICATION_ENCODE_UNCOND)
                warning(op[idx - 1].lineno, "Instruction predicate is always true");
        }
    }
    rv->slot = ALUOUT;
    // Check for destination
    if (idx < op.size && op[idx].startsWith("word")) {
        int unit = -1;
        char *end;
        if (op[idx].type == tSTR) {
            if (isdigit(op[idx].s[4])) {
                unit = strtol(op[idx].s + 4, &end, 10);
                if (*end) unit = -1;
            }
        } else if (op[idx].vec.size == 2 && op[idx][1].type == tINT) {
            unit = op[idx][1].i;
        }
        if (unit >= Target::STATEFUL_OUTPUT_UNITS())
            error(op[idx].lineno, "Invalid output dest %s", value_desc(op[idx]));
        else
            rv->slot = unit + ALUOUT0;
        idx++;
    }
    // Check mux operand
    if (idx < op.size) {
        rv->src = operand(tbl, act, op[idx], false);
        // DANGER -- decoding the output mux here (as part of input parsing) requires that
        // the phv section be before the section we're currently parsing in the .bfa file.
        // That's always the case with compiler output, but do we want to require it for
        // hand-written code?  Could reorg stuff to do this in pass1 instead.
        rv->decode_output_mux(tbl, op[idx]);
        if (rv->output_mux < 0)
            error(op[idx].lineno, "invalid operand '%s' for '%s' instruction", value_desc(op[idx]),
                  op[0].s);
        idx++;
    } else {
        error(rv->lineno, "too few operands for %s instruction", op[0].s);
    }
    while (idx < op.size) {
        if (rv->decode_output_option(op[idx]) < 0) break;
        ++idx;
    }
    if (idx < op.size) error(rv->lineno, "too many operands for %s instruction", op[0].s);

    return rv;
}

Instruction *OutOP::pass1(Table *tbl_, Table::Actions::Action *act) {
    auto tbl = dynamic_cast<StatefulTable *>(tbl_);
    BUG_CHECK(tbl, "Expected stateful table");
    if (src) src->pass1(tbl);
    if (output_mux == STATEFUL_PREDICATION_OUTPUT) {
        if (act->pred_comb_sel >= 0 && act->pred_comb_sel != predication_encode)
            error(lineno, "Only one output of predication allowed");
        act->pred_comb_sel = predication_encode;
    }
    if (lmatch) {
        if (tbl->output_lmatch) {
            auto *other = dynamic_cast<OutOP *>(tbl->output_lmatch);
            BUG_CHECK(other, "Expected outOP");
            if (lmatch_pred != other->lmatch_pred) {
                error(lineno, "Conflict lmatch output use in stateful %s", tbl->name());
                error(other->lineno, "conflicting use here");
            }
        }
        tbl->output_lmatch = this;
    }
    return this;
}

#include "jbay/salu_inst.cpp"    // NOLINT(build/include)
#include "tofino/salu_inst.cpp"  // NOLINT(build/include)

}  // end namespace StatefulAlu

bool StatefulTable::p4c_5192_workaround(const Actions::Action *act) const {
    // when trying to output bits 96..127
    // of either memory or phv input in an SALU in 128-bit mode, the model asserts
    // Not clear if this is a hardware limitation or a model bug.
    // RMT_ASSERTS on lines 547 and 565 of model/src/shared/mau-stateful-alu.cpp
    // Workaround is to use 64x2 mode instead which is otherwise equivalent, except
    // for possible problems if minmax is used
    using namespace StatefulAlu;
    if (format->log2size != 7 || is_dual_mode()) return false;  // only apply in 128-bit mode
    for (auto &inst : act->instr) {
        if (auto *out = dynamic_cast<const OutOP *>(inst.get())) {
            if (out->slot > ALUOUT1 && (out->output_mux == 1 || out->output_mux == 3)) {
                return true;
            }
        }
    }
    return false;
}
