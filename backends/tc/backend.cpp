#include <cassert>
#include <type_traits>
#include <typeinfo>

/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include <filesystem>

#include "backend.h"
#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/target.h"

namespace P4::TC {

using namespace P4::literals;

const cstring Extern::dropPacket = "drop_packet"_cs;
const cstring Extern::sendToPort = "send_to_port"_cs;

cstring pnaMainParserInputMetaFields[TC::MAX_PNA_PARSER_META] = {"recirculated"_cs,
                                                                 "input_port"_cs};

cstring pnaMainInputMetaFields[TC::MAX_PNA_INPUT_META] = {
    "recirculated"_cs, "timestamp"_cs, "parser_error"_cs, "class_of_service"_cs, "input_port"_cs};

cstring pnaMainOutputMetaFields[TC::MAX_PNA_OUTPUT_META] = {"class_of_service"_cs};

const cstring pnaParserMeta = "pna_main_parser_input_metadata_t"_cs;
const cstring pnaInputMeta = "pna_main_input_metadata_t"_cs;
const cstring pnaOutputMeta = "pna_main_output_metadata_t"_cs;

static void append_type_for_width(EBPF::CodeBuilder *bld, unsigned int w) {
    if (w > 64)
        bld->appendFormat("struct internal_bit_%u", w);
    else
        bld->append("u64");
}

static const char *cmp_string(unsigned char c) {
    switch (c) {
        case CMP_EQ:
            return ("eq");
            break;
        case CMP_NE:
            return ("ne");
            break;
        case CMP_LT:
            return ("lt");
            break;
        case CMP_LE:
            return ("le");
            break;
        case CMP_GT:
            return ("gt");
            break;
        case CMP_GE:
            return ("ge");
            break;
        /* Just returning here to avoid g++ warning */
        default:
            return "";
    }
    assert(!"Invalid call to cmp_string");
}

Visitor::profile_t ScanWidths::init_apply(const IR::Node *n) {
    return (this->Inspector::init_apply(n));
}

void ScanWidths::end_apply(const IR::Node *n) { Inspector::end_apply(n); }

void ScanWidths::expr_common(const IR::Expression *e) {
    const IR::Type *t;

    t = typemap->getType(e, true);
    if (t->is<IR::Type_Type>()) t = t->to<const IR::Type_Type>()->type;
    if (t->is<IR::Type_Bits>()) {
        add_width(t->width_bits());
    } else if (t->is<IR::Type_Varbits>()) {
        add_width(t->max_width_bits());
    }
}

bool ScanWidths::preorder(const IR::Expression *e) {
    expr_common(e);
    return (true);
}

bool ScanWidths::preorder(const IR::Concat *e) {
    const IR::Type *lt;
    const IR::Type *rt;

    expr_common(e);
    lt = typemap->getType(e->left, true);
    if (lt->is<IR::Type_Type>()) lt = lt->to<const IR::Type_Type>()->type;
    rt = typemap->getType(e->right, true);
    if (rt->is<IR::Type_Type>()) rt = rt->to<const IR::Type_Type>()->type;
    if (lt->is<IR::Type_Bits>() && rt->is<IR::Type_Bits>()) {
        add_concat(lt->width_bits(), rt->width_bits());
    }
    return (true);
}

bool ScanWidths::arith_common_2(const IR::Operation_Binary *e, WrType t, bool docommon) {
    if (docommon) expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt && (lt->width_bits() == rt->width_bits())) {
            add_arith(t, lt->width_bits());
        }
    }
    return (true);
}

bool ScanWidths::arith_common_1(const IR::Operation_Unary *e, WrType t, bool docommon) {
    if (docommon) expr_common(e);
    auto et = e->expr->type->to<IR::Type_Bits>();
    if (et) add_arith(t, et->width_bits());
    return (true);
}

bool ScanWidths::bigXSmallMul(const IR::Expression *big, const IR::Constant *small) {
    auto bt = big->type->to<IR::Type_Bits>();
    if (bt) add_bxsmul(bt->width_bits(), static_cast<unsigned int>(small->value));
    return (true);
}

bool ScanWidths::sarith_common_2(const IR::Operation_Binary *e, WrType t, bool docommon) {
    if (docommon) expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt && (lt->width_bits() == rt->width_bits())) {
            add_sarith(t, lt->width_bits(), lt->isSigned);
        }
    }
    return (true);
}

bool ScanWidths::preorder(const IR::Add *e) { return (arith_common_2(e, WR_ADD)); }

bool ScanWidths::preorder(const IR::Sub *e) { return (arith_common_2(e, WR_SUB)); }

bool ScanWidths::preorder(const IR::Mul *e) {
    const IR::Constant *cx;

    expr_common(e);
    cx = e->left->to<IR::Constant>();
    if (cx && (cx->value >= 0) && (cx->value < (1U << 22))) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt && (rt->width_bits() > 64)) {
            return (bigXSmallMul(e->right, cx));
        }
    }
    cx = e->right->to<IR::Constant>();
    if (cx && (cx->value >= 0) && (cx->value < (1U << 22))) {
        auto rt = e->left->type->to<IR::Type_Bits>();
        if (rt && (rt->width_bits() > 64)) {
            return (bigXSmallMul(e->left, cx));
        }
    }
    return (arith_common_2(e, WR_MUL, false));
}

bool ScanWidths::preorder(const IR::Cast *e) {
    expr_common(e);
    expr_common(e->expr);
    if (e->expr->type->is<IR::Type_Bits>() && e->type->is<IR::Type_Bits>()) {
        add_cast(e->expr->type->to<IR::Type_Bits>()->width_bits(),
                 e->type->to<IR::Type_Bits>()->width_bits());
    }
    return (true);
}

bool ScanWidths::preorder(const IR::Neg *e) { return (arith_common_1(e, WR_NEG)); }

bool ScanWidths::preorder(const IR::Cmpl *e) { return (arith_common_1(e, WR_NOT)); }

bool ScanWidths::preorder(const IR::Shl *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rc = e->right->to<IR::Constant>();
        if (rc) {
            if (rc->value < lt->width_bits()) {
                add_shift_c(WR_SHL_C, lt->width_bits(), static_cast<unsigned int>(rc->value));
            }
        } else {
            auto rt = e->right->type->to<IR::Type_Bits>();
            if (rt) {
                add_shift_x(WR_SHL_X, lt->width_bits(), rt->width_bits());
            }
        }
    }
    return (true);
}

bool ScanWidths::preorder(const IR::Shr *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rc = e->right->to<IR::Constant>();
        if (rc) {
            if (rc->value < lt->width_bits()) {
                add_shift_c(lt->isSigned ? WR_SHRA_C : WR_SHRL_C, lt->width_bits(),
                            static_cast<unsigned int>(rc->value));
            }
        } else {
            auto rt = e->right->type->to<IR::Type_Bits>();
            if (rt) {
                add_shift_x(lt->isSigned ? WR_SHRA_X : WR_SHRL_X, lt->width_bits(),
                            rt->width_bits());
            }
        }
    }
    return (true);
}

bool ScanWidths::preorder(const IR::Equ *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_EQ);  // isSigned doesn't matter for eq
    return (true);
}

bool ScanWidths::preorder(const IR::MethodCallExpression *mx) {
    auto mi = P4::MethodInstance::resolve(mx, refmap, typemap);
    auto em = mi->to<P4::ExternMethod>();
    P4::P4CoreLibrary &p4lib = P4::P4CoreLibrary::instance();
    if (em && (em->method->name.name == p4lib.packetIn.extract.name)) {
        int nargs = em->expr->arguments->size();
        const IR::Expression *arg1 = em->expr->arguments->at(0)->expression;
        auto xt = typemap->getType(arg1);
        auto slt = xt->to<IR::Type_StructLike>();

        if (nargs == 2 && slt->variable()) add_width(xt->max_width_bits());
    }
    return true;
}

bool ScanWidths::preorder(const IR::Neq *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_NE);  // isSigned doesn't matter for ne
    return (true);
}

bool ScanWidths::preorder(const IR::Lss *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_LT | (lt->isSigned ? CMP_SIGNED : 0));
    return (true);
}

bool ScanWidths::preorder(const IR::Leq *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_LE | (lt->isSigned ? CMP_SIGNED : 0));
    return (true);
}

bool ScanWidths::preorder(const IR::Grt *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_GT | (lt->isSigned ? CMP_SIGNED : 0));
    return (true);
}

bool ScanWidths::preorder(const IR::Geq *e) {
    expr_common(e);
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) add_cmp(lt->width_bits(), CMP_GE | (lt->isSigned ? CMP_SIGNED : 0));
    return (true);
}

bool ScanWidths::preorder(const IR::BAnd *e) { return (arith_common_2(e, WR_BITAND)); }

bool ScanWidths::preorder(const IR::BOr *e) { return (arith_common_2(e, WR_BITOR)); }

bool ScanWidths::preorder(const IR::BXor *e) { return (arith_common_2(e, WR_BITXOR)); }

bool ScanWidths::preorder(const IR::AddSat *e) { return (sarith_common_2(e, WR_ADDSAT)); }

bool ScanWidths::preorder(const IR::SubSat *e) { return (sarith_common_2(e, WR_SUBSAT)); }

bool ScanWidths::preorder(const IR::AssignmentStatement *s) {
    auto et = EBPF::EBPFTypeFactory::instance->create(typemap->getType(s->left));
    if (et->is<EBPF::EBPFScalarType>()) {
        auto bits = et->to<EBPF::EBPFScalarType>()->implementationWidthInBits();
        auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(target);
        if ((bits > 64) && !tcTarget->isPrimitiveByteAligned(bits)) add_assign(bits);
    }
    return (true);
}

void ScanWidths::insert_wr(WidthRec &wr) {
    int l;
    int m;
    int h;
    int i;

    l = -1;
    h = nwr;
    while (h - l > 1) {
        m = (h + l) / 2;
        if (wrv[m] <= wr) l = m;
        if (wrv[m] >= wr) h = m;
    }
    if (l == h) return;
    wrv = static_cast<WidthRec *>(std::realloc(wrv, (nwr + 1) * sizeof(*wrv)));
    for (i = nwr; i > h; i--) wrv[i] = wrv[i - 1];
    nwr++;
    wrv[h] = wr;
}

void ScanWidths::add_width(int w) {
    WidthRec wr;

    if (w <= 64) return;
    wr.type = WR_VALUE;
    wr.u.value = w;
    insert_wr(wr);
}

void ScanWidths::add_concat(int lw, int rw) {
    WidthRec wr;

    assert((lw > 0) && (lw < 1048576) && (rw > 0) && (rw < 1048576));
    if (lw + rw <= 64) return;
    wr.type = WR_CONCAT;
    wr.u.concat.lhsw = lw;
    wr.u.concat.rhsw = rw;
    insert_wr(wr);
}

void ScanWidths::add_arith(WrType t, int w) {
    WidthRec wr;

    assert((w > 0) && (w < 1048576));
    if (w <= 64) return;
    wr.type = t;
    wr.u.arith.w = w;
    insert_wr(wr);
}

void ScanWidths::add_bxsmul(int bw, unsigned int sv) {
    WidthRec wr;

    assert((bw > 0) && (bw < 1048576) && (sv < (1U << 22)));
    if (bw <= 64) return;
    wr.type = WR_BXSMUL;
    wr.u.bxsmul.bw = bw;
    wr.u.bxsmul.sv = sv;
    insert_wr(wr);
}

void ScanWidths::add_cast(unsigned int fw, unsigned int tw) {
    WidthRec wr;

    if (((fw <= 64) && (tw <= 64)) || (fw == tw)) return;
    wr.type = WR_CAST;
    wr.u.cast.fw = fw;
    wr.u.cast.tw = tw;
    insert_wr(wr);
}

void ScanWidths::add_shift_c(WrType t, unsigned int lw, unsigned int sv) {
    WidthRec wr;

    if (lw <= 64) return;
    wr.type = t;
    wr.u.shift_c.lw = lw;
    wr.u.shift_c.sv = sv;
    insert_wr(wr);
}

void ScanWidths::add_shift_x(WrType t, unsigned int lw, unsigned int rw) {
    WidthRec wr;

    if ((lw <= 64) && (rw <= 64)) return;
    wr.type = t;
    wr.u.shift_x.lw = lw;
    wr.u.shift_x.rw = rw;
    insert_wr(wr);
}

void ScanWidths::add_cmp(unsigned int w, unsigned char c) {
    WidthRec wr;

    if (w <= 64) return;
    wr.type = WR_CMP;
    wr.u.cmp.w = w;
    wr.u.cmp.cmp = c;
    insert_wr(wr);
}

void ScanWidths::add_sarith(WrType t, int w, bool sgn) {
    WidthRec wr;

    assert((w > 0) && (w < 1048576));
    wr.type = t;
    wr.u.sarith.w = w;
    wr.u.sarith.issigned = sgn;
    insert_wr(wr);
}

void ScanWidths::add_assign(unsigned int w) {
    WidthRec wr;

    assert(w > 64);
    wr.type = WR_ASSIGN;
    wr.u.assign.w = w;
    insert_wr(wr);
}

static void gen_concat(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int rw;
    unsigned int i;

    assert(wr->type == WR_CONCAT);
    lw = wr->u.concat.lhsw;
    rw = wr->u.concat.rhsw;
    bld->newline();
    bld->appendFormat("static __always_inline struct internal_bit_%u concat_%u_%u(", lw + rw, lw,
                      rw);
    append_type_for_width(bld, lw);
    bld->append(" lhs, ");
    append_type_for_width(bld, rw);
    bld->append(" rhs)\n");
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", lw + rw);
    bld->append("\n");
    if (rw % 8) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Not yet implemented for RHS width %u",
                    rw);
    } else {
        if (rw > 64) {
            bld->appendFormat(" __builtin_memcpy(&ret.bits[0],&rhs.bits[0],%u);\n", rw >> 3);
        } else {
            for (i = 0; i < rw; i += 8) {
                bld->appendFormat(" ret.bits[%u] = (rhs >> %u) & 255;\n", i >> 3, i);
            }
        }
    }
    if (lw > 64) {
        bld->appendFormat(" __builtin_memcpy(&ret.bits[%u],&lhs.bits[0],%u);\n", rw >> 3,
                          (lw + 7) >> 3);
    } else {
        for (i = 0; i < lw; i += 8) {
            bld->appendFormat(" ret.bits[%u] = (lhs >> %u) & %u;\n", (rw >> 3) + (i >> 3), i,
                              ((i + 8) <= lw) ? 255 : (255 >> ((i + 8) - lw)));
        }
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_add(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int bits;
    unsigned int bytes;
    int i;
    const char *suf;

    assert(wr->type == WR_ADD);
    bits = wr->u.arith.w;
    bytes = (bits + 7) >> 3;
    assert(bits > 64);
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u add_%u(struct internal_bit_%u lhs, struct "
        "internal_bit_%u rhs)\n",
        bits, bits, bits, bits);
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", bits);
    bld->append(" u16 a;\n");
    bld->append("\n");
    bld->append(" SETGUARDS(ret);\n");
    suf = "";
    for (i = bytes - 1; i >= 0; i--) {
        bld->appendFormat(" a = BITS(lhs)[%d] + BITS(rhs)[%d]%s;\n", i, i, suf);
        suf = " + (a >> 8)";
        bld->appendFormat(" BITS(ret)[%d] = a & ", i);
        if (i > 0)
            bld->append("255");
        else
            bld->appendFormat("%u", 255 >> ((bytes * 8) - bits));
        bld->append(";\n");
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_sub(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;
    unsigned int b;
    int i;
    const char *suf;

    assert(wr->type == WR_SUB);
    w = wr->u.arith.w;
    b = (w + 7) >> 3;
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u sub_%u(struct internal_bit_%u lhs, struct "
        "internal_bit_%u rhs)\n",
        w, w, w, w);
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", w);
    bld->append(" u16 a;\n");
    bld->append("\n");
    bld->append(" SETGUARDS(ret);\n");
    suf = "";
    for (i = b - 1; i >= 0; i--) {
        bld->appendFormat(" a = BITS(lhs)[%u] - BITS(rhs)[%u]%s;\n", i, i, suf);
        suf = " - ((a >> 8) & 1)";
        bld->appendFormat(" BITS(ret)[%u] = a & ", i);
        if (i > 0)
            bld->append("255");
        else
            bld->appendFormat("%u", 255 >> ((b * 8) - w));
        bld->append(";\n");
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_mul(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;
    unsigned int b;
    unsigned int i;
    int j;

    assert(wr->type == WR_MUL);
    w = wr->u.arith.w;
    b = (w + 7) >> 3;
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u mul_%u(struct internal_bit_%u lhs, struct "
        "internal_bit_%u rhs)\n",
        w, w, w, w);
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", w);
    bld->append(" u32 a;\n");
    bld->append("\n");
    bld->append(" SETGUARDS(ret);\n");
    for (i = 0; i < b; i++) {
        bld->appendFormat(" a =%s", i ? " (a >> 8) +" : "");
        for (j = i; j >= 0; j--) {
            bld->appendFormat(" (BITS(lhs)[%d] * BITS(rhs)[%d])%s", b - 1 - j, b - 1 - ((int)i - j),
                              j ? " +" : "");
        }
        bld->append(";\n");
        bld->appendFormat(" BITS(ret)[%u] = a & ", b - 1 - i);
        if (i + 1 < b)
            bld->append("255");
        else
            bld->appendFormat("%u", 255U >> ((b * 8) - w));
        bld->append(";\n");
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_bxsmul(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int bw;
    unsigned long long int sv;
    unsigned int b;
    int i;
    const char *suf;

    assert(wr->type == WR_BXSMUL);
    bw = wr->u.bxsmul.bw;
    sv = wr->u.bxsmul.sv;
    b = (bw + 7) >> 3;
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u bxsmul_%u_%llu(struct internal_bit_%u arg "
        ")\n",
        bw, bw, sv, bw);
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", bw);
    if (sv > 0x007fffff) {
        bld->append(" u64 a;\n");
        suf = "ULL";
    } else {
        bld->append(" u32 a;\n");
        suf = "U";
    }
    bld->append("\n");
    bld->append(" SETGUARDS(ret);\n");
    for (i = b - 1; i >= 0; i--) {
        bld->appendFormat(" a =%s (BITS(arg)[%u] * %llu%s);\n",
                          (i < (int)b - 1) ? " (a >> 8) +" : "", i, sv, suf);
        bld->appendFormat(" BITS(ret)[%u] = a & ", i);
        if (i > 0)
            bld->append("255");
        else
            bld->appendFormat("%u", 255U >> ((b * 8) - bw));
        bld->append(";\n");
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_neg(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;
    int i;
    int j;
    int bytes;

    assert(wr->type == WR_NEG);
    w = wr->u.arith.w;
    assert(w > 64);
    bytes = (w + 7) >> 3;
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u neg_%u(struct internal_bit_%u arg "
        ")\n",
        w, w, w);
    bld->append("{\n");
    bld->append(" u16 a;\n");
    bld->appendFormat(" struct internal_bit_%u ret;\n", w);
    bld->append("\n");
    bld->append(" SETGUARDS(ret);\n");
    for (i = (w >> 3) - 1, j = 0; i >= 0; i--, j++) {
        bld->appendFormat(" a = %s + (255 ^ BITS(arg)[%d]);\n", j ? "(a >> 8)" : "1",
                          bytes - 1 - j);
        bld->appendFormat(" BITS(ret)[%d] = a & 255;\n", bytes - 1 - j);
    }
    if (w & 7) {  // always a>>8: w>64, so the above loop ran >once
        bld->appendFormat(" BITS(ret)[0] = ((a >> 8) + (255 ^ BITS(arg)[0])) & %d;\n",
                          (1 << (w & 7)) - 1);
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

/*
 * Signed addition overflows exactly when the arguments have the same
 *  sign but the (overflow-ignored) result has a different sign,
 *  implemented here as ~(lhs^rhs) & (ret^lhs) & signbit.
 *
 * Unsigned addition overflows exactly when there is a carry out of the
 *  high bit; equivalently, if the (overflow-ignored) result is less
 *  than at least one of the input operands.
 */
static void gen_addsat(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int bits;
    unsigned int bytes;
    int i;
    const char *suf;

    assert(wr->type == WR_ADDSAT);
    bits = wr->u.sarith.w;
    bytes = (bits + 7) >> 3;
    bld->newline();
    bld->append("static __always_inline ");
    append_type_for_width(bld, bits);
    bld->appendFormat(" addsat_%d(", bits);
    append_type_for_width(bld, bits);
    bld->append(" lhs, ");
    append_type_for_width(bld, bits);
    bld->append(" rhs");
    if (bits > 64) bld->append(" ");
    bld->append(")\n");
    bld->append("{\n");
    bld->append(" ");
    append_type_for_width(bld, bits);
    bld->append(" ret;\n");
    bld->append("\n");
    if (bits <= 64) {
        unsigned long long int max;
        max = (bits < 64) ? (1ULL << bits) - 1ULL : 0xffffffffffffffffULL;
        // let the optimizer delete the &0x...ULL when appropriate
        bld->appendFormat(" ret = (lhs + rhs) & 0x%llxULL;\n", max);
        if (wr->u.sarith.issigned) {
            bld->appendFormat(
                " if (~(lhs ^ rhs) & (ret ^ lhs) & (1ULL << %u)) ret = (lhs & (1ULL << %u)) ? "
                "0x%llxULL : 0x%llxULL;\n",
                bits - 1, bits - 1, max & ~(max >> 1), max);
        } else {
            bld->appendFormat(" if ((ret < lhs) || (ret < rhs)) ret = 0x%llxULL;\n", max);
        }
    } else {
        bld->append(" u16 a;\n");
        bld->append("\n");
        bld->append(" SETGUARDS(ret);\n");
        suf = "";
        for (i = bytes - 1; i >= 0; i--) {
            bld->appendFormat(" a = BITS(lhs)[%d] + BITS(rhs)[%d]%s;\n", i, i, suf);
            suf = " + (a >> 8)";
            bld->appendFormat(" BITS(ret)[%d] = a & ", i);
            if (i > 0)
                bld->append("255");
            else
                bld->appendFormat("%u", 255 >> ((bytes * 8) - bits));
            bld->append(";\n");
        }
        if (wr->u.sarith.issigned) {
            unsigned int signbit;
            signbit = 128U >> ((bytes * 8) - bits);
            bld->appendFormat(" if (~(BITS(lhs)[0] ^ BITS(rhs)[0]) & (a ^ BITS(lhs)[0]) & %u)\n",
                              signbit);
            bld->appendFormat("  { if (BITS(lhs)[0] & %u)\n", signbit);
            bld->appendFormat("     { __builtin_memset(&BITS(ret)[1],0,%u);\n", bytes - 1);
            bld->appendFormat("       BITS(ret)[0] = %u;\n", signbit);
            bld->append("     }\n");
            bld->append("    else\n");
            bld->appendFormat("     { __builtin_memset(&BITS(ret)[1],255,%u);\n", bytes - 1);
            bld->appendFormat("       BITS(ret)[0] = %u;\n", signbit - 1);
            bld->append("     }\n");
            bld->append("  }\n");
        } else {
            bld->appendFormat(" if (a > %d)", 255 >> ((bytes * 8) - bits));
            // we know bits > 64, and thus bytes > 1, at this point
            if (bits & 7) {
                bld->append("\n");
                bld->appendFormat("  { __builtin_memset(&BITS(ret)[1],255,%u);\n", bytes - 1);
                bld->appendFormat("    BITS(ret)[0] = %u;\n", 255 >> ((bytes * 8) - bits));
                bld->append("  }\n");
            } else {
                bld->appendFormat(" __builtin_memset(&BITS(ret)[0],255,%u);\n", bytes);
            }
        }
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

/*
 * Signed subtraction overflows exactly when the arguments have
 *  different signs and the result's sign equals the RHS's sign,
 *  implemented here as (lhs^rhs) & ~(ret^rhs) & signbit.
 *
 * Unsigned subtraction overflows exactly when the RHS is greater than
 *  the LHS.  For non-multioctet operations, this is easy to test.  We
 *  implement multioctet unsigned subtraction as unsigned addition with
 *  the RHS complemented and a carry-in of 1 into the low byte; the
 *  subtraction then overflows exactly when there is *no* carry out of
 *  the top bit.
 */
static void gen_subsat(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int bits;
    int bytes;
    int i;
    const char *suf;

    assert(wr->type == WR_SUBSAT);
    bits = wr->u.sarith.w;
    bytes = (bits + 7) >> 3;
    bld->newline();
    bld->append("static __always_inline ");
    append_type_for_width(bld, bits);
    bld->appendFormat(" subsat_%d(", bits);
    append_type_for_width(bld, bits);
    bld->append(" lhs, ");
    append_type_for_width(bld, bits);
    bld->append(" rhs");
    if (bits > 64) bld->append(" ");
    bld->append(")\n");
    bld->append("{\n");
    bld->append(" ");
    append_type_for_width(bld, bits);
    bld->append(" ret;\n");
    bld->append("\n");
    if (bits <= 64) {
        unsigned long long int max;
        max = (bits < 64) ? (1ULL << bits) - 1ULL : 0xffffffffffffffffULL;
        if (wr->u.sarith.issigned) {  // let the optimizer delete the &0x...ULL when appropriate
            bld->appendFormat(" ret = (lhs - rhs) & 0x%llxULL;\n", max);
            bld->appendFormat(
                " if ((lhs ^ rhs) & ~(ret ^ rhs) & (1ULL << %u)) ret = (ret & (1ULL << %u)) ? "
                "0x%llxULL : 0x%llxULL;\n",
                bits - 1, bits - 1, max, max & ~(max >> 1));
        } else {
            bld->append(" ret = (rhs > lhs) ? 0 : lhs - rhs;\n");
        }
    } else {
        unsigned int signbit;
        signbit = 128U >> ((bytes * 8) - bits);
        bld->append(" u16 a;\n");
        bld->append("\n");
        bld->append(" SETGUARDS(ret);\n");
        suf = "1";
        for (i = bytes - 1; i >= 0; i--) {
            bld->appendFormat(" a = BITS(lhs)[%u] + (BITS(rhs)[%u] ^ %d) + %s;\n", i, i,
                              i ? 255 : (signbit | (signbit - 1)), suf);
            suf = "((a >> 8) & 1)";
            bld->appendFormat(" BITS(ret)[%u] = a & ", i);
            if (i > 0)
                bld->append("255");
            else
                bld->appendFormat("%u", (2 * signbit) - 1);
            bld->append(";\n");
        }
        if (wr->u.sarith.issigned) {
            bld->appendFormat(" if ((BITS(lhs)[0] ^ BITS(rhs)[0]) & ~(a ^ BITS(rhs)[0]) & %u)\n",
                              signbit);
            bld->appendFormat("  { if (a & %u)\n", signbit);
            bld->appendFormat("     { __builtin_memset(&BITS(ret)[1],255,%u);\n", bytes - 1);
            bld->appendFormat("       BITS(ret)[0] = %u;\n", signbit - 1);
            bld->append("     }\n");
            bld->append("    else\n");
            bld->appendFormat("     { __builtin_memset(&BITS(ret)[1],0,%u);\n", bytes - 1);
            bld->appendFormat("       BITS(ret)[0] = %u;\n", signbit);
            bld->append("     }\n");
            bld->append("  }\n");
        } else {
            bld->appendFormat(" if (a <= %u) __builtin_memset(&BITS(ret)[0],0,%u);\n",
                              signbit | (signbit - 1), bytes);
        }
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_bitop(WrType wrt, EBPF::CodeBuilder *bld, const WidthRec *wr, const char *name,
                      const char *op) {
    unsigned int w;
    int b;
    int i;

    /* Just doing this to avoid g++ warning */
    (void)wrt;
    assert(wr->type == wrt);
    w = wr->u.arith.w;
    b = (w + 7) >> 3;
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%d %s_%d(struct internal_bit_%d lhs, struct "
        "internal_bit_%d rhs)\n",
        w, name, w, w, w);
    bld->append("{\n");
    bld->appendFormat(" struct internal_bit_%d ret;\n", w);
    bld->append("\n");
    for (i = 0; i < b; i++)
        bld->appendFormat(" ret.bits[%u] = lhs.bits[%u] %s rhs.bits[%u];\n", i, i, op, i);
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_bitand(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    gen_bitop(WR_BITAND, bld, wr, "bitand", "&");
}

static void gen_bitor(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    gen_bitop(WR_BITOR, bld, wr, "bitor", "|");
}

static void gen_bitxor(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    gen_bitop(WR_BITXOR, bld, wr, "bitxor", "^");
}

static void gen_not(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;
    int i;
    int j;

    assert(wr->type == WR_NOT);
    w = wr->u.arith.w;
    assert(w > 64);
    bld->newline();
    bld->appendFormat(
        "static __always_inline struct internal_bit_%u not_%u(struct internal_bit_%d arg)\n", w, w,
        w);
    bld->append("{\n");
    bld->append(" u16 a;\n");
    bld->append("\n");
    for (i = (w >> 3) - 1, j = 0; i >= 0; i--, j++) {
        bld->appendFormat(" ret.bits[%d] = ~arg.bits[%d];\n", j, j);
    }
    if (w & 7) {
        bld->appendFormat(" ret.bits[%d] = arg.bits[%d] ^ %d;\n", j, j, (1 << (w & 7)) - 1);
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_cast(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int fw;
    unsigned int tw;
    int fb1;
    int tb1;
    int fb2;
    int tb2;
    int minb1;

    assert(wr->type == WR_CAST);
    fw = wr->u.cast.fw;
    tw = wr->u.cast.tw;
    fb1 = fw >> 3;
    tb1 = tw >> 3;
    minb1 = (fb1 < tb1) ? fb1 : tb1;
    fb2 = (fw + 7) >> 3;
    tb2 = (tw + 7) >> 3;
    assert(((fw > 64) || (tw > 64)) && (fw != tw));
    bld->newline();
    bld->append("static __always_inline ");
    append_type_for_width(bld, tw);
    bld->appendFormat(" cast_%u_to_%u(", fw, tw);
    append_type_for_width(bld, fw);
    bld->append(" arg )\n");
    bld->append("{\n");
    if (tw > 64) {
        bld->appendFormat(" struct internal_bit_%d ret;\n", tw);
        bld->append("\n");
        bld->append(" SETGUARDS(ret);\n");
        if (fw > 64) {  // Copy as many full bytes as both widths contain
            bld->appendFormat(" __builtin_memcpy(&BITS(ret)[%d],&BITS(arg)[%d],%d);\n", tb2 - minb1,
                              fb2 - minb1, minb1);
            if (tw > fw) {  // If we are widening...
                // Handle partial source byte, if present.
                if (fb2 != fb1)
                    bld->appendFormat(" BITS(ret)[%d] = BITS(arg)[0] & %u;\n", tb2 - fb2,
                                      ~((~0U) << (fw & 7)));
                // Zero any further destination bytes.
                if (tb2 > fb2)
                    bld->appendFormat(" __builtin_memset(&BITS(ret)[0],0,%d);\n", tb2 - fb2);
            } else {  // If we are narrowing...
                // Copy trailing partial byte, if present.
                if (tb2 != tb1)
                    bld->appendFormat(" BITS(ret)[0] = BITS(arg)[%d] & %d;\n", fb2 - tb2,
                                      ~((~0U) << (tw & 7)));
            }
        } else {
            int i;
            int j;
            for (i = 0, j = tb2 - 1; i < fb1; i++, j--) {
                bld->appendFormat(" BITS(ret)[%d] = ", j);
                if (i)
                    bld->appendFormat("(arg >> %d)", i * 8);
                else
                    bld->appendFormat("arg");
                bld->appendFormat(" & 255;\n");
            }
            if (i < fb2) {
                bld->appendFormat(" BITS(ret)[%d] = ", j);
                if (i)
                    bld->appendFormat("(arg >> %d)", i * 8);
                else
                    bld->appendFormat("arg");
                bld->appendFormat(" & %u;\n", (1U << (fw & 7)) - 1);
                j--;
            }
            if (j >= 0) bld->appendFormat(" __builtin_memset(&BITS(ret)[0],0,%u);\n", j + 1);
        }
    } else {
        bld->appendFormat(" u64 ret;\n");
        bld->append("\n");
        bld->append(" SETGUARDS(ret);\n");
        // know fw>64 from assert above, since tw<=64 here
        const char *pref = " ret = ";
        if (tw & 7) {
            bld->appendFormat("%s((BITS(arg)[%d] & (u64)%u) << %d)", pref, fb2 - 1 - (tw >> 3),
                              (1U << (tw & 7)) - 1, tw & ~7U);
            pref = " |\n       ";
        }
        for (int i = (tw >> 3) - 1; i >= 0; i--) {
            bld->appendFormat("%s(((u64)BITS(arg)[%d]) << %d)", pref, fb2 - 1 - i, i << 3);
            pref = " |\n       ";
        }
        bld->append(";\n");
    }
    bld->append(" return(ret);\n");
    bld->append("}\n");
}

static void gen_cmp(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;

    assert(wr->type == WR_CMP);
    w = wr->u.cmp.w;
    bld->newline();
    bld->appendFormat(
        "static __always_inline int cmp_%s_%u_%s(internal_bit_%u l, internal_bit_%u r)\n",
        (wr->u.cmp.cmp & CMP_SIGNED) ? "s" : "u", w, cmp_string(wr->u.cmp.cmp & CMP_BASE), w, w);
    bld->append("{\n");
    switch (wr->u.cmp.cmp) {
        case CMP_EQ:
            bld->append(" return(");
            if (w & 7)
                bld->appendFormat("!((BITS(l)[%u]^BITS(r)[%u])&%u)&&", w >> 3, w >> 3,
                                  (1U << (w & 7)) - 1U);
            bld->appendFormat("!__builtin_memcmp(&BITS(l)[0],&BITS(r)[0],%u));", w >> 3);
            break;
        case CMP_NE:
            bld->append(" return(");
            if (w & 7)
                bld->appendFormat("((BITS(l)[%u]^BITS(r)[%u])&%u)||", w >> 3, w >> 3,
                                  (1U << (w & 7)) - 1U);
            bld->appendFormat("__builtin_memcmp(&BITS(l)[0],&BITS(r)[0],%u));", w >> 3);
            break;
            {
                const char *t_op;
                const char *f_op;
                const char *pref;
                const char *suff;
                const char *postop;
                unsigned int postnum;
                int eqrv;
                case CMP_LT:
                    f_op = ">";
                    t_op = "<";
                    eqrv = 0;
                    pref = "";
                    postop = "&";
                    suff = "";
                    postnum = (1U << (w & 7)) - 1U;
                    if (0) {
                        case CMP_LE:
                            f_op = ">";
                            t_op = "<";
                            eqrv = 1;
                            pref = "";
                            postop = "&";
                            suff = "";
                            postnum = (1U << (w & 7)) - 1U;
                    }
                    if (0) {
                        case CMP_GT:
                            f_op = "<";
                            t_op = ">";
                            eqrv = 0;
                            pref = "";
                            postop = "&";
                            suff = "";
                            postnum = (1U << (w & 7)) - 1U;
                    }
                    if (0) {
                        case CMP_GE:
                            f_op = "<";
                            t_op = ">";
                            eqrv = 1;
                            pref = "";
                            postop = "&";
                            suff = "";
                            postnum = (1U << (w & 7)) - 1U;
                    }
                    if (0) {
                        case CMP_LT | CMP_SIGNED:
                            f_op = ">";
                            t_op = "<";
                            eqrv = 0;
                            pref = "((i8)";
                            postop = "<<";
                            suff = ")";
                            postnum = 8 - (w & 7);
                    }
                    if (0) {
                        case CMP_LE | CMP_SIGNED:
                            f_op = ">";
                            t_op = "<";
                            eqrv = 1;
                            pref = "((i8)";
                            postop = "<<";
                            suff = ")";
                            postnum = 8 - (w & 7);
                    }
                    if (0) {
                        case CMP_GT | CMP_SIGNED:
                            f_op = "<";
                            t_op = ">";
                            eqrv = 0;
                            pref = "((i8)";
                            postop = "<<";
                            suff = ")";
                            postnum = 8 - (w & 7);
                    }
                    if (0) {
                        case CMP_GE | CMP_SIGNED:
                            f_op = "<";
                            t_op = ">";
                            eqrv = 1;
                            pref = "((i8)";
                            postop = "<<";
                            suff = ")";
                            postnum = 8 - (w & 7);
                    }
                    bld->append(" int i;\n");
                    bld->append("\n");
                    if (w & 7) {
                        bld->appendFormat(
                            " if (%s(BITS(l)[%u] %s %u)%s %s %s(BITS(r)[%u] %s %u)%s) return(1);\n",
                            pref, w >> 3, postop, postnum, suff, t_op, pref, w >> 3, postop,
                            postnum, suff);
                        bld->appendFormat(
                            " if (%s(BITS(l)[%u] %s %u)%s %s %s(BITS(r)[%u] %s %u)%s) return(0);\n",
                            pref, w >> 3, postop, postnum, suff, f_op, pref, w >> 3, postop,
                            postnum, suff);
                    }
                    bld->appendFormat(" for (i=%u;i>=0;i--)\n", (w >> 3) - 1);
                    bld->appendFormat("  { if (BITS(l)[i] %s BITS(r)[i]) return(1);\n", t_op);
                    bld->appendFormat("    if (BITS(l)[i] %s BITS(r)[i]) return(0);\n", f_op);
                    bld->append("  }\n");
                    bld->appendFormat(" return(%d);\n", eqrv);
            }
            break;
    }
    bld->append("}\n");
}

static void zero_shift_constant(EBPF::CodeBuilder *bld, unsigned int w) {
    bld->append("#if GUARDSIZE > 0\n");
    bld->appendFormat(" struct internal_bit_%u rv;\n", w);
    bld->append(" rv = v;\n");
    bld->append(" SETGUARDS(rv);\n");
    bld->append(" return(rv);\n");
    bld->append("#else\n");
    bld->append(" return(v);\n");
    bld->append("#endif\n");
}

static void gen_shl_c(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int sv;
    int o;
    int i;
    const char *pref;
    const char *suff;
    int nb;

    assert(wr->type == WR_SHL_C);
    lw = wr->u.shift_c.lw;
    sv = wr->u.shift_c.sv;
    assert((lw > 64) && (sv < lw));
    bld->newline();
    bld->appendFormat("struct internal_bit_%u shl_%u_c_%u(struct internal_bit_%u v )\n", lw, lw, sv,
                      lw);
    bld->append("{\n");
    if (sv == 0) {
        zero_shift_constant(bld, lw);
    } else {
        bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
        o = sv >> 3;
        sv &= 7;
        if (sv) bld->append(" u16 a;\n");
        bld->newline();
        bld->append(" SETGUARDS(rv);\n");
        if (o)
            bld->appendFormat(" __builtin_memset(&BITS(rv)[%u],0,%u);\n", ((lw + 7) >> 3) - o, o);
        if (sv) {
            pref = "";
            suff = "";
            // This can be optimized more in some cases.
            // "First make it work..."
            if (lw & 7) {
                i = 0;
            } else {
                i = 1;
                o++;
            }
            nb = lw >> 3;
            for (; o < nb; o++, i++) {
                bld->appendFormat(" a = %sBITS(v)[%u] << %u%s;\n", pref, nb - i, sv, suff);
                pref = "(a >> 8) | (";
                suff = ")";
                bld->appendFormat(" BITS(rv)[%u] = a & 255;\n", nb - o);
            }
            bld->appendFormat(" a = %sBITS(v)[%u] << %u%s;\n", pref, nb - i, sv, suff);
            bld->appendFormat(" BITS(rv)[%u] = a & %u;\n", nb - o,
                              (lw & 7) ? (1U << (lw & 7)) - 1U : 255);
        } else {
            if (lw & 7) {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[1],&BITS(v)[%u],%u);\n", o + 1,
                                  (lw >> 3) - o);
                bld->appendFormat(" BITS(rv)[0] = BITS(v)[%u] & %u;\n", o, (1U << (lw & 7)) - 1U);
            } else {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[0],&BITS(v)[%u],%u);\n", o,
                                  (lw >> 3) - o);
            }
        }
        bld->append(" return(rv);\n");
    }
    bld->append("}\n");
}

static void gen_shrl_c(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int sv;
    int i;
    int j;
    int s;
    int nb;

    assert(wr->type == WR_SHRL_C);
    lw = wr->u.shift_c.lw;
    sv = wr->u.shift_c.sv;
    assert(lw > 64);
    bld->newline();
    append_type_for_width(bld, lw);
    bld->appendFormat(" shrl_%u_c_%u(", lw, sv);
    append_type_for_width(bld, lw);
    bld->append(" v )\n");
    bld->append("{\n");
    if (sv == 0) {
        zero_shift_constant(bld, lw);
    } else {
        bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
        if ((sv < (lw & ~7U)) && (sv & 7)) bld->append(" u32 a;\n");
        bld->newline();
        bld->append(" SETGUARDS(rv);\n");
        if (sv >= (lw & ~7U)) {  // No bits survive from anything below the top byte of v
            bld->appendFormat(" __builtin_memset(&BITS(rv)[0],0,%u);\n", (lw - 1) >> 3);
            if (sv == (lw & ~7U)) {
                bld->appendFormat(" BITS(rv)[%u] = BITS(v)[0]", (lw - 1) >> 3);
                if (lw & 7) bld->appendFormat(" & %u", (1U << (lw & 7)) - 1);
                bld->append(";\n");
            } else if (lw & 7) {
                bld->appendFormat(" BITS(rv)[%u] = (BITS(v)[0] & %u) >> %u;\n", (lw - 1) >> 3,
                                  (1U << (lw & 7)) - 1U, sv - (lw & ~7U));
            } else {
                bld->appendFormat(" BITS(rv)[%u] = BITS(v)[0] >> %u;\n", (lw - 1) >> 3,
                                  sv - (lw & ~7U));
            }
        } else if (!(sv & 7)) {
            if (lw & 7) {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[%u],&BITS(v)[1],%u);\n",
                                  (sv >> 3) + 1, (lw - sv) >> 3);
                bld->appendFormat(" BITS(rv)[%u] = BITS(v)[0] & %u;\n", sv >> 3,
                                  (1U << (lw & 7)) - 1U);
                bld->appendFormat(" __builtin_memset(&BITS(rv)[0],0,%u);\n", sv >> 3);
            } else {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[%u],&BITS(v)[0],%u);\n", sv >> 3,
                                  (lw - sv) >> 3);
                bld->appendFormat(" __builtin_memset(&BITS(rv)[0],0,%u);\n", sv >> 3);
            }
        } else {
            bld->append(" a = BITS(v)[0]");
            if (lw & 7) bld->appendFormat(" & %u", (1U << (lw & 7)) - 1);
            bld->append(";\n");
            s = 8 + ((lw - 1) & 7) - ((lw - 1 - sv) & 7);
            j = (lw - 9) >> 3;
            i = (lw - 1 - sv) >> 3;
            nb = (lw - 1) >> 3;
            if (i < nb) bld->appendFormat(" __builtin_memset(&BITS(rv)[0],0,%u);\n", nb - i);
            for (; i > 0; i--, j--) {
                bld->appendFormat(" a = (a << 8) | BITS(v)[%u];\n", nb - j);
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & 255;\n", nb - i, s);
            }
            if (s == 8) {
                bld->appendFormat(" BITS(rv)[%u] = a & 255;\n", nb - i);
            } else if (s > 8) {
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & 255;\n", nb - i, s - 8);
            } else {
                bld->appendFormat(" a = (a << 8) | BITS(v)[%u];\n", nb - j);
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & 255;\n", nb - i, s);
            }
        }
        bld->append(" return(rv);\n");
    }
    bld->append("}\n");
}

static void gen_shra_c(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int sv;
    int i;
    int j;
    int s;
    unsigned int m;

    assert(wr->type == WR_SHRA_C);
    lw = wr->u.shift_c.lw;
    sv = wr->u.shift_c.sv;
    assert(lw > 64);
    bld->newline();
    append_type_for_width(bld, lw);
    bld->appendFormat(" shra_%u_c_%u(", lw, sv);
    append_type_for_width(bld, lw);
    bld->append(" v )\n");
    bld->append("{\n");
    if (sv == 0) {
        zero_shift_constant(bld, lw);
    } else {
        bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
        if ((sv < (lw & ~7U)) && (sv & 7)) bld->append(" u32 a;\n");
        bld->newline();
        bld->append(" SETGUARDS(rv);\n");
        if (sv >= (lw & ~7U)) {  // No bits survive from anything below the top byte of v
            if (sv == lw - 1) {  // All bits copies of sign bit
                if (lw & 7) {
                    bld->appendFormat(" __builtin_memset(&BITS(rv)[1],(BITS(v)[0]&%u)?255:0,%u);\n",
                                      1U << ((lw - 1) & 7), lw >> 3);
                    bld->appendFormat(" BITS(rv)[0] = (BITS(v)[0] & %u) ? %u : 0;\n",
                                      1U << ((lw - 1) & 7), (1U << (lw & 7)) - 1U);
                } else {
                    bld->appendFormat(" __builtin_memset(&BITS(rv)[0],(BITS(v)[0]&%u)?255:0,%u);\n",
                                      1U << ((lw - 1) & 7), lw >> 3);
                }
            } else {
                if (lw & 7) {
                    bld->appendFormat(" if (BITS(v)[0] & %u)\n", 1U << ((lw - 1) & 7));
                    bld->appendFormat("  { __builtin_memset(&BITS(rv)[1],255,%u);\n",
                                      (lw >> 3) - 1);
                    bld->appendFormat("    BITS(rv)[0] = %u;\n", (1U << (lw & 7)) - 1U);
                    bld->append("  }\n");
                    bld->append(" else\n");
                    bld->appendFormat("  { __builtin_memset(&BITS(rv)[0],0,%u);\n", lw >> 3);
                    bld->append("  }\n");
                } else {
                    bld->appendFormat(
                        " __builtin_memset(&BITS(rv)[0],(BITS(v)[0]&128)?255:0,%u);\n",
                        (lw - 1) >> 3);
                }
                bld->appendFormat(" BITS(rv)[%u] = (", (lw - 1) >> 3);
                if (sv == (lw & ~7U)) {
                    bld->appendFormat("BITS(v)[0]");
                    if (lw & 7) bld->appendFormat(" & %u", (1U << (lw & 7)) - 1);
                } else if (lw & 7) {
                    bld->appendFormat("(BITS(v)[0] & %u) >> %u", (1U << (lw & 7)) - 1U,
                                      sv - (lw & ~7U));
                } else {
                    bld->appendFormat("BITS(v)[0] >> %u", sv - (lw & ~7U));
                }
                bld->appendFormat(") | ((BITS(v)[0] & %u) ? %u : 0);\n", 1U << ((lw - 1) & 7),
                                  (255U << ((lw - (sv & 7)) & 7)) & 255U);
            }
        } else if (!(sv & 7)) {
            if (lw & 7) {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[%u],&BITS(v)[0],%u);\n", sv >> 3,
                                  (lw + 8 - sv) >> 3);
                bld->appendFormat(" BITS(rv)[%u] |= (BITS(v)[0] & %u) ? %u : 0;\n",
                                  (lw >> 3) - ((lw - sv) >> 3), 1U << ((lw - 1) & 7),
                                  (255U << (lw & 7)) & 255U);
                if ((sv >> 3) > 1)
                    bld->appendFormat(" __builtin_memset(&BITS(rv)[1],(BITS(v)[0]&%u)?255:0,%u);\n",
                                      1U << ((lw - 1) & 7), (sv >> 3) - 1);
                bld->appendFormat(" BITS(rv)[0] = (BITS(v)[0] & %u) ? %u : 0;\n",
                                  1U << ((lw - 1) & 7), (1U << (lw & 7)) - 1U);
            } else {
                bld->appendFormat(" __builtin_memcpy(&BITS(rv)[%u],&BITS(v)[0],%u);\n", sv >> 3,
                                  (lw - sv) >> 3);
                bld->appendFormat(" __builtin_memset(&BITS(rv)[0],(BITS(v)[0]&128)?255:0,%u);\n",
                                  sv >> 3);
            }
        } else {
            switch (lw & 7) {
                case 0:
                    bld->append(" a = BITS(v)[0] | ((BITS(v)[0] & 128) ? ~(u32)255 : 0);\n");
                    break;
                case 1:
                    bld->append(" a = (BITS(v)[0] & 1) ? ~(u32)0 : 0;\n");
                    break;
                default:
                    bld->appendFormat(
                        " a = (BITS(v)[0] & %u) | ((BITS(v)[0] & %u) ? ~(u32)%u : 0);\n",
                        (1U << (lw & 7)) - 1, 1U << ((lw - 1) & 7), (1U << (lw & 7)) - 1U);
                    break;
            }
            s = 8 + ((lw - 1) & 7) - ((lw - 1 - sv) & 7);
            j = (lw - 9) >> 3;
            i = (lw - 1 - sv) >> 3;
            m = (lw & 7) ? (1U << (lw & 7)) - 1U : 255;
            if (i < (int)((lw - 1) >> 3)) {
                if (lw & 7) {
                    if (((lw - 1) >> 3) - i > 1)
                        bld->appendFormat(
                            " __builtin_memset(&BITS(rv)[1],(BITS(v)[0]&%u)?255:0,%u);\n",
                            1U << ((lw - 1) & 7), ((lw - 1) >> 3) - i - 1);
                    bld->appendFormat(" BITS(rv)[0] = (BITS(v)[0] & %u) ? %u : 0;\n",
                                      1U << ((lw - 1) & 7), (1U << (lw & 7)) - 1U);
                    m = 255;
                } else {
                    bld->appendFormat(" __builtin_memset(&BITS(rv)[0],(BITS(v)[0]&%u)?255:0,%u);\n",
                                      1U << ((lw - 1) & 7), ((lw - 1) >> 3) - i);
                }
            }
            for (; i > 0; i--, j--) {
                bld->appendFormat(" a = (a << 8) | BITS(v)[%u];\n", ((lw - 1) >> 3) - j);
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & %u;\n", ((lw - 1) >> 3) - i, s, m);
                m = 255;
            }
            if (s == 8) {
                bld->appendFormat(" BITS(rv)[%u] = a & %u;\n", ((lw - 1) >> 3) - i, m);
            } else if (s > 8) {
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & %u;\n", ((lw - 1) >> 3) - i, s - 8,
                                  m);
            } else {
                bld->appendFormat(" a = (a << 8) | BITS(v)[%u];\n", ((lw - 1) >> 3) - j);
                bld->appendFormat(" BITS(rv)[%u] = (a >> %u) & %u;\n", ((lw - 1) >> 3) - i, s, m);
            }
        }
        bld->append(" return(rv);\n");
    }
    bld->append("}\n");
}

static void gen_sh_x_shvar(EBPF::CodeBuilder *bld, unsigned int lw, unsigned int rw,
                           const char *sharg, const char *shint, const char **shvarr) {
    const char *indent;
    int s_set;
    int i;

    bld->append(" do\n");
    indent = "  { ";
    if (rw <= 64) {  // We assume at least one of the next two if blocks will run;
        //  we don't care about the case where lw >= 2^64.
        if (rw < 64) {
            bld->appendFormat("%s%s &= %lluULL;\n", indent, sharg, (1ULL << rw) - 1ULL);
            indent = "    ";
        }
        if ((rw > 63) || (lw < (1ULL << rw) - 1ULL)) {
            bld->appendFormat("%sif (%s >= %u) break;\n", indent, sharg, lw);
            // indent = "    "; don't bother; it isn't used further
        }
        *shvarr = sharg;
    } else {
        if (rw & 7) {
            bld->appendFormat("%s%s = BITS(%s)[%d] & %d;\n", indent, shint, sharg, rw >> 3,
                              (1 << (rw & 7)) - 1);
            indent = "    ";
            if ((1U << (rw & 7)) - 1U >= lw)
                bld->appendFormat("    if (%s >= %u) break;\n", shint, lw);
            s_set = 1;
        } else {
            s_set = 0;
        }
        // rw > 64 here, so this loop runs at least once
        for (i = (rw - 1) >> 3; i >= 0; i--) {
            bld->appendFormat("%s%s = ", indent, shint);
            indent = "    ";
            if (s_set) bld->appendFormat("(%s << 8) | (", shint);
            bld->appendFormat("BITS(%s)[%d]%s;\n", sharg, i, s_set ? ")" : "");
            bld->appendFormat("    if (%s >= %u) break;\n", shint, lw);
            s_set = 1;
        }
        *shvarr = shint;
    }
}

static void gen_shl_x(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int rw;
    const char *shvar;

    assert(wr->type == WR_SHL_X);
    lw = wr->u.shift_x.lw;
    rw = wr->u.shift_x.rw;
    assert((lw > 64) || (rw > 64));
    bld->newline();
    append_type_for_width(bld, lw);
    bld->appendFormat(" shl_%u_x_%u(", lw, rw);
    append_type_for_width(bld, lw);
    bld->append(" v, ");
    append_type_for_width(bld, rw);
    bld->append(" sh )\n");
    bld->append("{\n");
    if (rw > 64) bld->append(" unsigned int s;\n");
    if (lw > 64) bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
    bld->append(" unsigned int i;\n");
    bld->append(" u16 a;\n");
    bld->append(" unsigned int left;\n");
    bld->append(" unsigned int o;\n");
    bld->newline();
    bld->append(" SETGUARDS(rv);\n");
    gen_sh_x_shvar(bld, lw, rw, "sh", "s", &shvar);
    // the value in the variable named by shvar is < lw, now
    // (but we can't assert() that; it's a packet-processing-time thing)
    if (lw < 64) {
        bld->appendFormat("    return((v<<%s)&%llu);\n", shvar, (1ULL << lw) - 1ULL);
    } else if (lw == 64) {
        bld->appendFormat("    return(v<<%s);\n", shvar);
    } else {
        bld->appendFormat("    if (%s == 0) return(COPYGUARDS(v,rv));\n", shvar);
        if (rw > 3) {
            bld->appendFormat("    for (o=%u,left=%u;%s>=8;o--,%s-=8,left-=8) BITS(rv)[o] = 0;\n",
                              (lw - 1) >> 3, lw, shvar, shvar);
            bld->append("    a = 0;\n");
            bld->appendFormat("    for (i=%u;left>=8;left-=8)\n", (lw - 1) >> 3);
        } else {
            bld->appendFormat("    a = 0;\n");
            bld->appendFormat("    for (i=o=%u,left=%u;left>=8;left-=8)\n", (lw - 1) >> 3, lw);
        }
        bld->appendFormat("     { a = (a >> 8) | (BITS(v)[i--] << %s);\n", shvar);
        bld->append("       BITS(rv)[o--] = a & 255;\n");
        bld->append("     }\n");
        bld->append("    if (left)\n");
        bld->appendFormat(
            "     { if (left > %s) a = (a >> 8) | (BITS(v)[i--] << %s); else a >>= 8;\n", shvar,
            shvar);
        bld->append("       BITS(rv)[o--] = a & ((1U << left) - 1);\n");
        bld->append("     }\n");
        bld->append("    return(rv);\n");
    }
    bld->append("  } while (0);\n");
    bld->append(" return(");
    if (lw <= 64) {
        bld->append("0");
    } else {
        bld->appendFormat("CONSTGUARDS((struct internal_bit_%u){{0}},rv)", lw);
    }
    bld->append(");\n");
    bld->append("}\n");
}

static void gen_shrl_x(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int rw;
    const char *shvar;

    assert(wr->type == WR_SHRL_X);
    lw = wr->u.shift_x.lw;
    rw = wr->u.shift_x.rw;
    assert((lw > 64) || (rw > 64));
    bld->newline();
    append_type_for_width(bld, lw);
    bld->appendFormat(" shrl_%u_x_%u(", lw, rw);
    append_type_for_width(bld, lw);
    bld->append(" v, ");
    append_type_for_width(bld, rw);
    bld->append(" sh )\n");
    bld->append("{\n");
    if (rw > 64) bld->append(" unsigned int s;\n");
    if (lw > 64) bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
    bld->append(" u16 a;\n");
    bld->append(" int i;\n");
    bld->append(" int o;\n");
    bld->newline();
    bld->append(" SETGUARDS(rv);\n");
    gen_sh_x_shvar(bld, lw, rw, "sh", "s", &shvar);
    // the value in the variable named by shvar is < lw, now
    // (but we can't assert() that; it's a packet-processing-time thing)
    if (lw <= 64) {
        bld->appendFormat("    return(v>>%s);\n", shvar);
    } else {
        bld->appendFormat("    if (%s == 0) return(COPYGUARDS(v,rv));\n", shvar);
        bld->appendFormat("    i = %u - (%s >> 3);\n", (lw - 1) >> 3, shvar);
        bld->appendFormat("    %s = 8 - (%s & 7ULL);\n", shvar, shvar);
        bld->appendFormat("    a = BITS(v)[i--] << %s;\n", shvar);
        bld->appendFormat("    o = %u;\n", (lw - 1) >> 3);
        bld->append("    for (;i>=0;i--)\n");
        bld->appendFormat("     { a = (a >> 8) | (BITS(v)[i] << %s);\n", shvar);
        bld->appendFormat("       BITS(rv)[o--] = a & 255;\n");
        bld->append("     }\n");
        if (lw & 7) bld->appendFormat("    a &= %u << %s;\n", (1U << (lw & 7)) - 1U, shvar);
        bld->append("    BITS(rv)[o--] = a >> 8;\n");
        bld->append("    for (;o>=0;o--) BITS(rv)[o] = 0;\n");
        bld->append("    return(rv);\n");
    }
    bld->append("  } while (0);\n");
    bld->append(" return(");
    if (lw <= 64) {
        bld->append("0");
    } else {
        bld->appendFormat("CONSTGUARDS((struct internal_bit_%u){{0}},rv)", lw);
    }
    bld->append(");\n");
    bld->append("}\n");
}

static void gen_shra_x(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int lw;
    unsigned int rw;
    const char *shvar;
    int i;

    assert(wr->type == WR_SHRA_X);
    lw = wr->u.shift_x.lw;
    rw = wr->u.shift_x.rw;
    assert((lw > 64) || (rw > 64));
    bld->newline();
    append_type_for_width(bld, lw);
    bld->appendFormat(" shra_%u_x_%u(", lw, rw);
    append_type_for_width(bld, lw);
    bld->append(" v, ");
    append_type_for_width(bld, rw);
    bld->append(" sh )\n");
    bld->append("{\n");
    if (rw > 64) bld->append(" unsigned int s;\n");
    if (lw > 64) bld->appendFormat(" struct internal_bit_%u rv;\n", lw);
    bld->append(" u16 a;\n");
    bld->append(" int i;\n");
    bld->append(" int o;\n");
    bld->append(" u8 sign;\n");
    bld->newline();
    bld->append(" SETGUARDS(rv);\n");
    gen_sh_x_shvar(bld, lw, rw, "sh", "s", &shvar);
    // the value in the variable named by shvar is < lw, now
    // (but we can't assert() that; it's a packet-processing-time thing)
    if (lw <= 64) {
        bld->appendFormat("    return((v>>%s)|((v&%lluULL)?(((1ULL<<%s)-1ULL)<<(%u-%s)):0));\n",
                          shvar, 1ULL << (lw - 1), shvar, lw, shvar);
    } else {
        bld->appendFormat("    if (! %s) return(COPYGUARDS(v,rv));\n", shvar);
        bld->appendFormat("    sign = (BITS(v)[0] & %u) ? 255 : 0;\n", 1U << ((lw - 1) & 7));
        bld->appendFormat("    i = %s >> 3;\n", shvar);
        bld->appendFormat("    %s = 8 - (%s & 7ULL);\n", shvar, shvar);
        if (lw & 7) {
            bld->appendFormat(
                "    a = ((i == %u) ? (BITS(v)[0] & %u) | ((sign << %u) & 255) : BITS(v)[%u-i]) << "
                "%s;\n",
                lw >> 3, (1U << (lw & 7)) - 1U, lw & 7, lw >> 3, shvar);
            bld->append("    i ++;\n");
            bld->appendFormat("    o = %u;\n", lw >> 3);
            bld->appendFormat("    for (i=%u-i;i>0;i--)\n", lw >> 3);
            bld->appendFormat("     { a = (a >> 8) | (BITS(v)[i] << %s);\n", shvar);
            bld->appendFormat("       BITS(rv)[o--] = a & 255;\n");
            bld->append("     }\n");
            bld->append("    if (i == 0)\n");
            bld->appendFormat(
                "     { a = (a >> 8) | ((BITS(v)[0] & %u) << %s) | (sign << (%s + %u));\n",
                (1U << (lw & 7)) - 1U, shvar, shvar, lw & 7);
            bld->append("       BITS(rv)[o--] = a & 255;\n");
            bld->append("     }\n");
            bld->appendFormat("    a |= sign << (8 + %s);\n", shvar);
            bld->append("    if (o == 0)\n");
            bld->appendFormat("     { BITS(rv)[o--] = (a >> 8) & %u;\n", (1U << (lw & 7)) - 1U);
            bld->append("     }\n");
            bld->append("    else\n");
            bld->append("     { BITS(rv)[o--] = (a >> 8) & 255;\n");
            bld->appendFormat("       for (;o>0;o--) BITS(rv)[o] = sign;\n");
            bld->appendFormat("       BITS(rv)[0] = sign & %u;\n", (1U << (lw & 7)) - 1U);
            bld->append("     }\n");
        } else {
            bld->appendFormat("    i = %u - i;\n", (lw - 1) >> 3);
            bld->appendFormat("    a = BITS(v)[i--] << %s;\n", shvar);
            bld->appendFormat("    o = %u;\n", (lw - 1) >> 3);
            bld->append("    for (;i>=0;i--)\n");
            bld->appendFormat("     { a = (a >> 8) | (BITS(v)[i] << %s);\n", shvar);
            bld->appendFormat("       BITS(rv)[o--] = a & 255;\n");
            bld->append("     }\n");
            bld->append("    if (o >= 0)\n");
            bld->appendFormat("     { a = (a >> 8) | (sign << %s);\n", shvar);
            bld->appendFormat("       BITS(rv)[o--] = a & 255;\n");
            bld->append("     }\n");
            bld->append("    for (;o>=0;o--) BITS(rv)[o] = sign;\n");
        }
        bld->append("    return(rv);\n");
    }
    bld->append("  } while (0);\n");
    bld->append(" return(");
    if (lw <= 64) {
        bld->appendFormat("(v&%lluULL)?%lluULL:0", 1ULL << (lw - 1),
                          ((1ULL << (lw - 1)) << 1) | 1ULL);
    } else {
        bld->appendFormat("CONSTGUARDS(((BITS(v)[0]&%u)?(struct internal_bit_%u){{",
                          1U << ((lw - 1) & 7), lw);
        bld->appendFormat("%u", (1U << (((lw - 1) & 7) + 1)) - 1);
        for (i = (lw - 1) >> 3; i > 0; i--) bld->append(",255");
        bld->appendFormat("}}:(struct internal_bit_%u){{0}}", lw);
        bld->append("),rv)");
    }
    bld->append(");\n");
    bld->append("}\n");
}

/*
 * The reason we have assign_* functions at all is that it's the
 *  easiest way to get the RHS into an addressible temporary, so we can
 *  memcpy out of it.  If we were to try to generate the code inline by
 *  hand, we'd have trouble because the RHS may not have an address
 *  (from a C perspective, that is; it usually will be stored in memory
 *  and thus will have an address at the machine-language level) and
 *  thus may not be a suitable argument to unary &.  Making this a
 *  function forces the compiler to create a temporary as necessary.
 */
static void gen_assign(EBPF::CodeBuilder *bld, const WidthRec *wr) {
    unsigned int w;

    assert(wr->type == WR_ASSIGN);
    w = wr->u.assign.w;
    bld->newline();
    bld->appendFormat("static __always_inline void assign_%u(u8 *lhs, ", w);
    append_type_for_width(bld, w);
    bld->append(" rhs)\n");
    bld->append("{\n");
    bld->appendFormat(" __builtin_memcpy(lhs,&rhs.bits[0],%u);\n", (w + 7) >> 3);
    bld->append("}\n");
}

void ScanWidths::gen_h(EBPF::CodeBuilder *bld) const {
    int i;

    bld->appendLine("#define BITS(v) (v).bits");
    bld->appendLine("#define SETGUARDS(x) do ; while (0)");
    for (i = 0; i < nwr; i++) {
        switch (wrv[i].type) {
            case WR_VALUE:
                bld->newline();
                bld->appendFormat("struct internal_bit_%d {\n", wrv[i].u.value);
                bld->appendFormat("  u8 bits[%d];\n", (wrv[i].u.value + 7) >> 3);
                bld->append("};\n");
                bld->newline();
                bld->appendFormat(
                    "static __always_inline struct internal_bit_%u loadfrom_%u(u8 *in)\n",
                    wrv[i].u.value, wrv[i].u.value);
                bld->append("{\n");
                bld->appendFormat(" struct internal_bit_%d rv;\n", wrv[i].u.value);
                bld->append("\n");
                bld->appendFormat(" __builtin_memcpy(&rv.bits[0],in,%d);\n",
                                  (wrv[i].u.value + 7) >> 3);
                if (wrv[i].u.value & 7)
                    bld->appendFormat(" rv.bits[%d] &= %d;\n", wrv[i].u.value >> 3,
                                      (1U << (wrv[i].u.value & 7)) - 1);
                bld->append(" return(rv);\n");
                bld->append("}\n");
                break;
            case WR_CONCAT:
            case WR_ADD:
            case WR_SUB:
            case WR_MUL:
            case WR_BXSMUL:
            case WR_CAST:
            case WR_NEG:
            case WR_NOT:
            case WR_SHL_X:
            case WR_SHL_C:
            case WR_SHRA_X:
            case WR_SHRA_C:
            case WR_SHRL_X:
            case WR_SHRL_C:
            case WR_CMP:
            case WR_BITAND:
            case WR_BITOR:
            case WR_BITXOR:
            case WR_ADDSAT:
            case WR_SUBSAT:
            case WR_ASSIGN:
                break;
            default:
                abort();
                break;
        }
    }
    bld->newline();
    for (i = 0; i < nwr; i++) {
        if (wrv[i].type != WR_VALUE) {
            bld->append(
                "/*\n"
                " * These are here because EBPF quasi-C does not support aggregate\n"
                " * return - but this is a code-generation thing, not a language thing,\n"
                " * so if we force them to be inlined, it works fine. It's easier to\n"
                " * generate them here than into the .c file, plus this way they're\n"
                " * available to everything that includes this file.\n"
                " */\n");
            break;
        }
    }
    for (i = 0; i < nwr; i++) {
        switch (wrv[i].type) {
            case WR_VALUE:
                break;
            case WR_CONCAT:
                gen_concat(bld, &wrv[i]);
                break;
            case WR_ADD:
                gen_add(bld, &wrv[i]);
                break;
            case WR_SUB:
                gen_sub(bld, &wrv[i]);
                break;
            case WR_MUL:
                gen_mul(bld, &wrv[i]);
                break;
            case WR_BXSMUL:
                gen_bxsmul(bld, &wrv[i]);
                break;
            case WR_CAST:
                gen_cast(bld, &wrv[i]);
                break;
            case WR_NEG:
                gen_neg(bld, &wrv[i]);
                break;
            case WR_NOT:
                gen_not(bld, &wrv[i]);
                break;
            case WR_SHL_X:
                gen_shl_x(bld, &wrv[i]);
                break;
            case WR_SHL_C:
                gen_shl_c(bld, &wrv[i]);
                break;
            case WR_SHRA_X:
                gen_shra_x(bld, &wrv[i]);
                break;
            case WR_SHRA_C:
                gen_shra_c(bld, &wrv[i]);
                break;
            case WR_SHRL_X:
                gen_shrl_x(bld, &wrv[i]);
                break;
            case WR_SHRL_C:
                gen_shrl_c(bld, &wrv[i]);
                break;
            case WR_CMP:
                gen_cmp(bld, &wrv[i]);
                break;
            case WR_BITAND:
                gen_bitand(bld, &wrv[i]);
                break;
            case WR_BITOR:
                gen_bitor(bld, &wrv[i]);
                break;
            case WR_BITXOR:
                gen_bitxor(bld, &wrv[i]);
                break;
            case WR_ADDSAT:
                gen_addsat(bld, &wrv[i]);
                break;
            case WR_SUBSAT:
                gen_subsat(bld, &wrv[i]);
                break;
            case WR_ASSIGN:
                gen_assign(bld, &wrv[i]);
                break;
            default:
                abort();
                break;
        }
    }
}

void ScanWidths::gen_c(EBPF::CodeBuilder *bld) const {
    (void)bld;  // empty at the moment
}

bool Backend::process() {
    CHECK_NULL(toplevel);
    if (toplevel->getMain() == nullptr) {
        ::P4::error("main is missing in the package");
        return false;  //  no main
    }
    auto refMapEBPF = refMap;
    auto typeMapEBPF = typeMap;
    auto hook = options.getDebugHook();
    target = new EBPF::P4TCTarget(options.emitTraceMessages);
    ScanWidths *sw = new ScanWidths(typeMap, refMap, target);
    parseTCAnno = new ParseTCAnnotations();
    tcIR = new ConvertToBackendIR(toplevel, pipeline, refMap, typeMap, options);
    genIJ = new IntrospectionGenerator(pipeline, refMap, typeMap);
    EBPF::EBPFTypeFactory::createFactory(typeMapEBPF, true);
    PassManager backEnd = {};
    backEnd.addPasses({parseTCAnno, new P4::ClearTypeMap(typeMap),
                       new P4::TypeChecking(refMap, typeMap, true), sw, tcIR, genIJ});
    widths = sw;
    backEnd.addDebugHook(hook, true);

    auto newpgm = toplevel->getProgram()->apply(backEnd);
    if (::P4::errorCount() > 0) return false;
    if (!ebpfCodeGen(refMapEBPF, typeMapEBPF, newpgm)) return false;
    return true;
}

bool Backend::ebpfCodeGen(P4::ReferenceMap *refMapEBPF, P4::TypeMap *typeMapEBPF,
                          const IR::P4Program *prog) {
    ebpfOption.xdp2tcMode = options.xdp2tcMode;
    ebpfOption.exe_name = options.exe_name;
    ebpfOption.file = options.file;
    PnaProgramStructure structure(refMapEBPF, typeMapEBPF);
    auto parsePnaArch = new ParsePnaArchitecture(&structure);
    auto main = toplevel->getMain();
    if (!main) return false;

    if (main->type->name != "PNA_NIC") {
        ::P4::warning(ErrorType::WARN_INVALID,
                      "%1%: the main package should be called PNA_NIC"
                      "; are you using the wrong architecture?",
                      main->type->name);
        return false;
    }

    main->apply(*parsePnaArch);
    auto evaluator = new P4::EvaluatorPass(refMapEBPF, typeMapEBPF);
    auto program = prog;

    PassManager rewriteToEBPF = {
        evaluator,
        new VisitFunctor([this, evaluator, structure]() { top = evaluator->getToplevelBlock(); }),
    };

    auto hook = options.getDebugHook();
    rewriteToEBPF.addDebugHook(hook, true);
    program = program->apply(rewriteToEBPF);

    // map IR node to compile-time allocated resource blocks.
    top->apply(*new P4::BuildResourceMap(&structure.resourceMap));

    main = top->getMain();
    if (!main) return false;  // no main
    main->apply(*parsePnaArch);
    program = top->getProgram();

    auto convertToEbpf = new ConvertToEbpfPNA(ebpfOption, refMapEBPF, typeMapEBPF, tcIR);
    PassManager toEBPF = {
        new P4::DiscoverStructure(&structure),
        new InspectPnaProgram(refMapEBPF, typeMapEBPF, &structure),
        // convert to EBPF objects
        new VisitFunctor([evaluator, convertToEbpf]() {
            auto tlb = evaluator->getToplevelBlock();
            tlb->apply(*convertToEbpf);
        }),
    };

    toEBPF.addDebugHook(hook, true);
    program = program->apply(toEBPF);

    ebpf_program = convertToEbpf->getEBPFProgram();

    return true;
}

void Backend::serialize() const {
    std::string progName = tcIR->getPipelineName().string();
    if (ebpf_program == nullptr) return;
    EBPF::CodeBuilder c(target), p(target), h(target);
    ebpf_program->emit(&c);
    ebpf_program->emitParser(&p);
    auto pgm = ebpf_program->checkedTo<const PNAArchTC>();
    pgm->emitHeaderIncludes(&h);
    if (widths) {
        widths->gen_h(&h);
        widths->gen_c(&c);
    }
    pgm->emitHeaderDefs(&h);
    if (::P4::errorCount() > 0) {
        return;
    }
    std::filesystem::path outputFile = options.outputFolder / (progName + ".template");

    if (auto outstream = openFile(outputFile, false)) {
        *outstream << pipeline->toString();
        outstream->flush();
        std::filesystem::permissions(outputFile.c_str(),
                                     std::filesystem::perms::owner_all |
                                         std::filesystem::perms::group_all |
                                         std::filesystem::perms::others_all,
                                     std::filesystem::perm_options::add);
    }
    std::filesystem::path parserFile = options.outputFolder / (progName + "_parser.c");
    std::filesystem::path postParserFile = options.outputFolder / (progName + "_control_blocks.c");
    std::filesystem::path headerFile = options.outputFolder / (progName + "_parser.h");

    auto cstream = openFile(postParserFile, false);
    auto pstream = openFile(parserFile, false);
    auto hstream = openFile(headerFile, false);
    if (cstream == nullptr) {
        ::P4::error("Unable to open File %1%", postParserFile);
        return;
    }
    if (pstream == nullptr) {
        ::P4::error("Unable to open File %1%", parserFile);
        return;
    }
    if (hstream == nullptr) {
        ::P4::error("Unable to open File %1%", headerFile);
        return;
    }
    *cstream << c.toString();
    *pstream << p.toString();
    *hstream << h.toString();
    cstream->flush();
    pstream->flush();
    hstream->flush();
}

bool Backend::serializeIntrospectionJson(std::ostream &out) const {
    if (genIJ->serializeIntrospectionJson(out)) {
        out.flush();
        return true;
    }
    return false;
}

void ConvertToBackendIR::setPipelineName() {
    if (options.file.empty()) {
        ::P4::error("filename is not given in command line option");
        return;
    }

    pipelineName = cstring(options.file.stem());
}

bool ConvertToBackendIR::preorder(const IR::P4Program *p) {
    if (p != nullptr) {
        setPipelineName();
        return true;
    }
    return false;
}

cstring ConvertToBackendIR::externalName(const IR::IDeclaration *declaration) const {
    cstring name = declaration->externalName();
    if (name.startsWith(".")) name = name.substr(1);
    auto Name = name.replace('.', '/');
    return Name;
}

bool ConvertToBackendIR::isDuplicateAction(const IR::P4Action *action) {
    auto actionName = externalName(action);
    if (actions.find(actionName) != actions.end()) return true;
    return false;
}

void ConvertToBackendIR::postorder(const IR::P4Action *action) {
    if (action != nullptr) {
        if (isDuplicateAction(action)) return;
        auto actionName = externalName(action);
        if (actionName == P4::P4CoreLibrary::instance().noAction.name) {
            tcPipeline->addNoActionDefinition(new IR::TCAction("NoAction"_cs));
            actions.emplace("NoAction"_cs, action);
            return;
        }
        actions.emplace(actionName, action);
        actionCount++;
        unsigned int actionId = actionCount;
        IR::TCAction *tcAction = new IR::TCAction(actionName);
        tcAction->setPipelineName(pipelineName);
        tcAction->setActionId(actionId);
        actionIDList.emplace(actionId, actionName);
        auto paramList = action->getParameters();
        if (paramList != nullptr && !paramList->empty()) {
            for (auto param : paramList->parameters) {
                auto paramType = typeMap->getType(param);
                IR::TCActionParam *tcActionParam = new IR::TCActionParam();
                tcActionParam->setParamName(param->name.originalName);
                if (!paramType->is<IR::Type_Bits>()) {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1% parameter with type other than bit is not supported", param);
                    return;
                } else {
                    auto paramTypeName = paramType->to<IR::Type_Bits>()->baseName();
                    if (paramTypeName != "bit") {
                        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                    "%1% parameter with type other than bit is not supported",
                                    param);
                        return;
                    }
                    tcActionParam->setDataType(TC::BIT_TYPE);
                    unsigned int width = paramType->to<IR::Type_Bits>()->width_bits();
                    tcActionParam->setBitSize(width);
                }
                if (const auto *anno = param->getAnnotation(ParseTCAnnotations::tcType)) {
                    auto expr = anno->getExpr(0);
                    if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
                        auto val = getTcType(typeLiteral);
                        if (val != TC::BIT_TYPE) {
                            tcActionParam->setDataType(val);
                        } else {
                            ::P4::error(ErrorType::ERR_INVALID,
                                        "tc_type annotation cannot have '%1%' as value", expr);
                        }
                    } else {
                        ::P4::error(ErrorType::ERR_INVALID,
                                    "tc_type annotation cannot have '%1%' as value", expr);
                    }
                }
                auto direction = param->direction;
                if (direction == IR::Direction::InOut) {
                    tcActionParam->setDirection(TC::INOUT);
                } else if (direction == IR::Direction::In) {
                    tcActionParam->setDirection(TC::IN);
                } else if (direction == IR::Direction::Out) {
                    tcActionParam->setDirection(TC::OUT);
                } else {
                    tcActionParam->setDirection(TC::NONE);
                }
                tcAction->addActionParams(tcActionParam);
            }
        }
        tcPipeline->addActionDefinition(tcAction);
    }
}

void ConvertToBackendIR::updateTimerProfiles(IR::TCTable *tabledef) {
    if (options.timerProfiles > DEFAULT_TIMER_PROFILES) {
        tabledef->addTimerProfiles(options.timerProfiles);
    }
}
void ConvertToBackendIR::updateConstEntries(const IR::P4Table *t, IR::TCTable *tabledef) {
    // Check if there are const entries.
    auto entriesList = t->getEntries();
    if (entriesList == nullptr) return;
    auto keys = t->getKey();
    if (keys == nullptr) {
        return;
    }
    for (auto e : entriesList->entries) {
        auto keyset = e->getKeys();
        if (keyset->components.size() != keys->keyElements.size()) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "No of keys in const_entries should be same as no of keys in the table.");
            return;
        }
        ordered_map<cstring, cstring> keyList;
        for (size_t itr = 0; itr < keyset->components.size(); itr++) {
            auto keyElement = keys->keyElements.at(itr);
            auto keyString = keyElement->expression->toString();
            if (auto anno = keyElement->getAnnotation(IR::Annotation::nameAnnotation)) {
                keyString = anno->getExpr(0)->to<IR::StringLiteral>()->value;
            }
            auto keySetElement = keyset->components.at(itr);
            auto key = keySetElement->toString();
            if (keySetElement->is<IR::DefaultExpression>()) {
                key = "default"_cs;
            } else if (keySetElement->is<IR::Constant>()) {
                big_int kValue = keySetElement->to<IR::Constant>()->value;
                int kBase = keySetElement->to<IR::Constant>()->base;
                std::stringstream value;
                switch (kBase) {
                    case 2:
                        value << "0b";
                        break;
                    case 8:
                        value << "0o";
                        break;
                    case 16:
                        value << "0x";
                        break;
                    case 10:
                        break;
                    default:
                        BUG("Unexpected base %1%", kBase);
                }
                std::deque<char> buf;
                do {
                    const int digit = static_cast<int>(static_cast<big_int>(kValue % kBase));
                    kValue = kValue / kBase;
                    buf.push_front(Util::DigitToChar(digit));
                } while (kValue > 0);
                for (auto ch : buf) value << ch;
                key = value.str();
            } else if (keySetElement->is<IR::Range>()) {
                auto left = keySetElement->to<IR::Range>()->left;
                auto right = keySetElement->to<IR::Range>()->right;
                auto operand = keySetElement->to<IR::Range>()->getStringOp();
                key = left->toString() + operand + right->toString();
            } else if (keySetElement->is<IR::Mask>()) {
                auto left = keySetElement->to<IR::Mask>()->left;
                auto right = keySetElement->to<IR::Mask>()->right;
                auto operand = keySetElement->to<IR::Mask>()->getStringOp();
                key = left->toString() + operand + right->toString();
            }
            keyList.emplace(keyString, key);
        }
        cstring actionName;
        if (const auto *path = e->action->to<IR::PathExpression>())
            actionName = path->toString();
        else if (const auto *mce = e->action->to<IR::MethodCallExpression>())
            actionName = mce->method->toString();
        else
            BUG("Unexpected entry action type.");
        IR::TCEntry *constEntry = new IR::TCEntry(actionName, keyList);
        tabledef->addConstEntries(constEntry);
    }
}

void ConvertToBackendIR::updateDefaultMissAction(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto defaultAction = t->getDefaultAction();
    if (defaultAction == nullptr || !defaultAction->is<IR::MethodCallExpression>()) return;
    auto methodexp = defaultAction->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(methodexp, refMap, typeMap);
    auto actionCall = mi->to<P4::ActionCall>();
    if (actionCall == nullptr) return;
    auto actionName = externalName(actionCall->action);
    if (actionName != P4::P4CoreLibrary::instance().noAction.name) {
        for (auto tcAction : tcPipeline->actionDefs) {
            if (actionName == tcAction->actionName) {
                tabledef->setDefaultMissAction(tcAction);
                auto defaultActionProperty =
                    t->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
                if (defaultActionProperty->isConstant) {
                    tabledef->setDefaultMissConst(true);
                }
                bool isTCMayOverrideMiss = false;
                const IR::Annotation *overrideAnno =
                    defaultActionProperty->getAnnotation(ParseTCAnnotations::tcMayOverride);
                if (overrideAnno) {
                    isTCMayOverrideMiss = true;
                }
                bool directionParamPresent = false;
                auto paramList = actionCall->action->getParameters();
                for (auto param : paramList->parameters) {
                    if (param->direction != IR::Direction::None) directionParamPresent = true;
                }
                if (!directionParamPresent) {
                    auto i = 0;
                    if (isTCMayOverrideMiss) {
                        if (paramList->parameters.empty())
                            ::P4::warning(
                                ErrorType::WARN_INVALID,
                                "%1% annotation cannot be used with default_action without "
                                "parameters",
                                overrideAnno);
                        else
                            tabledef->setTcMayOverrideMiss();
                    }
                    for (auto param : paramList->parameters) {
                        auto defaultParam = new IR::TCDefaultActionParam();
                        for (auto actionParam : tcAction->actionParams) {
                            if (actionParam->paramName == param->name.originalName) {
                                defaultParam->setParamDetail(actionParam);
                            }
                        }
                        auto defaultArg = methodexp->arguments->at(i++);
                        if (auto constVal = defaultArg->expression->to<IR::Constant>()) {
                            if (!isTCMayOverrideMiss)
                                defaultParam->setDefaultValue(
                                    Util::toString(constVal->value, 0, true, constVal->base));
                            tabledef->defaultMissActionParams.push_back(defaultParam);
                        }
                    }
                } else {
                    if (isTCMayOverrideMiss)
                        ::P4::warning(ErrorType::WARN_INVALID,
                                      "%1% annotation cannot be used with default_action with "
                                      "directional parameters",
                                      overrideAnno);
                }
            }
        }
    }
}

void ConvertToBackendIR::updateDefaultHitAction(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto actionlist = t->getActionList();
    if (actionlist != nullptr) {
        unsigned int defaultHit = 0;
        unsigned int defaultHitConst = 0;
        cstring defaultActionName = nullptr;
        bool isTcMayOverrideHitAction = false;
        for (auto action : actionlist->actionList) {
            bool isTableOnly = false;
            bool isDefaultHit = false;
            bool isDefaultHitConst = false;
            bool isTcMayOverrideHit = false;
            for (const auto *anno : action->getAnnotations()) {
                if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                    isTableOnly = true;
                } else if (anno->name == ParseTCAnnotations::defaultHit) {
                    isDefaultHit = true;
                    defaultHit++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                } else if (anno->name == ParseTCAnnotations::defaultHitConst) {
                    isDefaultHitConst = true;
                    defaultHitConst++;
                    auto adecl = refMap->getDeclaration(action->getPath(), true);
                    defaultActionName = externalName(adecl);
                } else if (anno->name == ParseTCAnnotations::tcMayOverride) {
                    isTcMayOverrideHit = true;
                }
            }
            if (isTableOnly && isDefaultHit && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly', '@default_hit' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHit) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly' and '@default_hit'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTableOnly && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@tableonly' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isDefaultHit && isDefaultHitConst) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Table '%1%' has an action reference '%2%' which is "
                            "annotated with '@default_hit' and '@default_hit_const'",
                            t->name.originalName, action->getName().originalName);
                break;
            } else if (isTcMayOverrideHit) {
                auto adecl = refMap->getDeclaration(action->getPath(), true);
                auto p4Action = adecl->getNode()->checkedTo<IR::P4Action>();
                if (!isDefaultHit && !isDefaultHitConst) {
                    ::P4::warning(ErrorType::WARN_INVALID,
                                  "Table '%1%' has an action reference '%2%' which is "
                                  "annotated with '@tc_may_override' without '@default_hit' or "
                                  "'@default_hit_const'",
                                  t->name.originalName, action->getName().originalName);
                    isTcMayOverrideHit = false;
                    break;
                } else if (p4Action->getParameters()->parameters.empty()) {
                    ::P4::warning(ErrorType::WARN_INVALID,
                                  " '@tc_may_override' cannot be used for %1%  action "
                                  " without parameters",
                                  action->getName().originalName);
                    isTcMayOverrideHit = false;
                    break;
                }
                isTcMayOverrideHitAction = true;
            }
        }
        if (::P4::errorCount() > 0) {
            return;
        }
        if ((defaultHit > 0) && (defaultHitConst > 0)) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Table '%1%' cannot have both '@default_hit' action "
                        "and '@default_hit_const' action",
                        t->name.originalName);
            return;
        } else if (defaultHit > 1) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Table '%1%' can have only one '@default_hit' action",
                        t->name.originalName);
            return;
        } else if (defaultHitConst > 1) {
            ::P4::error(ErrorType::ERR_INVALID,
                        "Table '%1%' can have only one '@default_hit_const' action",
                        t->name.originalName);
            return;
        }
        if (defaultActionName != nullptr &&
            defaultActionName != P4::P4CoreLibrary::instance().noAction.name) {
            for (auto tcAction : tcPipeline->actionDefs) {
                if (defaultActionName == tcAction->actionName) {
                    tabledef->setDefaultHitAction(tcAction);
                    if (defaultHitConst == 1) {
                        tabledef->setDefaultHitConst(true);
                    }
                    if (isTcMayOverrideHitAction) {
                        if (!checkParameterDirection(tcAction)) {
                            tabledef->setTcMayOverrideHit();
                            for (auto param : tcAction->actionParams) {
                                auto defaultParam = new IR::TCDefaultActionParam();
                                defaultParam->setParamDetail(param);
                                tabledef->defaultHitActionParams.push_back(defaultParam);
                            }
                        }
                    }
                }
            }
        }
    }
}

void ConvertToBackendIR::updatePnaDirectCounter(const IR::P4Table *t, IR::TCTable *tabledef,
                                                unsigned tentries) {
    cstring propertyName = "pna_direct_counter"_cs;
    auto property = t->properties->getProperty(propertyName);
    if (property == nullptr) return;
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    auto ctrl = findContext<IR::P4Control>();
    auto cName = ctrl->name.originalName;
    auto externInstanceName = cName + "." + expr->toString();
    tabledef->setDirectCounter(externInstanceName);

    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap);
    if (!externInstance) {
        ::P4::error(ErrorType::ERR_INVALID,
                    "Expected %1% property value for table %2% to resolve to an "
                    "extern instance: %3%",
                    propertyName, t->name.originalName, property);
        return;
    }
    for (auto ext : externsInfo) {
        if (ext.first == externInstance->type->toString()) {
            auto externDefinition = tcPipeline->getExternDefinition(ext.first);
            if (externDefinition) {
                auto extInstDef = ((IR::TCExternInstance *)externDefinition->getExternInstance(
                    externInstanceName));
                extInstDef->setExternTableBindable(true);
                extInstDef->setNumElements(tentries);
                break;
            }
        }
    }
}

void ConvertToBackendIR::updatePnaDirectMeter(const IR::P4Table *t, IR::TCTable *tabledef,
                                              unsigned tentries) {
    cstring propertyName = "pna_direct_meter"_cs;
    auto property = t->properties->getProperty(propertyName);
    if (property == nullptr) return;
    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    auto ctrl = findContext<IR::P4Control>();
    auto cName = ctrl->name.originalName;
    auto externInstanceName = cName + "." + expr->toString();
    tabledef->setDirectMeter(externInstanceName);

    auto externInstance = P4::ExternInstance::resolve(expr, refMap, typeMap);
    if (!externInstance) {
        ::P4::error(ErrorType::ERR_INVALID,
                    "Expected %1% property value for table %2% to resolve to an "
                    "extern instance: %3%",
                    propertyName, t->name.originalName, property);
        return;
    }
    bool foundExtern = false;
    for (auto ext : externsInfo) {
        if (ext.first == externInstance->type->toString()) {
            auto externDefinition = tcPipeline->getExternDefinition(ext.first);
            if (externDefinition) {
                auto extInstDef = ((IR::TCExternInstance *)externDefinition->getExternInstance(
                    externInstanceName));
                extInstDef->setExternTableBindable(true);
                extInstDef->setNumElements(tentries);
                foundExtern = true;
                break;
            }
        }
    }
    if (foundExtern == false) {
        ::P4::error(ErrorType::ERR_INVALID,
                    "Expected %1% property value for table %2% to resolve to an "
                    "extern instance: %3%",
                    propertyName, t->name.originalName, property);
    }
}

void ConvertToBackendIR::updateAddOnMissTable(const IR::P4Table *t) {
    auto tblname = t->name.originalName;
    for (auto table : tcPipeline->tableDefs) {
        if (table->tableName == tblname) {
            add_on_miss_tables.push_back(t);
            auto tableDefinition = ((IR::TCTable *)table);
            tableDefinition->setTableAddOnMiss();
            tableDefinition->setTablePermission(HandleTableAccessPermission(t));
        }
    }
}

unsigned ConvertToBackendIR::GetAccessNumericValue(std::string_view access) {
    unsigned value = 0;
    for (auto s : access) {
        unsigned mask = 0;
        switch (s) {
            case 'C':
                mask = 1 << 6;
                break;
            case 'R':
                mask = 1 << 5;
                break;
            case 'U':
                mask = 1 << 4;
                break;
            case 'D':
                mask = 1 << 3;
                break;
            case 'X':
                mask = 1 << 2;
                break;
            case 'P':
                mask = 1 << 1;
                break;
            case 'S':
                mask = 1;
                break;
            default:
                ::P4::error(ErrorType::ERR_INVALID,
                            "tc_acl annotation cannot have '%1%' in access permisson", s);
        }
        value |= mask;
    }
    return value;
}

cstring ConvertToBackendIR::HandleTableAccessPermission(const IR::P4Table *t) {
    bool IsTableAddOnMiss = false;
    cstring control_path, data_path;
    for (auto table : add_on_miss_tables) {
        if (table->name.originalName == t->name.originalName) {
            IsTableAddOnMiss = true;
        }
    }
    auto find = tablePermissions.find(t->name.originalName);
    if (find != tablePermissions.end()) {
        auto paths = tablePermissions[t->name.originalName];
        control_path = paths->first;
        data_path = paths->second;
    }
    // Default access value of Control_path and Data_Path
    if (control_path.isNullOrEmpty()) {
        control_path = cstring(IsTableAddOnMiss ? DEFAULT_ADD_ON_MISS_TABLE_CONTROL_PATH_ACCESS
                                                : DEFAULT_TABLE_CONTROL_PATH_ACCESS);
    }
    if (data_path.isNullOrEmpty()) {
        data_path = cstring(IsTableAddOnMiss ? DEFAULT_ADD_ON_MISS_TABLE_DATA_PATH_ACCESS
                                             : DEFAULT_TABLE_DATA_PATH_ACCESS);
    }

    if (IsTableAddOnMiss) {
        auto access = data_path.find('C');
        if (!access) {
            ::P4::warning(
                ErrorType::WARN_INVALID,
                "Add on miss table '%1%' should have 'create' access permissons for data path.",
                t->name.originalName);
        }
    }
    // FIXME: refactor not to require cstring
    auto access_cp = GetAccessNumericValue(control_path);
    auto access_dp = GetAccessNumericValue(data_path);
    auto access_permisson = (access_cp << 7) | access_dp;
    std::stringstream value;
    value << "0x" << std::hex << access_permisson;
    return value.str();
}

std::pair<cstring, cstring> *ConvertToBackendIR::GetAnnotatedAccessPath(
    const IR::Annotation *anno) {
    cstring control_path, data_path;
    if (anno) {
        auto expr = anno->getExpr(0);
        if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
            auto permisson_str = typeLiteral->value;
            auto char_pos = permisson_str.find(":");
            control_path = permisson_str.before(char_pos);
            data_path = permisson_str.substr(char_pos - permisson_str.begin() + 1);
        }
    }
    auto paths = new std::pair<cstring, cstring>(control_path, data_path);
    return paths;
}

void ConvertToBackendIR::postorder(const IR::P4Table *t) {
    if (t != nullptr) {
        tableCount++;
        unsigned int tId = tableCount;
        auto tName = t->name.originalName;
        tableIDList.emplace(tId, tName);
        auto ctrl = findContext<IR::P4Control>();
        auto cName = ctrl->name.originalName;
        IR::TCTable *tableDefinition = new IR::TCTable(tId, tName, cName, pipelineName);
        auto tEntriesCount = TC::DEFAULT_TABLE_ENTRIES;
        auto sizeProperty = t->getSizeProperty();
        if (sizeProperty) {
            if (sizeProperty->fitsUint64()) {
                tEntriesCount = sizeProperty->asUint64();
            } else {
                ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "table with size %1% cannot be supported", t->getSizeProperty());
                return;
            }
        }
        tableDefinition->setTableEntriesCount(tEntriesCount);
        unsigned int keySize = 0;
        unsigned int keyCount = 0;
        auto key = t->getKey();
        if (key != nullptr && key->keyElements.size()) {
            for (auto k : key->keyElements) {
                auto keyExp = k->expression;
                auto keyExpType = typeMap->getType(keyExp);
                auto widthBits = keyExpType->width_bits();
                keySize += widthBits;
                keyCount++;
            }
        }
        tableDefinition->setKeySize(keySize);
        tableKeysizeList.emplace(tId, keySize);
        for (const auto *anno : t->getAnnotations()) {
            if (anno->name == ParseTCAnnotations::tc_acl) {
                tablePermissions.emplace(t->name.originalName, GetAnnotatedAccessPath(anno));
            } else if (anno->name == ParseTCAnnotations::numMask) {
                auto expr = anno->getExpr(0);
                if (auto val = expr->to<IR::Constant>()) {
                    tableDefinition->setNumMask(val->asUint64());
                } else {
                    ::P4::error(ErrorType::ERR_INVALID,
                                "nummask annotation cannot have '%1%' as value. Only integer "
                                "constants are allowed",
                                expr);
                }
            }
        }
        tableDefinition->setTablePermission(HandleTableAccessPermission(t));
        auto actionlist = t->getActionList();
        if (actionlist->size() == 0) {
            tableDefinition->addAction(tcPipeline->NoAction, TC::TABLEDEFAULT);
        } else {
            for (auto action : actionlist->actionList) {
                const IR::TCAction *tcAction = nullptr;
                auto adecl = refMap->getDeclaration(action->getPath(), true);
                auto actionName = externalName(adecl);
                for (auto actionDef : tcPipeline->actionDefs) {
                    if (actionName != actionDef->actionName) continue;
                    tcAction = actionDef;
                }
                if (actionName == P4::P4CoreLibrary::instance().noAction.name) {
                    tcAction = tcPipeline->NoAction;
                }
                unsigned int tableFlag = TC::TABLEDEFAULT;
                for (const auto *anno : action->getAnnotations()) {
                    if (anno->name == IR::Annotation::tableOnlyAnnotation) {
                        tableFlag = TC::TABLEONLY;
                    } else if (anno->name == IR::Annotation::defaultOnlyAnnotation) {
                        tableFlag = TC::DEFAULTONLY;
                    }
                }
                if (tcAction) {
                    tableDefinition->addAction(tcAction, tableFlag);
                }
            }
        }
        updatePnaDirectCounter(t, tableDefinition, tEntriesCount);
        updatePnaDirectMeter(t, tableDefinition, tEntriesCount);
        updateDefaultHitAction(t, tableDefinition);
        updateDefaultMissAction(t, tableDefinition);
        updateMatchType(t, tableDefinition);
        updateConstEntries(t, tableDefinition);
        updateTimerProfiles(tableDefinition);
        tcPipeline->addTableDefinition(tableDefinition);
    }
}

cstring ConvertToBackendIR::processExternPermission(const IR::Type_Extern *ext) {
    cstring control_path, data_path;
    // Check if access permissions is defined with annotation @tc_acl
    if (const auto *anno = ext->getAnnotation(ParseTCAnnotations::tc_acl)) {
        auto path = GetAnnotatedAccessPath(anno);
        control_path = path->first;
        data_path = path->second;
    }
    // Default access value of Control_path and Data_Path
    if (control_path.isNullOrEmpty()) {
        control_path = cstring(DEFAULT_EXTERN_CONTROL_PATH_ACCESS);
    }
    if (data_path.isNullOrEmpty()) {
        data_path = cstring(DEFAULT_EXTERN_DATA_PATH_ACCESS);
    }
    auto access_cp = GetAccessNumericValue(control_path);
    auto access_dp = GetAccessNumericValue(data_path);
    auto access_permisson = (access_cp << 7) | access_dp;
    std::stringstream value;
    value << "0x" << std::hex << access_permisson;
    return value.str();
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processExternConstructor(
    const IR::Type_Extern *extn, const IR::Declaration_Instance *decl,
    struct ExternInstance *instance) {
    safe_vector<const IR::TCKey *> keys;
    for (auto gd : *extn->getDeclarations()) {
        if (!gd->getNode()->is<IR::Method>()) {
            continue;
        }
        auto method = gd->getNode()->to<IR::Method>();
        auto params = method->getParameters();
        // Check if method is an constructor
        if (method->name != extn->name) {
            continue;
        }
        if (decl->arguments->size() != params->size()) {
            continue;
        }
        // Process all constructor arguments
        for (unsigned itr = 0; itr < params->size(); itr++) {
            auto parameter = params->getParameter(itr);
            auto exp = decl->arguments->at(itr)->expression;
            if (parameter->getAnnotation(ParseTCAnnotations::tc_numel)) {
                if (exp->is<IR::Constant>()) {
                    instance->is_num_elements = true;
                    instance->num_elements = exp->to<IR::Constant>()->asInt();
                }
            } else if (parameter->getAnnotation(ParseTCAnnotations::tc_init_val)) {
                // TODO: Process tc_init_val.
            } else {
                /* If a parameter is not annoated by tc_init or tc_numel then it is emitted as
                constructor parameters.*/
                cstring ptype = absl::StrCat("bit", parameter->type->width_bits());
                IR::TCKey *key = new IR::TCKey(0, parameter->type->width_bits(), ptype,
                                               parameter->toString(), "param"_cs, false);
                keys.push_back(key);
                if (exp->is<IR::Constant>()) {
                    key->setValue(exp->to<IR::Constant>()->asInt64());
                }
            }
        }
    }
    return keys;
}

cstring ConvertToBackendIR::getControlPathKeyAnnotation(const IR::StructField *field) {
    cstring annoName;
    auto annotation = field->getAnnotations().at(0);
    if (annotation->name == ParseTCAnnotations::tc_key ||
        annotation->name == ParseTCAnnotations::tc_data_scalar) {
        annoName = annotation->name;
    } else if (annotation->name == ParseTCAnnotations::tc_data) {
        annoName = "param"_cs;
    }
    return annoName;
}

ConvertToBackendIR::CounterType ConvertToBackendIR::toCounterType(const int type) {
    if (type == 0)
        return CounterType::PACKETS;
    else if (type == 1)
        return CounterType::BYTES;
    else if (type == 2)
        return CounterType::PACKETS_AND_BYTES;

    BUG("Unknown counter type %1%", type);
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processCounterControlPathKeys(
    const IR::Type_Struct *extern_control_path, const IR::Type_Extern *extn,
    const IR::Declaration_Instance *decl) {
    safe_vector<const IR::TCKey *> keys;
    auto typeArg = decl->arguments->at(decl->arguments->size() - 1)->expression->to<IR::Constant>();
    CounterType type = toCounterType(typeArg->asInt());
    int kId = 1;
    for (auto field : extern_control_path->fields) {
        /* If there is no annotation to control path key, ignore the key.*/
        if (field->getAnnotations().size() != 1) {
            continue;
        }
        cstring annoName = getControlPathKeyAnnotation(field);

        if (field->toString() == "pkts") {
            if (type == CounterType::PACKETS || type == CounterType::PACKETS_AND_BYTES) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            }
            continue;
        }
        if (field->toString() == "bytes") {
            if (type == CounterType::BYTES || type == CounterType::PACKETS_AND_BYTES) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            }
            continue;
        }

        /* If the field is of Type_Name example 'T'*/
        if (field->type->is<IR::Type_Name>()) {
            auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
            keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
        } else {
            cstring ptype = absl::StrCat("bit", field->type->width_bits());
            IR::TCKey *key = new IR::TCKey(kId++, field->type->width_bits(), ptype,
                                           field->toString(), annoName, true);
            keys.push_back(key);
        }
    }
    return keys;
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::processExternControlPath(
    const IR::Type_Extern *extn, const IR::Declaration_Instance *decl, cstring eName) {
    safe_vector<const IR::TCKey *> keys;
    auto find = ControlStructPerExtern.find(eName);
    if (find != ControlStructPerExtern.end()) {
        auto extern_control_path = ControlStructPerExtern[eName];
        if (eName == "DirectCounter" || eName == "Counter") {
            keys = processCounterControlPathKeys(extern_control_path, extn, decl);
            return keys;
        }

        int kId = 1;
        for (auto field : extern_control_path->fields) {
            /* If there is no annotation to control path key, ignore the key.*/
            if (field->getAnnotations().size() != 1) {
                continue;
            }
            cstring annoName = getControlPathKeyAnnotation(field);

            /* If the field is of Type_Name example 'T'*/
            if (field->type->is<IR::Type_Name>()) {
                auto temp_keys = HandleTypeNameStructField(field, extn, decl, kId, annoName);
                keys.insert(keys.end(), temp_keys.begin(), temp_keys.end());
            } else {
                cstring ptype = absl::StrCat("bit", field->type->width_bits());
                if (eName == "Meter") {
                    /* If Meter Type is Bytes, then set type of 'cir' and 'pir' as "rate"*/
                    auto meterTypeArg = decl->arguments->at(1)->expression->to<IR::Constant>();
                    if (meterTypeArg->value == 1 &&
                        (field->toString() == "cir" || field->toString() == "pir")) {
                        ptype = "rate"_cs;
                    }
                }
                IR::TCKey *key = new IR::TCKey(kId++, field->type->width_bits(), ptype,
                                               field->toString(), annoName, true);
                keys.push_back(key);
            }
        }
    }
    return keys;
}

safe_vector<const IR::TCKey *> ConvertToBackendIR::HandleTypeNameStructField(
    const IR::StructField *field, const IR::Type_Extern *extn, const IR::Declaration_Instance *decl,
    int &kId, cstring annoName) {
    safe_vector<const IR::TCKey *> keys;
    auto type_extern_params = extn->getTypeParameters()->parameters;
    for (unsigned itr = 0; itr < type_extern_params.size(); itr++) {
        if (type_extern_params.at(itr)->toString() == field->type->toString()) {
            auto decl_type = typeMap->getType(decl, true);
            auto ts = decl_type->to<IR::Type_SpecializedCanonical>();
            auto param_val = ts->arguments->at(itr);

            /* If 'T' is of Type_Struct, extract all fields of structure*/
            if (auto param_struct = param_val->to<IR::Type_Struct>()) {
                for (auto f : param_struct->fields) {
                    cstring ptype = absl::StrCat("bit", f->type->width_bits());
                    if (auto anno = f->getAnnotation(ParseTCAnnotations::tcType)) {
                        auto expr = anno->getExpr(0);
                        if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
                            ptype = typeLiteral->value;
                        }
                    }
                    IR::TCKey *key = new IR::TCKey(kId++, f->type->width_bits(), ptype,
                                                   f->toString(), annoName, true);
                    keys.push_back(key);
                }
            } else {
                cstring ptype = absl::StrCat("bit", param_val->width_bits());
                if (auto anno = field->getAnnotation(ParseTCAnnotations::tcType)) {
                    auto expr = anno->getExpr(0);
                    if (auto typeLiteral = expr->to<IR::StringLiteral>()) {
                        ptype = typeLiteral->value;
                    }
                }
                IR::TCKey *key = new IR::TCKey(kId++, param_val->width_bits(), ptype,
                                               field->toString(), annoName, true);
                keys.push_back(key);
            }
            break;
        }
    }
    return keys;
}

bool ConvertToBackendIR::hasExecuteMethod(const IR::Type_Extern *extn) {
    for (auto gd : *extn->getDeclarations()) {
        if (!gd->getNode()->is<IR::Method>()) {
            continue;
        }
        auto method = gd->getNode()->to<IR::Method>();
        const IR::Annotation *execAnnotation =
            method->getAnnotation(ParseTCAnnotations::tc_md_exec);
        if (execAnnotation) {
            return true;
        }
    }
    return false;
}

void ConvertToBackendIR::addExternTypeInstance(const IR::Declaration_Instance *decl,
                                               IR::TCExternInstance *tcExternInstance,
                                               cstring eName) {
    if (eName == "Meter" || eName == "DirectMeter" || eName == "Counter" ||
        eName == "DirectCounter") {
        auto typeArg =
            decl->arguments->at(decl->arguments->size() - 1)->expression->to<IR::Constant>();
        if (typeArg->value == 0) {
            tcExternInstance->setExternTypeInstance("PACKETS"_cs);
        } else if (typeArg->value == 1) {
            tcExternInstance->setExternTypeInstance("BYTES"_cs);
        } else {
            tcExternInstance->setExternTypeInstance("PACKETS_AND_BYTES"_cs);
        }
    }
}

/* Process each declaration instance of externs*/
void ConvertToBackendIR::postorder(const IR::Declaration_Instance *decl) {
    auto decl_type = typeMap->getType(decl, true);
    auto ts = decl_type->to<IR::Type_SpecializedCanonical>();
    if (decl_type->is<IR::Type_Extern>() || (ts && ts->baseType->is<IR::Type_Extern>())) {
        const IR::Type_Extern *extn;
        if (decl_type->is<IR::Type_Extern>()) {
            extn = decl_type->to<IR::Type_Extern>();
        } else {
            extn = ts->baseType->to<IR::Type_Extern>();
        }
        auto eName = extn->name;
        auto find = ControlStructPerExtern.find(eName);
        if (find == ControlStructPerExtern.end()) {
            return;
        }
        IR::TCExtern *externDefinition;
        auto instance = new struct ExternInstance();
        instance->instance_name = decl->toString();

        auto constructorKeys = processExternConstructor(extn, decl, instance);

        // Get Control Path information if specified for extern.
        auto controlKeys = processExternControlPath(extn, decl, eName);

        bool has_exec_method = hasExecuteMethod(extn);

        /* If the extern info is already present, add new instance
           Or else create new extern info.*/
        auto iterator = externsInfo.find(eName);
        if (iterator == externsInfo.end()) {
            struct ExternBlock *eb = new struct ExternBlock();
            if (eName == "DirectCounter") {
                eb->externId = "0x1A000000"_cs;
            } else if (eName == "Counter") {
                eb->externId = "0x19000000"_cs;
            } else if (eName == "Digest") {
                eb->externId = "0x05000000"_cs;
                instance->is_num_elements = true;
                instance->num_elements = 0;
            } else if (eName == "Meter") {
                eb->externId = "0x1B000000"_cs;
            } else if (eName == "DirectMeter") {
                eb->externId = "0x1C000000"_cs;
            } else {
                externCount += 1;
                std::stringstream value;
                value << "0x" << std::hex << externCount;
                eb->externId = value.str();
            }
            eb->permissions = processExternPermission(extn);
            eb->no_of_instances += 1;
            externsInfo.emplace(eName, eb);
            instance->instance_id = eb->no_of_instances;
            eb->eInstance.push_back(instance);

            externDefinition =
                new IR::TCExtern(eb->externId, eName, pipelineName, eb->no_of_instances,
                                 eb->permissions, has_exec_method);
            tcPipeline->addExternDefinition(externDefinition);
        } else {
            auto eb = externsInfo[eName];
            externDefinition = ((IR::TCExtern *)tcPipeline->getExternDefinition(eName));
            externDefinition->numinstances = ++eb->no_of_instances;
            instance->instance_id = eb->no_of_instances;
            eb->eInstance.push_back(instance);
        }
        IR::TCExternInstance *tcExternInstance =
            new IR::TCExternInstance(instance->instance_id, instance->instance_name,
                                     instance->is_num_elements, instance->num_elements);
        if (controlKeys.size() != 0) {
            tcExternInstance->addControlPathKeys(controlKeys);
        }
        if (constructorKeys.size() != 0) {
            tcExternInstance->addConstructorKeys(constructorKeys);
        }
        addExternTypeInstance(decl, tcExternInstance, eName);
        externDefinition->addExternInstance(tcExternInstance);
    }
}

void ConvertToBackendIR::postorder(const IR::Type_Struct *ts) {
    auto struct_name = ts->externalName();
    auto cp = "tc_ControlPath_";
    if (struct_name.startsWith(cp)) {
        auto type_extern_name = struct_name.substr(strlen(cp));
        ControlStructPerExtern.emplace(type_extern_name, ts);
    }
}

void ConvertToBackendIR::postorder(const IR::P4Program *p) {
    if (p != nullptr) {
        tcPipeline->setPipelineName(pipelineName);
        tcPipeline->setNumTables(tableCount);
    }
}

/**
 * This function is used for checking whether given member is PNA Parser metadata
 */
bool ConvertToBackendIR::isPnaParserMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaParserMeta) return true;
        }
    }
    return false;
}

bool ConvertToBackendIR::isPnaMainInputMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaInputMeta) return true;
        }
    }
    return false;
}

bool ConvertToBackendIR::isPnaMainOutputMeta(const IR::Member *mem) {
    if (mem->expr != nullptr && mem->expr->type != nullptr) {
        if (auto str_type = mem->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == pnaOutputMeta) return true;
        }
    }
    return false;
}

unsigned int ConvertToBackendIR::findMappedKernelMeta(const IR::Member *mem) {
    if (isPnaParserMeta(mem)) {
        for (auto i = 0; i < TC::MAX_PNA_PARSER_META; i++) {
            if (mem->member.name == pnaMainParserInputMetaFields[i]) {
                if (i == TC::PARSER_RECIRCULATED) {
                    return TC::SKBREDIR;
                } else if (i == TC::PARSER_INPUT_PORT) {
                    return TC::SKBIIF;
                }
            }
        }
    } else if (isPnaMainInputMeta(mem)) {
        for (auto i = 0; i < TC::MAX_PNA_INPUT_META; i++) {
            if (mem->member.name == pnaMainInputMetaFields[i]) {
                switch (i) {
                    case TC::INPUT_RECIRCULATED:
                        return TC::SKBREDIR;
                    case TC::INPUT_TIMESTAMP:
                        return TC::SKBTSTAMP;
                    case TC::INPUT_PARSER_ERROR:
                        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                    "%1% is not supported in this target", mem);
                        return TC::UNSUPPORTED;
                    case TC::INPUT_CLASS_OF_SERVICE:
                        return TC::SKBPRIO;
                    case TC::INPUT_INPUT_PORT:
                        return TC::SKBIIF;
                }
            }
        }
    } else if (isPnaMainOutputMeta(mem)) {
        if (mem->member.name == pnaMainOutputMetaFields[TC::OUTPUT_CLASS_OF_SERVICE]) {
            return TC::SKBPRIO;
        }
    }
    return TC::UNDEFINED;
}

const IR::Expression *ConvertToBackendIR::ExtractExpFromCast(const IR::Expression *exp) {
    const IR::Expression *castexp = exp;
    while (castexp->is<IR::Cast>()) {
        castexp = castexp->to<IR::Cast>()->expr;
    }
    return castexp;
}

unsigned ConvertToBackendIR::getTcType(const IR::StringLiteral *sl) {
    auto value = sl->value;
    auto typeVal = TC::BIT_TYPE;
    if (value == "dev") {
        typeVal = TC::DEV_TYPE;
    } else if (value == "macaddr") {
        typeVal = TC::MACADDR_TYPE;
    } else if (value == "ipv4") {
        typeVal = TC::IPV4_TYPE;
    } else if (value == "ipv6") {
        typeVal = TC::IPV6_TYPE;
    } else if (value == "be16") {
        typeVal = TC::BE16_TYPE;
    } else if (value == "be32") {
        typeVal = TC::BE32_TYPE;
    } else if (value == "be64") {
        typeVal = TC::BE64_TYPE;
    }
    return typeVal;
}

unsigned ConvertToBackendIR::getTableId(cstring tableName) const {
    for (auto t : tableIDList) {
        if (t.second == tableName) return t.first;
    }
    return 0;
}

unsigned ConvertToBackendIR::getActionId(cstring actionName) const {
    for (auto a : actionIDList) {
        if (a.second == actionName) return a.first;
    }
    return 0;
}

cstring ConvertToBackendIR::getExternId(cstring externName) const {
    for (auto e : externsInfo) {
        if (e.first == externName) return e.second->externId;
    }
    return ""_cs;
}

unsigned ConvertToBackendIR::getExternInstanceId(cstring externName, cstring instanceName) const {
    for (auto e : externsInfo) {
        if (e.first == externName) {
            for (auto eI : e.second->eInstance) {
                if (eI->instance_name == instanceName) {
                    return eI->instance_id;
                }
            }
        }
    }
    return 0;
}

unsigned ConvertToBackendIR::getTableKeysize(unsigned tableId) const {
    auto itr = tableKeysizeList.find(tableId);
    if (itr != tableKeysizeList.end()) return itr->second;
    return 0;
}

void ConvertToBackendIR::updateMatchType(const IR::P4Table *t, IR::TCTable *tabledef) {
    auto key = t->getKey();
    auto tableMatchType = TC::EXACT_TYPE;
    if (key != nullptr && key->keyElements.size()) {
        if (key->keyElements.size() == 1) {
            auto matchTypeExp = key->keyElements[0]->matchType->path;
            auto mtdecl = refMap->getDeclaration(matchTypeExp, true);
            auto matchTypeInfo = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().exactMatch.name) {
                tableMatchType = TC::EXACT_TYPE;
            } else if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
                tableMatchType = TC::LPM_TYPE;
            } else if (matchTypeInfo->name.name ==
                       P4::P4CoreLibrary::instance().ternaryMatch.name) {
                tableMatchType = TC::TERNARY_TYPE;
            } else if (matchTypeInfo->name.name == "range" ||
                       matchTypeInfo->name.name == "rangelist" ||
                       matchTypeInfo->name.name == "optional") {
                tableMatchType = TC::TERNARY_TYPE;
            } else {
                ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "match type %1% is not supported in this target",
                            key->keyElements[0]->matchType);
                return;
            }
        } else {
            unsigned totalKey = key->keyElements.size();
            unsigned exactKey = 0;
            unsigned lpmKey = 0;
            unsigned ternaryKey = 0;
            unsigned keyCount = 0;
            unsigned lastkeyMatchType = TC::EXACT_TYPE;
            unsigned keyMatchType;
            for (auto k : key->keyElements) {
                auto matchTypeExp = k->matchType->path;
                auto mtdecl = refMap->getDeclaration(matchTypeExp, true);
                auto matchTypeInfo = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchTypeInfo->name.name == P4::P4CoreLibrary::instance().exactMatch.name) {
                    keyMatchType = TC::EXACT_TYPE;
                    exactKey++;
                } else if (matchTypeInfo->name.name ==
                           P4::P4CoreLibrary::instance().lpmMatch.name) {
                    keyMatchType = TC::LPM_TYPE;
                    lpmKey++;
                } else if (matchTypeInfo->name.name ==
                           P4::P4CoreLibrary::instance().ternaryMatch.name) {
                    keyMatchType = TC::TERNARY_TYPE;
                    ternaryKey++;
                } else if (matchTypeInfo->name.name == "range" ||
                           matchTypeInfo->name.name == "rangelist" ||
                           matchTypeInfo->name.name == "optional") {
                    keyMatchType = TC::TERNARY_TYPE;
                    ternaryKey++;
                } else {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "match type %1% is not supported in this target", k->matchType);
                    return;
                }
                keyCount++;
                if (keyCount == totalKey) {
                    lastkeyMatchType = keyMatchType;
                }
            }
            if (ternaryKey >= 1 || lpmKey > 1) {
                tableMatchType = TC::TERNARY_TYPE;
            } else if (exactKey == totalKey) {
                tableMatchType = TC::EXACT_TYPE;
            } else if (lpmKey == 1 && lastkeyMatchType == TC::LPM_TYPE) {
                tableMatchType = TC::LPM_TYPE;
            }
        }
    }
    tabledef->setMatchType(tableMatchType);
}

bool ConvertToBackendIR::checkParameterDirection(const IR::TCAction *tcAction) {
    bool dirParam = false;
    for (auto actionParam : tcAction->actionParams) {
        if (actionParam->getDirection() != TC::NONE) {
            dirParam = true;
            break;
        }
    }
    return dirParam;
}
}  // namespace P4::TC
