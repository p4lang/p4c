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

#include "gateway.h"
#include <deque>
#include "split_gateways.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/lib/safe_width.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/mau/asm_output.h"
#include "ir/pattern.h"

const Device::GatewaySpec &TofinoDevice::getGatewaySpec() const {
    static const Device::GatewaySpec spec = {
        /* .PhvBytes = */       4,
        /* .HashBits = */       12,
        /* .PredicateBits = */  0,
        /* .MaxRows = */        4,
        /* .SupportXor = */     true,
        /* .SupportRange = */   true,
        /* .ExactShifts = */    1,
        /* .ByteSwizzle = */    true,
        /* .PerByteMatch = */   0,
        /* .XorByteSlots = */   0xf0,
    };
    return spec; }
#if HAVE_JBAY
const Device::GatewaySpec &JBayDevice::getGatewaySpec() const {
    static const Device::GatewaySpec spec = {
        /* .PhvBytes = */       4,
        /* .HashBits = */       12,
        /* .PredicateBits = */  0,
        /* .MaxRows = */        4,
        /* .SupportXor = */     true,
        /* .SupportRange = */   true,
        /* .ExactShifts = */    5,
        /* .ByteSwizzle = */    true,
        /* .PerByteMatch = */   0,
        /* .XorByteSlots = */   0xf0,
    };
    return spec; }
#endif

class CanonGatewayExpr::NeedNegate : public Inspector {
    bool        rv = false;
    bool preorder(const IR::Expression *) override { return !rv; }
    bool preorder(const IR::Neq *neq) override {
        if (safe_width_bits(neq->left->type) == 1) {
            /* 1-bit != can be done directly; no need to negate it into == */
            return true; }
        rv = true; return false; }
 public:
    explicit NeedNegate(const IR::Expression *e) { e->apply(*this); }
    explicit NeedNegate(safe_vector<GWRow_t> &rows) {
        for (auto &row : rows) {
            if (row.first) row.first->apply(*this);
            if (rv) break; } }
    explicit operator bool() const { return rv; }
};

// FIXME -- should make this work for signed types, but big_ints don't work well
// with 2s complement
static big_int SliceReduce(IR::Operation::Relation *rel, big_int val) {
    if (val <= 0) return val;
    int slice = ffs(val);
    if (slice > 0) {
        val >>= slice;
        LOG4("Slicing " << slice << " bits off the bottom of " << rel);
        rel->left = MakeSlice(rel->left, slice, safe_width_bits(rel->left->type) - 1);
        rel->right = new IR::Constant(val);
        LOG4("Now have " << rel); }
    return val;
}

/// Try to figure out where an expression should be sliced to better byte align things
/// by slicing on a byte boundary.  @returns the bit-within-byte of the lowest bit of the
/// expression (0..7)
static int byte_align(const IR::Expression *e) {
    if (auto *sl = e->to<IR::Slice>())
        return (sl->getL() + byte_align(sl->e0)) & 7;
    if (auto *b = e->to<IR::Operation::Binary>())
        return byte_align(b->left);
    if (auto *m = e->to<IR::Member>())
        return m->offset_bits() & 7;
    if (auto *u = e->to<IR::Operation::Unary>())
        return byte_align(u->expr);
    return 0;
}

IR::Node *CanonGatewayExpr::preorder(IR::MAU::Table *tbl) {
    auto &rows = tbl->gateway_rows;
    if (rows.empty() || !rows[0].first)
        return tbl;
    if (!tbl->gateway_cond.isNullOrEmpty()) return tbl;
    // Store original condition string as specified in p4 program
    cstring gateway_cond = cstring::to_cstring(rows[0].first);
    // Remove local gress references
    gateway_cond = gateway_cond.replace("ingress::", "");
    gateway_cond = gateway_cond.replace("egress::", "");
    gateway_cond = gateway_cond.replace(";", "");
    tbl->gateway_cond = gateway_cond;
    LOG3("Table: " << tbl->name << " Condition: " << tbl->gateway_cond);
    return tbl;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::Operation::Relation *e) {
    LOG5(_debugIndent << "IR::Rel " << e);
    // only called for Equ and Neq
    if (e->left->equiv(*e->right))  // if the two sides are the same expression, fold it
        return new IR::BoolLiteral(e->is<IR::Equ>() ? true : false);
    // If comparing with a constant, normalize the condition.
    // However, if both terms are constant, do not swap them, otherwise the IR tree
    // will continuously change, resulting in an infinite loop for GatewayOpt::PassRepeat
    // FIXME -- should have been constant folded?
    if (e->left->is<IR::Literal>() && !e->right->is<IR::Literal>()) {
        std::swap(e->left, e->right); }
    if (e->right->is<IR::Operation::Relation>()) {
        std::swap(e->left, e->right); }
    if (e->left->is<IR::Operation::Relation>()) {
        // comparing the result of a comparison with a boolean value
        bool add_not = e->is<IR::Neq>();
        if (auto k = e->right->to<IR::Constant>()) {
            add_not ^= (k->value == 0);
        } else if (auto k = e->right->to<IR::BoolLiteral>()) {
            add_not ^= (k->value == 0);
        } else {
            // This will currently cause 'Gateway expression too complex', but we
            // might eventually be able to handle it by splitting the expression.
            // we could also potentially transform to (a && !b) || (!a && b) instead of a^b
            // which might fit in one gateway without requiring added stages
            IR::Expression *rv = new IR::BXor(e->srcInfo, e->left, e->right);
            if (!add_not)
                rv = new IR::LNot(e);
            return rv; }
        if (add_not)
            return postorder(new IR::LNot(e->left));
        return e->left; }
    Pattern::Match<IR::Expression>      e1, e2;
    Pattern::Match<IR::Constant>        k1, k2;
    int width = safe_width_bits(e->left->type);
    if (((e1 & k1) == k2).match(e) || ((e1 & k1) != k2).match(e)) {
        BUG_CHECK(!e1->is<IR::Constant>(), "constant folding failed");
        // always true or always false
        if ((abs(k1->value) & abs(k2->value)) != abs(k2->value)) {
            auto rv = new IR::BoolLiteral(e->srcInfo, e->is<IR::Equ>() ? false : true);
            warning("Masked comparison is always %s", rv);
            return rv; }
        // masked comparison
        auto shift = ffs(abs(k1->value));
        if (shift > 0) {
            if (unsigned(ffs(abs(k2->value))) >= unsigned(shift)) {
                // subtle point -- unsigned casts above to be true when ffs returns -1
                e->left = new IR::BAnd(MakeSlice(e1, shift, width - 1),
                                       MakeSlice(k1, shift, width - 1));
                e->right = new IR::Constant(e->left->type, k2->value >> shift);
                width -= shift;
            } else {
                auto rv = new IR::BoolLiteral(e->srcInfo, e->is<IR::Equ>() ? false : true);
                warning("Masked comparison is always %s", rv);
                return rv; } }
    } else if (((e1 & k1) == (e2 & k2)).match(e) || ((e1 & k1) != (e2 & k2)).match(e)) {
        BUG_CHECK(!e1->is<IR::Constant>(), "constant folding failed");
        BUG_CHECK(!e2->is<IR::Constant>(), "constant folding failed");
        BUG_CHECK(width == safe_width_bits(e1->type), "Type mismatch in CanonGatewayExpr");
        auto shift = ffs(k1->value | k2->value);
        if (shift > 0) {
            e->left = new IR::BAnd(MakeSlice(e1, shift, width - 1),
                                   MakeSlice(k1, shift, width - 1));
            e->right = new IR::BAnd(MakeSlice(e2, shift, width - 1),
                                    MakeSlice(k2, shift, width - 1));
            width -= shift; } }
    if (width > 32) {
        // We can't handle wide matches in one go, so split it.
        int slice_at = 32 - byte_align(e->left);
        auto *clone = e->clone();
        e->left = MakeSlice(e->left, slice_at, width-1);
        e->right = MakeSlice(e->right, slice_at, width-1);
        clone->left = MakeSlice(clone->left, 0, slice_at-1);
        clone->right = MakeSlice(clone->right, 0, slice_at-1);
        LOG5(_debugIndent << "  Split wide to: " << clone <<
             (e->is<IR::Equ>() ? " && " : " || ") << e);
        if (e->is<IR::Equ>())
            return new IR::LAnd(postorder(e), postorder(clone));
        else
            return new IR::LOr(postorder(e), postorder(clone)); }
    return e;
}

static big_int maxValueOfType(const IR::Type *type_) {
    auto *type = type_->to<IR::Type::Bits>();
    BUG_CHECK(type, "%s is not an integral type", type_);
    big_int rv = 1U;
    rv <<= type->size - type->isSigned;
    return rv - 1;
}


const IR::Expression *CanonGatewayExpr::postorder(IR::Leq *e) {
    LOG5(_debugIndent << "IR::Leq " << e);
    if (e->left->equiv(*e->right))  // if the two sides are the same expression, fold it
        return new IR::BoolLiteral(true);
    if (e->left->is<IR::Constant>())
        return postorder(new IR::Geq(e->right, e->left));
    if (auto k = e->right->to<IR::Constant>()) {
        if (k->value == maxValueOfType(k->type))
            return new IR::BoolLiteral(true);
        return postorder(new IR::Lss(e->left, new IR::Constant(k->type, k->value + 1))); }
    return e;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::Lss *e) {
    LOG5(_debugIndent << "IR::Lss " << e);
    if (e->left->equiv(*e->right))  // if the two sides are the same expression, fold it
        return new IR::BoolLiteral(false);
    if (auto k = e->left->to<IR::Constant>()) {
        BUG_CHECK(!e->right->is<IR::Constant>(), "constant folding failed");
        if (k->value == maxValueOfType(k->type))
            return new IR::BoolLiteral(false);
        return postorder(new IR::Geq(e->right, new IR::Constant(k->type, k->value + 1))); }
    if (auto k = e->right->to<IR::Constant>()) {
        if (k->value == 0) {
            if (isSigned(e->left->type)) {
                int signbit = e->left->type->width_bits() - 1;
                return new IR::Equ(MakeSlice(e->left, signbit, signbit), new IR::Constant(1));
            } else {
                return new IR::BoolLiteral(false); } }
        if (!isSigned(e->left->type) && SliceReduce(e, k->value) == 1)
            return postorder(new IR::Equ(e->left, new IR::Constant(0))); }
    return e;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::Geq *e) {
    LOG5(_debugIndent << "IR::Geq " << e);
    if (e->left->equiv(*e->right))  // if the two sides are the same expression, fold it
        return new IR::BoolLiteral(true);
    if (auto k = e->left->to<IR::Constant>()) {
        BUG_CHECK(!e->right->is<IR::Constant>(), "constant folding failed");
        if (k->value == maxValueOfType(k->type))
            return new IR::BoolLiteral(true);
        return postorder(new IR::Lss(e->right, new IR::Constant(k->type, k->value + 1))); }
    if (auto k = e->right->to<IR::Constant>()) {
        if (k->value == 0) {
            if (isSigned(e->left->type)) {
                int signbit = e->left->type->width_bits() - 1;
                return new IR::Equ(MakeSlice(e->left, signbit, signbit), new IR::Constant(0));
            } else {
                return new IR::BoolLiteral(true); } }
        if (!isSigned(e->left->type) && SliceReduce(e, k->value) == 1)
            return postorder(new IR::Neq(e->left, new IR::Constant(0))); }
    return e;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::Grt *e) {
    LOG5(_debugIndent << "IR::Grt " << e);
    if (e->left->equiv(*e->right))  // if the two sides are the same expression, fold it
        return new IR::BoolLiteral(false);
    if (e->left->is<IR::Constant>())
        return postorder(new IR::Lss(e->right, e->left));
    if (auto k = e->right->to<IR::Constant>()) {
        if (k->value == maxValueOfType(k->type))
            return new IR::BoolLiteral(false);
        return postorder(new IR::Geq(e->left, new IR::Constant(k->type, k->value + 1))); }
    return e;
}

/// Simplify as fast as possible to avoid expansion when going between deMorgan
/// and distribution
const IR::Expression *CanonGatewayExpr::preorder(IR::LAnd *e) {
    if (e->left->equiv(*e->right))
        return e->left;
    if (auto k = e->left->to<IR::Constant>())
        return k->value ? e->right : k;
    if (auto k = e->left->to<IR::BoolLiteral>())
        return k->value ? e->right : k;
    if (auto k = e->right->to<IR::Constant>())
        return k->value ? e->left : k;
    if (auto k = e->right->to<IR::BoolLiteral>())
        return k->value ? e->left : k;
    return e;
}

const IR::Expression *CanonGatewayExpr::preorder(IR::LOr *e) {
    if (e->left->equiv(*e->right))
        return e->left;
    if (auto k = e->left->to<IR::Constant>())
        return k->value ? k : e->right;
    if (auto k = e->left->to<IR::BoolLiteral>())
        return k->value ? k : e->right;
    if (auto k = e->right->to<IR::Constant>())
        return k->value ? k : e->left;
    if (auto k = e->right->to<IR::BoolLiteral>())
        return k->value ? k : e->left;
    return e;
}

const IR::Expression *CanonGatewayExpr::preorder(IR::LNot *e) {
    if (auto a = e->expr->to<IR::LNot>()) {
        LOG5(_debugIndent << "r IR::LNot " << e);
        return a->expr;
    }
    return e;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::LAnd *e) {
    LOG5(_debugIndent++ << "IR::LAnd " << e);
    const IR::Expression *rv = e;
    if (e->left->equiv(*e->right))
        return e->left;
    if (auto k = e->left->to<IR::Constant>())
        return k->value ? e->right : k;
    if (auto k = e->left->to<IR::BoolLiteral>())
        return k->value ? e->right : k;
    if (auto k = e->right->to<IR::Constant>())
        return k->value ? e->left : k;
    if (auto k = e->right->to<IR::BoolLiteral>())
        return k->value ? e->left : k;
    while (auto r = e->right->to<IR::LAnd>()) {
        e->left = postorder(new IR::LAnd(e->left, r->left));
        e->right = r->right; }
    if (auto l = e->left->to<IR::LOr>()) {
        if (auto r = e->right->to<IR::LOr>()) {
            auto c1 = new IR::LAnd(l->left, r->left);
            auto c2 = new IR::LAnd(l->left, r->right);
            auto c3 = new IR::LAnd(l->right, r->left);
            auto c4 = new IR::LAnd(l->right, r->right);
            rv = new IR::LOr(new IR::LOr(new IR::LOr(c1, c2), c3), c4);
            LOG5(_debugIndent << "? IR::LAnd " << rv);
        } else {
            auto c1 = new IR::LAnd(l->left, e->right);
            auto c2 = new IR::LAnd(l->right, e->right);
            rv = new IR::LOr(c1, c2); }
        LOG5(_debugIndent << "/ IR::LAnd " << rv);
    } else if (auto r = e->right->to<IR::LOr>()) {
        auto c1 = new IR::LAnd(e->left, r->left);
        auto c2 = new IR::LAnd(e->left, r->right);
        rv = new IR::LOr(c1, c2);
        LOG5(_debugIndent << "* IR::LAnd " << rv);
    }
    if (rv != e)
        visit(rv);
    LOG5(--_debugIndent << "- IR::LAnd " << rv);
    return rv;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::LOr *e) {
    LOG5(_debugIndent++ << "IR::LOr " << e);
    if (e->left->equiv(*e->right))
        return e->left;
    if (auto k = e->left->to<IR::Constant>())
        return k->value ? k : e->right;
    if (auto k = e->left->to<IR::BoolLiteral>())
        return k->value ? k : e->right;
    if (auto k = e->right->to<IR::Constant>())
        return k->value ? k : e->left;
    if (auto k = e->right->to<IR::BoolLiteral>())
        return k->value ? k : e->left;
    while (auto r = e->right->to<IR::LOr>()) {
        e->left = postorder(new IR::LOr(e->left, r->left));
        e->right = r->right; }
    LOG5(--_debugIndent << "-IR::LOr " << e);
    return e;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::LNot *e) {
    LOG5(++_debugIndent << "IR::LNot " << e);
    const IR::Expression *rv = e;
    if (auto a = e->expr->to<IR::LNot>()) {
        LOG5(--_debugIndent << "r IR::LNot " << e);
        return a->expr; }
    if (auto k = e->expr->to<IR::Constant>()) {
        return new IR::Constant(k->value == 0);
    } else if (auto k = e->expr->to<IR::BoolLiteral>()) {
        return new IR::BoolLiteral(!k->value); }
    if (auto a = e->expr->to<IR::LAnd>()) {
        rv = new IR::LOr(new IR::LNot(a->left), new IR::LNot(a->right));
    } else if (auto a = e->expr->to<IR::LOr>()) {
        rv = new IR::LAnd(new IR::LNot(a->left), new IR::LNot(a->right));
    } else if (auto a = e->expr->to<IR::Equ>()) {
        rv = new IR::Neq(a->left, a->right);
    } else if (auto a = e->expr->to<IR::Neq>()) {
        rv = new IR::Equ(a->left, a->right);
    } else if (auto a = e->expr->to<IR::Leq>()) {
        rv = new IR::Grt(a->left, a->right);
    } else if (auto a = e->expr->to<IR::Lss>()) {
        rv = new IR::Geq(a->left, a->right);
    } else if (auto a = e->expr->to<IR::Geq>()) {
        rv = new IR::Lss(a->left, a->right);
    } else if (auto a = e->expr->to<IR::Grt>()) {
        rv = new IR::Grt(a->left, a->right); }
    if (rv != e)
        visit(rv);
    LOG5(--_debugIndent << "-IR::LNot " << rv);
    return rv;
}

const IR::Expression *CanonGatewayExpr::postorder(IR::BAnd *e) {
    if (e->left->is<IR::Constant>()) {
        BUG_CHECK(!e->right->is<IR::Constant>(), "constant folding failed");
        auto *t = e->left;
        e->left = e->right;
        e->right = t; }
    if (auto k = e->right->to<IR::Constant>()) {
        int maxbit = floor_log2(k->value);
        if (maxbit >= 0 && e->left->type->width_bits() > maxbit + 1) {
            e->left = MakeSlice(e->left, 0, maxbit);
            e->type = e->left->type; } }
    return e;
}
const IR::Expression *CanonGatewayExpr::postorder(IR::BOr *e) {
    if (e->left->is<IR::Constant>()) {
        BUG_CHECK(!e->right->is<IR::Constant>(), "constant folding failed");
        auto *t = e->left;
        e->left = e->right;
        e->right = t; }
    return e;
}

/*
 * is_validated() may be applied to a constant as the result of
 * constant propagation. In that case, the container validity bit
 * is automatically assumed to be set due to an earlier assignment.
 */
const IR::Expression *CanonGatewayExpr::postorder(IR::MAU::TypedPrimitive *p) {
    if (p->name != "is_validated")
        error("%scondition too complex", p->srcInfo);
    auto e = p->operands.at(0);
    if (e->is<IR::Constant>())
        return new IR::BoolLiteral(true);
    return p;
}

/* return true if two boolean expressions are contradictory (cannot both be true) */
static bool contradictory(const IR::Expression *a_, const IR::Expression *b_) {
    auto a = a_->to<IR::Operation::Relation>();
    auto b = b_->to<IR::Operation::Relation>();
    if (!a || !b) return false;
    // FIXME -- should we check for a->left is a slice of b->left or vice versa?
    if (!a->left->equiv(*b->left)) return false;
    if (a->right->equiv(*b->right)) {
        /* a and be are both 'x relop y' for two different relops */
        if (a->is<IR::Equ>() && (b->is<IR::Neq>() || b->is<IR::Lss>())) return true;
        if (b->is<IR::Equ>() && (a->is<IR::Neq>() || a->is<IR::Lss>())) return true;
        return false; }
    auto ak = a->right->to<IR::Constant>();
    auto bk = b->right->to<IR::Constant>();
    if (!ak || !bk) return false;
    BUG_CHECK(ak->value != bk->value, "equal constants %1% and %2% are not equivalent?", ak, bk);
    if (ak->value < bk->value) {
        if ((a->is<IR::Equ>() || a->is<IR::Lss>()) && (b->is<IR::Equ>() || b->is<IR::Geq>()))
            return true;
    } else {
        if ((a->is<IR::Equ>() || a->is<IR::Geq>()) && (b->is<IR::Equ>() || b->is<IR::Lss>()))
            return true; }
    return false;
}

/* simplify a canonical row by removing duplicate conjuncts and making the entire row
 * false if it contains contradictory conjuncts */
static const IR::Expression *simplifyRow(const IR::Expression *e) {
    std::vector<const IR::Expression *> terms;
    bool delta = false;
    auto *rest = e;
    auto *conj = rest->to<IR::LAnd>();
    while (auto *t = conj ? conj->right : rest) {
        bool remove = false;
        for (auto p : terms) {
            if (t->equiv(*p))
                remove = true;
            else if (contradictory(t, p))
                return new IR::BoolLiteral(e->srcInfo, false); }
        if (remove) {
            delta = true;
        } else {
            terms.push_back(t); }
        rest = nullptr;
        if (conj) {
            rest = conj->left;
            conj = rest->to<IR::LAnd>(); } }
    if (delta) {
        e = terms.back();
        terms.pop_back();
        while (!terms.empty()) {
            e = new IR::LAnd(e, terms.back());
            terms.pop_back(); } }
    return e;
}

/* break apart the terms of a conjunction into a set of conjuncts */
static std::set<const IR::Expression *> rowConjuncts(const IR::Expression *e) {
    std::set<const IR::Expression *> rv;
    while (auto *conj = e->to<IR::LAnd>()) {
        rv.insert(conj->right);
        e = conj->left; }
    rv.insert(e);
    return rv;
}

/* remove a conjunct from a set of conjuncts if present.  FIXME This would be much more
 * efficient if the IR generator supported autogen hash and/or operator< for IR nodes
 * to allow efficient set tracking */
static void removeConjunct(std::set<const IR::Expression *> &s, const IR::Expression *e) {
    for (auto it = s.begin(); it != s.end();) {
        if (e->equiv(**it))
            it = s.erase(it);
        else
            ++it; }
}

/** Test a row conjunction to see if it contains every term in a previous row.  If it
 * does it can be safely reomved as it will never match (the previous row will also
 * match, and at a higher priority, dominating it).   We need a copy of the 'prev'
 * set here so we can modify to see if all elements are present */
static bool rowDominates(std::set<const IR::Expression *> prev, const IR::Expression *e) {
    while (auto *conj = e->to<IR::LAnd>()) {
        removeConjunct(prev, conj->right);
        if (prev.empty()) return true;
        e = conj->left; }
    removeConjunct(prev, e);
    return prev.empty();
}

void CanonGatewayExpr::removeUnusedRows(IR::MAU::Table *tbl, bool isCanon) {
    auto &rows = tbl->gateway_rows;
    // Remove rows that can never match because it is
    // - after a row that always matches,
    // - after a row that dominates their match (only if gateway is canonicalized)
    // - condition is 'false'.
    // While doing that, track the next tags that we remove and keep.
    bool erase_rest = false;
    std::set<cstring>   removed, present;  // next tags in the table from the gateway
    std::vector<std::set<const IR::Expression *>> prevRowTerms;
    for (auto row = rows.begin(); row != rows.end();) {
        if (erase_rest) {
            removed.insert(row->second);
            row = rows.erase(row);
            continue; }
        if (!row->first) {
            erase_rest = true;
            present.insert(row->second);
            ++row;
            continue; }
        row->first = simplifyRow(row->first);
        if (auto k = row->first->to<IR::Constant>()) {
            if (k->value == 0) {
                removed.insert(row->second);
                row = rows.erase(row);
            } else {
                row->first = nullptr; }
            continue; }
        if (auto k = row->first->to<IR::BoolLiteral>()) {
            if (k->value == 0) {
                removed.insert(row->second);
                row = rows.erase(row);
            } else {
                row->first = nullptr; }
            continue; }
        bool remove_row = false;
        for (auto &prev : prevRowTerms) {
            if (rowDominates(prev, row->first)) {
                remove_row = true;
                break; } }
        if (remove_row) {
            removed.insert(row->second);
            row = rows.erase(row);
        } else {
            if (isCanon)
                prevRowTerms.push_back(rowConjuncts(row->first));
            present.insert(row->second);
            ++row; } }
    // If we removed ALL gateway rows that refer to a next tag, remove that tag from
    // next, as it's unreachable.  This relies on the fact that gateway next tags and
    // action next tags are always disjoint.
    for (auto next_tag : removed)
        if (!present.count(next_tag))
            tbl->next.erase(next_tag);
}

/* return the number of conjunct terms in a gateway row expression */
static int conjuncts(const IR::Expression *e) {
    if (e == nullptr) return 0;
    int rv = 1;
    while (auto cj = e->to<IR::LAnd>()) {
        rv += conjuncts(cj->right);  // should always be 1 if canonicalized
        e = cj->left; }
    return rv;
}

/* reorder gateway rows such that other optimizations are more effective.  When a sequence
 * of rows all have the same action, we can freely reorder them without affecting the result of
 * the program, so we sort them to have those with the fewest conjuncts first, as that is more
 * likely to allow removing redundant rows.
 */
void CanonGatewayExpr::sortGatewayRows(safe_vector<GWRow_t> &rows) {
    auto s = rows.begin(), e = rows.begin();
    while (s != rows.end()) {
        while (e != rows.end() && e->second == s->second) ++e;
        std::stable_sort(s, e, [](const GWRow_t &a, const GWRow_t &b) {
            return conjuncts(a.first) < conjuncts(b.first); });
        s = e; }
}

void CanonGatewayExpr::splitGatewayRows(safe_vector<GWRow_t> &rows) {
    /* split logical-OR operations across rows */
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        LOG3("    " << it->first << " -> " << it->second);
        while (auto *e = dynamic_cast<const IR::LOr *>(it->first)) {
            auto act = it->second;
            it->first = e->left;
            it = rows.emplace(++it, e->right, act);
            --it; } }
    if (rows.back().first)
        rows.emplace_back(nullptr, cstring());
}

void CanonGatewayExpr::removeNotEquals(safe_vector<GWRow_t> &rows) {
    /* This function does not actuall remove != operations -- it just rearranges the rows
     * and inserts ! operations such that recanonicalization should remove (or at least
     * reduce) the number of != operations.
     * @returns 'true' if it made some changes and the code needs to be recanonicalized
     * @returns 'false' if there were no != that need to be removed
     */
    std::deque<GWRow_t> need_negate;
    /* move things that need negation to the end and negate them */
    for (auto it = rows.begin(); it != rows.end()-1;) {
        if (!it->first)
            break;
        if (NeedNegate(it->first)) {
            need_negate.push_back(*it);
            it = rows.erase(it);
        } else {
            for (auto &n : need_negate) {
                if (n.second != it->second)
                    it->first = new IR::LAnd(it->first, new IR::LNot(n.first)); }
            ++it; } }
    while (!need_negate.empty()) {
        auto &back = rows.back();
        for (auto &n : need_negate) {
            if (n.second == back.second) continue;
            if (back.first)
                back.first = new IR::LAnd(back.first, new IR::LNot(n.first));
            else
                back.first = new IR::LNot(n.first); }
        if (back.first)
            rows.emplace_back(nullptr, need_negate.back().second);
        need_negate.pop_front(); }
}

const IR::Node *CanonGatewayExpr::postorder(IR::MAU::Table *tbl) {
    auto &rows = tbl->gateway_rows;
    if (rows.empty() || !rows[0].first)
        return tbl;
    LOG2("CanonGateway on table " << tbl->name);
    LOG1("Postorder gateway cond: " << tbl->gateway_cond);
    removeUnusedRows(tbl, false);

    if (rows.empty() || !rows[0].first) {
        if (tbl->conditional_gateway_only()) {
            LOG3("eliminating completely dead gateway-only table " << tbl->name);
            if (!rows.empty() && tbl->next.count(rows.front().second))
                return &tbl->next.at(rows.front().second)->tables;
            return nullptr; }
        return tbl; }

    /* The process for removing != operations doesn't always work immediately -- it might
     * require a second pass to get rid of more != operations.  More passes could be done,
     * but its likely if 2 isn't enough, its in a state where repeating more just expands
     * things repeatedly without actually making any progress */
    for (int trial = 0;; ++trial) {
        splitGatewayRows(rows);
        sortGatewayRows(rows);
        removeUnusedRows(tbl, true);
        if (LOGGING(4)) {
            LOG4("gateway " << tbl->name << " after canonicalization trial=" << trial);
            for (auto &r : rows) LOG4("  " << r.first << " -> " << r.second); }
        if (!NeedNegate(rows))
            return tbl;
        if (trial >= 2 || rows.size() > 32) {
            error("%sgateway condition too complex", rows.front().first->srcInfo);
            return tbl; }
        removeNotEquals(rows);
        /* reprocess this gateway to re-canonicalize it. */
        if (LOGGING(4)) {
            LOG4("reprocess gateway " << tbl->name << " trial=" << trial);
            for (auto &r : rows) LOG4("  " << r.first << " -> " << r.second); }
        for (auto &row : rows)
            visit(row.first); }
}

bool CollectGatewayFields::preorder(const IR::MAU::Table *tbl) {
    unsigned row = 0;
    BUG_CHECK(info.empty() && !this->tbl , "reusing CollectGatewayFields");
    if (tbl->uses_gateway())
        LOG5("CollectGatewayFields for table " << tbl->name);
    this->tbl = tbl;
    for (auto &gw : tbl->gateway_rows) {
        if (++row > row_limit)
            return false;
        visit(gw.first, "gateway_row"); }
    return false;
}

bool CollectMatchFieldsAsGateway::preorder(const IR::MAU::Table *tbl) {
    LOG5("CollectMatchFieldsAsGateway for table " << tbl->name);
    if (tbl->uses_gateway() || !tbl->entries_list ||
        tbl->entries_list->entries.size() > Device::gatewaySpec().ExactShifts + 0U ||
        tbl->entries_list->entries.size() > Device::gatewaySpec().MaxRows + 0U) {
        // FIXME -- could deal with more entries by splitting the gateway afterwards
        // SplitComplexGateways will do that if it is run
        fail = true;
    } else {
        for (auto key : tbl->match_key) {
            if (!key->for_match()) {
                fail = true;
                break; }
            if (key->for_range()) {
                // FIXME -- could deal with this, but need update to expression code below
                fail = true;
                break; }
            visit(key->expr, "match_key"); }
    }
    return false;
}

// TOFINO1-ONLY
bool CollectGatewayFields::preorder(const IR::MAU::TypedPrimitive *prim) {
    if (prim->name != "is_validated")
        error("%scondition too complex", prim->srcInfo);
    le_bitrange bits;
    auto e = prim->operands.at(0);
    auto finfo = phv.field(e, &bits);
    if (!finfo) return false;
    info_t &info = this->info[{finfo, bits}];
    info.valid_bit = true;
    return false;
}
bool CollectGatewayFields::preorder(const IR::Expression *e) {
    std::optional<cstring> aliasSourceName = phv.get_alias_name(e);
    le_bitrange bits;
    auto finfo = phv.field(e, &bits);
    if (!finfo) return true;
    info_t &info = this->info[{finfo, bits}];
    const Context *ctxt = nullptr;
    if (findContext<IR::RangeMatch>()) {
        info.need_range = need_range = true;
    } else if (auto *rel = findContext<IR::Operation::Relation>(ctxt)) {
        if (!rel->is<IR::Equ>() && !rel->is<IR::Neq>()) {
            info.need_range = need_range = true;
        } else if (ctxt->child_index > 0) {
            if (xor_match)
                info.xor_with.insert(xor_match);
            else
                info.const_eq = true;
        } else {
            xor_match = PHV::FieldSlice(finfo, bits); }
    } else {
        info.const_eq = true; }
    if (aliasSourceName != std::nullopt) {
        info_to_uses[&info] = *aliasSourceName;
        LOG5("Adding entry to info_to_uses: " << &info << " : " << *aliasSourceName); }
    return false;
}

void CollectGatewayFields::postorder(const IR::Literal *) {
    if (xor_match.field() && getParent<IR::Operation::Relation>())
        info[xor_match].const_eq = true;
}

bool CollectGatewayFields::compute_offsets() {
    if (info.empty()) return true;
    LOG5("CollectGatewayFields::compute_offsets" << DBPrint::Brief);
    LOG7(*this);
    auto &gws = Device::gatewaySpec();
    bytes = bits = 0;
    std::vector<decltype(info)::value_type *> sort_by_size;
    for (auto &field : info) {
        sort_by_size.push_back(&field);
        field.second.offsets.clear();
        field.second.xor_offsets.clear(); }
    std::sort(sort_by_size.begin(), sort_by_size.end(),
              [](decltype(info)::value_type *a, decltype(info)::value_type *b) -> bool {
                  return a->first.size() > b->first.size(); });
    // track where each container byte is located on the gateway input
    std::map<std::pair<PHV::Container, int>, std::set<int>>       alloc_bytes;
    std::map<std::pair<std::pair<PHV::Container, int>, int>, std::pair<PHV::Container, int>>
        xor_pairs;

    PHV::FieldUse use_read(PHV::FieldUse::READ);

    for (auto &i : this->info) {
        auto &field = i.first;
        auto &info = i.second;
        for (auto &xor_with : info.xor_with) {
            auto &with = this->info[xor_with];
            int shift = field.range().lo - xor_with.range().lo;

            // Collect all the bytes corresponding to the field
            std::vector<std::pair<PHV::Container, int>> field_bytes;
            field.foreach_byte(tbl, &use_read, [&](const PHV::AllocSlice &sl) {
                field_bytes.push_back(sl.container_byte());
            });

            // Walk through the bytes being xored
            int field_byte_idx = 0;
            xor_with.foreach_byte(tbl, &use_read, [&](const PHV::AllocSlice &sl) {
                auto alloc_byte = sl.container_byte();
                auto field_byte = field_bytes[field_byte_idx];
                bool duplicate = false;
                if (alloc_bytes.count(alloc_byte)) {
                    for (int byte : alloc_bytes[alloc_byte]) {
                        auto source = std::make_pair(alloc_byte, byte);
                        if (xor_pairs.count(source) && xor_pairs[source] == field_byte) {
                            duplicate = true;
                            break;
                        }
                    }
                }
                auto bit = (duplicate ? *alloc_bytes.at(alloc_byte).begin() : bytes) * 8U +
                           sl.container_slice().lo % 8U;
                with.offsets.emplace_back(bit, sl.field_slice());
                info.xor_offsets.emplace_back(bit, sl.field_slice().shiftedByBits(shift));
                LOG5("  " << (duplicate ? "duplicate " : "") << "byte " << (bit/8) << " " <<
                     field << "(" << info.xor_offsets.back().second << ") xor " << xor_with <<
                     ' ' << sl << " (" << with.offsets.back().second << ")");
                if (!duplicate) {
                    alloc_bytes[alloc_byte].insert(bytes);
                    auto source = std::make_pair(alloc_byte, bytes);
                    xor_pairs[source] = field_byte;
                    ++bytes; }
                ++field_byte_idx;
            }); } }
    // we collect slices that could be allocated to either bytes or bits here, then
    // allocate the largest ones to the bytes and then try to pack any remaining ones
    // into the bits
    struct need_alloc_t {
        const PHV::FieldSlice   *field;
        info_t                  *info;
        PHV::AllocSlice         slice;    // need a copy as the ref from foreach_byte is local
    };
    std::vector<need_alloc_t>                   need_alloc;
    for (auto *it : sort_by_size) {
        const PHV::FieldSlice &field = it->first;
        info_t &info = it->second;
        if (!info.need_range && !info.const_eq) continue;
        int size = field.is_unallocated() ? (field.size() + 7)/8U : field.container_bytes();
        bitvec field_bits(field.range().lo, field.size());  // bits of the field needed
        if (auto ixbar = dynamic_cast<const Tofino::IXBar::Use *>(this->ixbar)) {
            for (auto &f : ixbar->bit_use) {
                if (f.field == field.field()->name && field.range().overlaps(f.lo, f.hi())) {
                    // Portions of the field could be in the hash vs in the search bus, and
                    // this is only for the hash
                    auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                                        (field.range().intersectWith(f.lo, f.hi()));
                    BUG_CHECK(boost_sl != std::nullopt, "Gateway field cannot find correct "
                              "overlap in input xbar");
                    le_bitrange b = *boost_sl;
                    info.offsets.emplace_back(f.bit + b.lo - f.lo + 32, b);
                    LOG5("  bit " << f.bit + b.lo - f.lo + 32 << " " << field);
                    field_bits.clrrange(b.lo, b.size());
                    if (f.bit + b.hi - f.lo >= bits)
                        bits = f.bit + b.hi - f.lo + 1; } }
            if (field_bits.empty()) continue; }
        bool alloc_in_bits = info.need_range;
        if (!alloc_in_bits && size == 1) {
            if (bytes+size > gws.PhvBytes && bits+field.size() <= gws.HashBits) {
                // if we've run out of bytes and there are available hash bits, try using them
                alloc_in_bits = true; } }
        if (alloc_in_bits) {
            BUG_CHECK(field_bits.ffz(field.range().lo) == size_t(field.range().hi + 1),
                      "field only partly in hash needed all in hash");
            info.offsets.emplace_back(bits + 32, field.range());
            field_bits.clrrange(field.range().lo, field.size());
            LOG5("  bit " << bits + 32 << " " << field);
            bits += field.size();
        } else {
            field.foreach_byte(tbl, &use_read, [&](const PHV::AllocSlice &sl) {
                if (size_t(field_bits.ffs(sl.field_slice().lo)) > size_t(sl.field_slice().hi)) {
                    LOG5(sl << " already done via hash");
                    return; }
                auto alloc_byte = sl.container_byte();
                bool add_to_need_alloc = false;
                if (alloc_bytes.count(alloc_byte)) {
                    for (auto byte : alloc_bytes.at(alloc_byte)) {
                        auto bit = byte * 8U + sl.container_slice().lo % 8U;
                        if (std::find_if(info.offsets.begin(), info.offsets.end(),
                                 [&](const std::pair<int, le_bitrange> &offset) {
                                     return le_bitinterval(offset.first,
                                                           offset.first + offset.second.size())
                                         .overlaps(bit, bit + sl.container_slice().size());
                                 }) != info.offsets.end()) {
                            add_to_need_alloc = true;
                        } else {
                            info.offsets.emplace_back(bit, sl.field_slice());
                            LOG5("  duplicate byte " << (bit/8) << " " << field << ' ' << sl);
                        }
                    }
                } else {
                    add_to_need_alloc = true; }
                if (add_to_need_alloc) {
                    need_alloc.push_back({ &field, &info, sl }); }
                field_bits.clrrange(sl.field_slice().lo, sl.width()); }); }
#if 0
        // FIXME -- should check this, but sometimes this gets run when PHV allocation has failed
        // to allocate this field, in which case we don't want to crash here.
        BUG_CHECK(field_bits.empty(), "failed to cover all of %s in gateway", field);
#endif
    }

    // TOFINO1-ONLY: handle implicit container valid bit
    if (Device::currentDevice() == Device::TOFINO) {
        for (auto *it : sort_by_size) {
            const PHV::FieldSlice &field = it->first;
            info_t &info = it->second;
            if (!info.valid_bit) continue;
            LOG1("handle valid bit for field slice: " << field);
            info.offsets.emplace_back(bits + 32, le_bitrange(0, 0));
            LOG5("  bit " << bits + 32 << " " << field);
            bits += 1;
        } }

    std::stable_sort(need_alloc.begin(), need_alloc.end(),
                     [](const need_alloc_t &a, const need_alloc_t &b) {
                             return a.slice.width() > b.slice.width(); });
    for (need_alloc_t &n : need_alloc) {
        if (bytes < gws.PhvBytes) {
            auto bit = (bytes * 8U) + (n.slice.container_slice().lo % 8U);
            n.info->offsets.emplace_back(bit, n.slice.field_slice());
            LOG5("  byte " << bytes << ' ' << *n.field << ' ' << n.slice);
            ++bytes;
        } else {
            n.info->offsets.emplace_back(32 + bits, n.slice.field_slice());
            LOG5("  bit " << bits << ' ' << *n.field << ' ' << n.slice);
            bits += n.slice.width(); } }

    LOG6("CollectGatewayFields::compute_offsets finished " << *this << DBPrint::Reset);
    if (bytes > gws.PhvBytes) return false;
    if (bits > gws.HashBits) return false;
    return bits <= Tofino::IXBar::get_hash_single_bits();
}


/** SetupRanges: convert IR::Operation::Relation expressions into IR::RangeMatch operations
 * for encoding in a gateway DirtCAM.  A Relation just a </<=/>/>=/==/!= comparison.  TCAMs
 * cannot match for >/< type things, so they must be converted to RangeMatch expression.
 * ==/!= can be matched directly in the lower part of the gateway, or can be converted to
 * RangeMatches for use in the upper part when we are using DirtCAMs.
 *
 * An IR::RangeMatch expression more or less directly represents a DirtCAM lookup of up to
 * 4 bits.  As an expression, it has two operands; the left operand is of type bit<k> or
 * int<k> while the right operand must a constant of type bit<2**k>.  The left operand is
 * used as an index to select a single bit from the right operand constant, which will be
 * the result of the expression (a bit<1> or bool value).  When the left operand is unsigned
 * this is straightforward, but for a signed valuem we use two's complement, so the upper half
 * of the bits in the constant are the bits for negative input values.
 *
 * Since Tofino DirtCAMs are limited to 4 bit input, we also break up wider comparisions into
 * 4 bit chunks -- we first split off the top bits%4 bits into a small RangeMatch, then split
 * the rest into 4 bit RangeMatches.
 */
class GatewayRangeMatch::SetupRanges : public Transform {
    const PhvInfo               &phv;
    const CollectGatewayFields  &fields;
    IR::MAU::Action *preorder(IR::MAU::Action *af) override { prune(); return af; }
    IR::P4Table *preorder(IR::P4Table *t) override { prune(); return t; }
    IR::Attached *preorder(IR::Attached *a) override { prune(); return a; }
    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *s) override { prune(); return s; }

    const IR::Expression *postorder(IR::Operation::Relation *rel) override {
        le_bitrange bits;
        auto f = phv.field(rel->left, &bits);
        bool sign = isSigned(rel->left->type);
        if (!f || !rel->right->is<IR::Constant>() || !fields.info.count({f, bits})) return rel;
        LOG3("SetupRange for " << rel);
        bool forceRange = !(rel->is<IR::Equ>() || rel->is<IR::Neq>());
        bool reverse = (rel->is<IR::Geq>() || rel->is<IR::Neq>());
        auto info = fields.info.at({f, bits});
        int64_t val = rel->right->to<IR::Constant>()->asInt64();
        BUG_CHECK(!forceRange || sign || (val & 1), "Non-canonicalized range value");
        int base = 0;
        int orig_lo = bits.lo;
        for (auto &alloc : info.offsets) {
            if (alloc.first < 32) {
                if (!forceRange && alloc.second.hi <= bits.lo) {
                    val >>= (bits.lo - alloc.second.hi + 1);
                    bits.lo = alloc.second.hi + 1;
                    if (bits.lo > bits.hi) {
                        LOG4("  all bits in lower 32, skipping");
                        return rel; } }
                continue; }
            if (!alloc.second.overlaps(bits)) continue;
            BUG_CHECK(base == 0, "bits for %s split in range match", f->name);
            base = alloc.first + bits.lo - alloc.second.lo;
            BUG_CHECK(base >= 32 && base + bits.size() <= alloc.first + alloc.second.size(),
                      "bad gateway field layout for range match");
            break; }
        if (!base) return rel;
        if (base & 3) {
            bits.lo -= base & 3;
            val <<= base & 3;
            base &= ~3; }
        bits.lo -= orig_lo;
        bits.hi -= orig_lo;
        const IR::Expression *rv = nullptr, *himatch = nullptr;
        LOG5("  bits = " << bits);
        for (int lo = ((bits.hi - bits.lo) & ~3) + bits.lo; lo >= bits.lo; lo -= 4) {
            int hi = std::min(lo + 3, bits.hi);
            int size = hi - lo + 1;  // size of nibble in bits
            unsigned mask = (1U << (1U << size)) - 1;
            unsigned data = 1U << ((val >> (lo - bits.lo)) & ((1U << size) - 1));
            // data is now a 1-hot bitvector for the value we're comparing against (suitable
            // for use as the right operand of a RangeMatch
            const IR::Expression *eq, *c;
            eq = new IR::RangeMatch(MakeSlice(rel->left, std::max(0, lo), hi), data);
            if (forceRange) {
                // doing a less-than or greater-equal comparison, so convert 'data` to have
                // one bits in all the bits that correspind to values less than the value
                // being matched.  For unsigned, that is easy (just subtract 1), but for
                // signed it is more complex.
                unsigned minval = 1;
                if (sign) {
                    minval = 1U << (1U << (size - 1));
                    if (minval > data) {
                        minval++;
                        data += mask + 1; } }
                data -= minval; }
            BUG_CHECK(data <= mask, "bit snafu in RangeMatch");
            if (reverse) {
                data ^= mask;
                if (forceRange && lo != bits.lo) {
                    if (sign)  {
                        // rotate 2^size bits left by 1
                        data = ((data << 1) | (data >> ((1 << size) - 1))) & mask;
                        // clear the bit corresponding to most negative value
                        data &= ~(1U << (1U << (size - 1)));
                    } else {
                        data = (data << 1) & mask; } } }
            c = new IR::RangeMatch(MakeSlice(rel->left, std::max(0, lo), hi), data);
            if (forceRange)
                c = himatch ? new IR::LAnd(himatch, c) : c;
            if (rv) {
                if (forceRange || reverse)
                    rv = new IR::LOr(rv, c);
                else
                    rv = new IR::LAnd(rv, c);
            } else {
                rv = c; }
            himatch = himatch ? new IR::LAnd(himatch, eq) : eq;
            sign = false; }
        LOG3("SetupRange result: " << rv);
        return rv;
    }

 public:
    SetupRanges(const PhvInfo &phv, const CollectGatewayFields &fields)
    : phv(phv), fields(fields) {}
};

void GatewayRangeMatch::postorder(IR::MAU::Table *tbl) {
    CollectGatewayFields collect(phv);
    tbl->apply(collect);
    if (!collect.need_range) return;
    if (!collect.compute_offsets()) return;
    SetupRanges setup(phv, collect);
    for (auto &gw : tbl->gateway_rows)
        if (gw.first) gw.first = gw.first->apply(setup);
}

bool CheckGatewayExpr::preorder(const IR::MAU::Table *tbl) {
    CollectGatewayFields collect(phv);
    tbl->apply(collect);
    if (!collect.compute_offsets()) {
        char tmp[32] = { 0 };
        if (Device::gatewaySpec().HashBits > 0)
            snprintf(tmp, sizeof tmp, " + %d bits", Tofino::IXBar::get_hash_single_bits());
        error("%s: condition too complex, limit of %d bytes%s of PHV input exceeded",
              tbl->srcInfo, Device::gatewaySpec().PhvBytes, tmp); }
    return true;
}

bool CheckGatewayExpr::preorder(const IR::Expression *e) {
    if (e->is<IR::MAU::TypedPrimitive>())
        return false;
    if (!phv.field(e))
        error("%s: condition expression too complex", e->srcInfo);
    return false;
}

bool CheckGatewayExpr::needConstOperand(const IR::Operation::Binary *e) {
    if (!e->right->is<IR::Constant>()) {
        error("condition too complex, one operand of %s must be constant", e);
        return false; }
    return true;
}

BuildGatewayMatch::BuildGatewayMatch(const PhvInfo &phv, CollectGatewayFields &f)
: phv(phv), fields(f) {
    // FIXME -- we need to shift down the match so bit 0 of the match is the lowest
    // valid bit in the match: format.  This is to match up with the hack in the assembler
    // to ignore all bits that are not part of the match: format.  See FIXME in
    // GatewayTable::pass1 in the assembler
    shift = INT_MAX;    // lowest bit being matched
    maxbit = 0;         // one more than highest bit being matched
    for (auto &info : fields.info) {
        for (auto &off : info.second.offsets) {
            if (off.first < shift)
                shift = off.first;
            if (off.first + off.second.size() > maxbit)
                maxbit = off.first + off.second.size();
        }
    }
}

Visitor::profile_t BuildGatewayMatch::init_apply(const IR::Node *root) {
    LOG3("BuildGatewayMatch for " << root);
    match.setwidth(0);  // clear out old value
    if (fields.need_range || !fields.bits) {
        if (fields.bytes)
            match.setwidth(maxbit - shift);
    } else {
        match.setwidth(32 + fields.bits - shift); }
    range_match.clear();
    match_field = {};
    andmask = UINT64_MAX;
    ormask = 0;
    return Inspector::init_apply(root);
}

bool BuildGatewayMatch::preorder(const IR::Expression *e) {
    le_bitrange bits;
    auto field = phv.field(e, &bits);
    if (!field)
        BUG("Unhandled expression in BuildGatewayMatch: %s", e);
    if (!match_field) {
        match_field = { field, bits };
        LOG4("  match_field = " << field->name << ' ' << bits);
        if (bits.size() == 1 && !findContext<IR::Operation::Relation>()) {
            auto &match_info = fields.info.at(match_field);
            LOG4("  match_info = " << match_info);
            for (auto &off : match_info.offsets) {
                if (getParent<IR::LNot>())
                    match.word1 &= ~(UINT64_C(1) << off.first >> shift);
                else
                    match.word0 &= ~(UINT64_C(1) << off.first >> shift); }
            match_field = {}; }
    } else {
        LOG4("  xor_field = " << field->name << ' ' << bits);
        auto &gws = Device::gatewaySpec();
        // FIXME -- could/should call `constant(0)` here to do a compare with 0 for the xor?
        // avoid duplicating all this code;
        size_t size = std::max(static_cast<int>(bits.size()), match_field.size());
        big_int mask = ~(~big_int(0) << size), donemask = 0;
        big_int val = cmplmask & mask;
        mask &= andmask & ~ormask;
        mask <<= match_field.range().lo;
        val <<= match_field.range().lo;
        auto *field_info = &fields.info.at({field, bits});
        auto *match_info = &fields.info.at(match_field);
        if (match_info->offsets.empty() && !match_info->xor_offsets.empty())
            std::swap(field_info, match_info);
        LOG4("  match_info = " << *match_info << ", mask=0x" << std::hex << mask << std::dec <<
             " shift=" << shift);
        LOG4("  xor_match_info = " << *field_info);
        auto it = field_info->xor_offsets.begin();
        auto end = field_info->xor_offsets.end();
        auto orig_mask = mask;
        big_int upper_bit_mask = 0;
        for (auto &off : match_info->offsets) {
            while (it != end && it->first < off.first) ++it;
            if (it == end) break;
            if (off.first < it->first) continue;
            if (it->first != off.first || it->second.size() != off.second.size()) {
                BUG("field equality comparison misaligned in gateway"); }
            big_int elmask = ~(~big_int(0) << off.second.size()) << off.second.lo;
            if ((elmask &= mask) == 0) continue;
            if (off.first/8 < gws.PerByteMatch) {
                if (!check_per_byte_match(off, mask, val)) continue;
                mask &= ~elmask;
            } else {
                upper_bit_mask |= elmask; }
            int shft = off.first - off.second.lo - shift;
            LOG6("    elmask=0x" << std::hex << elmask << std::dec << " shft=" << shft);
            if (shft >= 0) {
                match.word0 &= ~(val << shft) | ~(elmask << shft);
                match.word1 &= (val << shft) | ~(elmask << shft);
            } else {
                match.word0 &= ~(val >> -shft) | ~(elmask >> -shft);
                match.word1 &= (val >> -shft) | ~(elmask >> -shft); }
            LOG6("    match now " << match);
            donemask |= elmask;
            if (++it == end) break; }
        const IR::Expression *tmp;
        BUG_CHECK(orig_mask == donemask, "failed to match all bits of %s",
                  (tmp = findContext<IR::Operation::Relation>()) ? tmp : e);
        BUG_CHECK((mask ^ upper_bit_mask) << shift == 0, "Didn't cover all bytes");
        match_field = {}; }
    return false;
}

//  TOFINO1-ONLY
bool BuildGatewayMatch::preorder(const IR::MAU::TypedPrimitive *prim) {
    if (Device::currentDevice() != Device::TOFINO)
        return false;
    if (prim->name != "is_validated")
        error("%sunsupported primitive %s", prim->srcInfo, prim->name);
    le_bitrange bits;
    auto e = prim->operands.at(0);
    auto field = phv.field(e, &bits);
    if (!field)
        BUG("Unhandled expression in BuildGatewayMatch: %s", e);
    match_field = {field, bits};
    auto &match_info = fields.info.at(match_field);
    for (auto &off : match_info.offsets) {
        if (getParent<IR::LNot>())
            match.word1 &= ~(UINT64_C(1) << off.first >> shift);
        else
            match.word0 &= ~(UINT64_C(1) << off.first >> shift);
    }
    match_field = {};
    return false;
}

/** Unused */
bool BuildGatewayMatch::check_per_byte_match(const std::pair<int, le_bitrange> &byte,
                                             big_int mask, big_int val) {
    BUG_CHECK((byte.first % 8) + byte.second.size() <= 8, "match crosses byte boundary");
    int byte_idx = byte.first / 8;
    int shft = (byte.second.lo & -8) + (byte.first % 8);
    unsigned bmask = unsigned(mask >> shft) & 0xff;
    unsigned bval = unsigned(val >> shft) & bmask;
    BUG_CHECK(bmask != 0, "no bits in the identified byte?");
    match_t &bmatch = byte_matches.emplace(byte_idx, match_t::dont_care(8)).first->second;
    if ((~bval & bmask & ~bmatch.word0) || (bval & bmask & ~bmatch.word1)) {
        LOG6("    skip byte " << (byte.first/8) << " " << match_t(8, bval, bmask) << " " << bmatch);
        return false; }
    bmatch.word0 &= ~bval;
    bmatch.word1 &= ~bmask | bval;
    return true;
}

void BuildGatewayMatch::constant(big_int c) {
    auto ctxt = getContext();
    auto &gws = Device::gatewaySpec();
    if (ctxt->node->is<IR::BAnd>()) {
        andmask = c;
        LOG4("  andmask = 0x" << std::hex << andmask << std::dec);
    } else if (ctxt->node->is<IR::BOr>()) {
        ormask = c;
        LOG4("  ormask = 0x" << std::hex << ormask << std::dec);
    } else if (match_field) {
        big_int mask = ~(~big_int(0) << match_field.size());
        big_int val = (c ^ cmplmask) & mask;
        if ((val & mask & ~andmask) || (~val & mask & ormask))
            warning("%smasked comparison in gateway can never match", ctxt->node->srcInfo);
        mask &= andmask & ~ormask;
        mask <<= match_field.range().lo;
        val <<= match_field.range().lo;
        auto &match_info = fields.info.at(match_field);
        LOG4("  match_info = " << match_info << ", val=" << val << " mask=0x" << std::hex <<
             mask << std::dec << " shift=" << shift);
        big_int upper_bits_mask = 0;   // bits being matched that don't require per-bye sharing
        for (auto &off : match_info.offsets) {
            big_int elmask = ~(~big_int(0) << off.second.size()) << off.second.lo;
            if ((elmask &= mask) == 0) continue;
            int shft = off.first - off.second.lo - shift;
            if (off.first/8 < gws.PerByteMatch) {
                if (!check_per_byte_match(off, mask, val)) continue;
                mask &= ~elmask;
            } else {
                upper_bits_mask |= elmask; }
            LOG6("    elmask=0x" << std::hex << elmask << std::dec << " shft=" << shft);
            if (shft >= 0) {
                match.word0 &= ~(val << shft) | ~(elmask << shft);
                match.word1 &= (val << shft) | ~(elmask << shft);
            } else {
                match.word0 &= ~(val >> -shft) | ~(elmask >> -shft);
                match.word1 &= (val >> -shft) | ~(elmask >> -shft); }
            LOG6("    match now " << match); }
        BUG_CHECK((mask ^ upper_bits_mask) << shift == 0, "Didn't cover all bytes");
        match_field = {};
    } else {
        BUG("Invalid context for constant in BuildGatewayMatch"); }
}

bool BuildGatewayMatch::preorder(const IR::Equ *) {
    match_field = {};
    andmask = -1;
    ormask = cmplmask = 0;
    return true;
}

bool BuildGatewayMatch::preorder(const IR::Neq *neq) {
    if (safe_width_bits(neq->left->type) != 1)
        return preorder(static_cast<const IR::Operation_Relation *>(neq));
    /* we can only handle 1-bit neq here */
    match_field = {};
    andmask = cmplmask = -1;
    ormask = 0;
    return true;
}

bool BuildGatewayMatch::preorder(const IR::RangeMatch *rm) {
    le_bitrange bits;
    auto field = phv.field(rm->expr, &bits);
    BUG_CHECK(field, "invalid RangeMatch in BuildGatewayMatch");
    int unit = -1;
    for (auto &alloc : fields.info.at({field, bits}).offsets) {
        if (alloc.first < 32 || !alloc.second.overlaps(bits))
            continue;
        unit = (alloc.first + bits.lo - alloc.second.lo - 32) / 4;
        BUG_CHECK(unit == (alloc.first + bits.hi - alloc.second.lo - 32) / 4,
                  "RangeMatch source (%s) misaligned at %d..%d", rm->expr,
                  alloc.first + bits.lo - alloc.second.lo, alloc.first + bits.lo - alloc.second.lo);
        break; }
    BUG_CHECK(unit >= 0 && unit < 3, "invalid RangeMatch unit %d", unit);
    LOG4("RangeMatch " << rm->expr << " unit=" << unit << " size=" << bits.size() <<
         " data=0x" << hex(rm->data));
    while (range_match.size() <= size_t(unit))
        range_match.push_back(0xffff);
    unsigned val = rm->data;
    int size = bits.size();
    if (size < 2) {
        val &= 0x3;
        val |= val << 2; }
    if (size < 3) {
        val &= 0xf;
        val |= val << 4; }
    if (size < 4) {
        val &= 0xff;
        val |= val << 8; }
    range_match.at(unit) &= val;
    return false;
}

GatewayOpt::GatewayOpt(const PhvInfo &phv) : PassManager {
    new PassRepeated {
        new CanonGatewayExpr,
        new SplitComplexGateways(phv),
        new GatewayRangeMatch(phv) },
    new CheckGatewayExpr(phv)
} {}
