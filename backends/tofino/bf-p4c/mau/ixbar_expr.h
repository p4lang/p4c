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

#ifndef BF_P4C_MAU_IXBAR_EXPR_H_
#define BF_P4C_MAU_IXBAR_EXPR_H_

#include "bf-p4c/common/utils.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "boost/range/adaptor/reversed.hpp"
#include "lib/bitvec.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

/** Functor to test if an expression can be computed in the ixbar & hash matrixes */
class CanBeIXBarExpr : public Inspector {
    static constexpr int MAX_HASH_BITS = 52;

    static int get_max_hash_bits() {
        // If hash parity is enabled reserve a bit for parity
        if (!BackendOptions().disable_gfm_parity)
            return MAX_HASH_BITS - 1;
        return MAX_HASH_BITS;
    }

    bool        rv = true;
    // FIXME -- if we want to run this *before* SimplifyReferences has converted all
    // PathExpressions into the referred to object, we need some way of resolving what the
    // path expressions refer to, or at least whether they refer to something that can be
    // accessed in the ixbar.  So we use this function on a per-use basis.  The default
    // implementation throws a BUG if ever called (as normally we're after SimplifyReferences
    // and all PathExpressions are gone), but in the case where we're before that (calling
    // from CreateSaluInstruction called from AttachTables called from extract_maupipe),
    // we use a functor that can tell a local of the RegisterAction (which can't be in the
    // ixbar) from anything else (which is assumed to be a header or metadata instance.)
    // If we ever clean up the frontend reference handling so we no longer need to resolve
    // names via the refmap, this can go away.  Or if we could do SimplifyRefernces before
    // trying to build StatefulAlu instructions.
    std::function<bool(const IR::PathExpression *)> checkPath;

    static const IR::Type_Extern *externType(const IR::Type *type) {
        if (auto *spec = type->to<IR::Type_SpecializedCanonical>())
            type = spec->baseType;
        return type->to<IR::Type_Extern>(); }

    profile_t init_apply(const IR::Node *n) {
        auto *expr = n->to<IR::Expression>();
        BUG_CHECK(expr, "CanBeIXBarExpr called on non-expression");
        rv = expr->type->width_bits() <= get_max_hash_bits();
        return Inspector::init_apply(n); }
    bool preorder(const IR::Node *) { return false; }  // ignore non-expressions
    bool preorder(const IR::PathExpression *pe) {
        if (!checkPath(pe)) rv = false;
        return false; }
    bool preorder(const IR::Constant *) { return false; }
    bool preorder(const IR::Member *m) {
        auto *base = m->expr;
        while ((m = base->to<IR::Member>())) base = m->expr;
        if (auto *pe = base->to<IR::PathExpression>()) {
            if (!checkPath(pe)) rv = false;
        } else if (base->is<IR::HeaderRef>() || base->is<IR::TempVar>()) {
            // ok
        } else {
            rv = false; }
        return false; }
    bool preorder(const IR::TempVar *) { return false; }
    bool preorder(const IR::Slice *) { return rv; }
    bool preorder(const IR::Concat *) { return rv; }
    bool preorder(const IR::Cast *) { return rv; }
    bool preorder(const IR::BFN::SignExtend *) { return rv; }
    bool preorder(const IR::BFN::ReinterpretCast *) { return rv; }
    bool preorder(const IR::BXor *) { return rv; }
    bool preorder(const IR::BAnd *e) {
        if (!e->left->is<IR::Constant>() && !e->right->is<IR::Constant>()) rv = false;
        return rv; }
    bool preorder(const IR::BOr *e) {
        if (!e->left->is<IR::Constant>() && !e->right->is<IR::Constant>()) rv = false;
        return rv; }
    bool preorder(const IR::MethodCallExpression *mce) {
        if (auto *method = mce->method->to<IR::Member>()) {
            if (auto *ext = externType(method->type)) {
                if (ext->name == "Hash" && method->member == "get") {
                    return false; } } }
        return rv = false; }
    // any other expression cannot be an ixbar expression
    bool preorder(const IR::Expression *) { return rv = false; }

 public:
    explicit CanBeIXBarExpr(const IR::Expression *e,
                            std::function<bool(const IR::PathExpression *)> checkPath =
        [](const IR::PathExpression *)->bool { BUG("Unexpected path expression"); })
    : checkPath(checkPath) { e->apply(*this); }
    operator bool() const { return rv; }
};


/* FIXME -- this should be a constructor of bitvec in the open source code */
inline bitvec to_bitvec(big_int v) {
    bitvec rv(static_cast<uintptr_t>(v));
    v >>= sizeof(uintptr_t) * CHAR_BIT;
    if (v > 0)
        rv |= to_bitvec(v) << (sizeof(uintptr_t) * CHAR_BIT);
    return rv;
}

/** Functor to compute the constant (seed) part of an IXBar expression */
class IXBarExprSeed : public Inspector {
    le_bitrange slice;
    int         shift = 0;
    bitvec      rv;

    bool preorder(const IR::Annotation *) { return false; }
    bool preorder(const IR::Type *) { return false; }
    bool preorder(const IR::Member *) { return false; }
    bool preorder(const IR::Constant *k) {
        if (getParent<IR::BAnd>()) return false;
        rv ^= to_bitvec((k->value >> slice.lo) & ((big_int(1) << slice.size()) - 1)) << shift;
        return false; }
    bool preorder(const IR::Concat *e) {
        auto tmp = slice;
        int rwidth = e->right->type->width_bits();
        if (slice.lo < rwidth) {
            slice = slice.intersectWith(0, rwidth-1);
            visit(e->right, "right"); }
        if (tmp.hi >= rwidth) {
            slice = tmp;
            slice.lo = rwidth;
            shift += rwidth;
            slice = slice.shiftedByBits(-rwidth);
            visit(e->left, "left");
            shift -= rwidth; }
        slice = tmp;
        return false; }
    bool preorder(const IR::StructExpression *fl) {
        // delegate to the ListExpression case
        IR::ListExpression listExpr(*getListExprComponents(*fl));
        return preorder(&listExpr); }
    bool preorder(const IR::ListExpression *fl) {
        auto tmp = slice;
        auto old_shift = shift;
        for (auto *e : boost::adaptors::reverse(fl->components)) {
            int width = e->type->width_bits();
            if (slice.lo < width) {
                auto t2 = slice;
                slice = slice.intersectWith(0, width-1);
                visit(e);
                slice = t2; }
            if (slice.hi < width) break;
            slice = slice.shiftedByBits(-width);
            shift += width; }
        slice = tmp;
        shift = old_shift;
        return false; }
    bool preorder(const IR::Slice *sl) {
        auto tmp = slice;
        slice = slice.shiftedByBits(-sl->getL());
        int width = sl->getH() - sl->getL() + 1;
        if (slice.size() > width)
            slice.hi = slice.lo + width - 1;
        visit(sl->e0, "e0");
        slice = tmp;
        return false; }
    bool preorder(const IR::MethodCallExpression *mce) {
        BUG("MethodCallExpression not supported in IXBarExprSeed: %s", mce);
        return false; }

 public:
    IXBarExprSeed(const IR::Expression *e, le_bitrange sl) : slice(sl) { e->apply(*this); }
    operator bitvec() const { return rv >> slice.lo; }
};

struct P4HashFunction : public IHasDbPrint {
    safe_vector<const IR::Expression *> inputs;
    le_bitrange hash_bits;
    IR::MAU::HashFunction algorithm;
    cstring dyn_hash_name;
    LTBitMatrix symmetrically_hashed_inputs;
    const IR::Expression *hash_gen_expr = nullptr;
    // FIXME -- we record the expression the hash is derived from so it can be copied to
    // the IXBar::Use::HashDistHash and then output it in the .bfa file.  Perhaps we should
    // compute the (raw) GFM bits here instead?

    bool is_dynamic() const {
       bool rv = !dyn_hash_name.isNull();
       // Symmetric is currently not supported with the dynamic hash library in bf-utils
       if (rv)
           BUG_CHECK(symmetrically_hashed_inputs.empty(), "Dynamically hashed values cannot "
               "have symmetric fields");
       return rv;
    }
    bool equiv_dyn(const P4HashFunction *func) const;
    bool equiv_inputs_alg(const P4HashFunction *func) const;


    void slice(le_bitrange hash_slice);
    cstring name() const { return is_dynamic() ? dyn_hash_name : "static_hash"_cs; }
    int size() const { return hash_bits.size(); }
    bool equiv(const P4HashFunction *) const;
    bool is_next_bit_of_hash(const P4HashFunction *) const;

    bool overlap(const P4HashFunction *, le_bitrange *my_overlap, le_bitrange *hash_overlap) const;
    void dbprint(std::ostream &out) const;
};

/**
 * The purpose of this function is to verify that symmetric hashes are valid and can be allocated.
 * A key can currently be symmetric if it is on either a selector or a hash calculation,
 * and as long as algorithm is a CRC.
 *
 * Currently full string literals are required, perhaps at sometime this can be a function
 * within the tna/t2na files.
 */
bool verifySymmetricHashPairs(
    const PhvInfo &phv,
    safe_vector<const IR::Expression *> &field_list,
    const IR::Annotations *annotations,
    gress_t gress,
    const IR::MAU::HashFunction& hf,
    LTBitMatrix *sym_pairs);

/**
 * The purpose of this function is to convert an Expression into a P4HashFunction, which can
 * be used by the internal algorithms to compare, allocate, and generate JSON.  The hash function
 * is built entirely around the IR::MAU::HashGenExpression, where the HashGenExpression
 * indicate which fields are inputs and the algorithm, and the Slice on the outside can also
 * determine hash_bits
 *
 * FIXME: This pass is an extension of the IXBar::FieldManagement pass, and similar to the
 * IR::MAU::HashGenExpression, the goal would be obsolete the IXBar::FieldManagement pass,
 * which gathers up information about all xbar information for all types of inputs, and
 * gathers a list of functions to allocate
 */
class BuildP4HashFunction : public PassManager {
    P4HashFunction* _func = nullptr;
    const PhvInfo &phv;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        _func = nullptr;
        return rv;
    }

    class InsideHashGenExpr : public MauInspector {
        BuildP4HashFunction &self;
        safe_vector<const IR::Expression *> fields;
        LTBitMatrix sym_fields;

        enum class State {
            OUTSIDE,
            INSIDE,
            FIELD_LIST,
            IXBAR_EXPR,
        } state = State::OUTSIDE;

        Visitor::profile_t init_apply(const IR::Node *node) override {
            auto rv = MauInspector::init_apply(node);
            fields.clear();
            sym_fields.clear();
            state = State::OUTSIDE;
            return rv;
        }

        bool preorder(const IR::MAU::HashGenExpression *) override;
        bool preorder(const IR::MAU::FieldListExpression *) override;
        bool preorder(const IR::Constant *) override;
        bool preorder(const IR::Expression *) override;
        bool preorder(const IR::MAU::ActionArg *) override;
        bool preorder(const IR::Mask *) override;
        bool preorder(const IR::Cast *) override;
        bool preorder(const IR::Concat*) override;
        bool preorder(const IR::StructExpression*) override;
        bool preorder(const IR::ListExpression*) override;
        void postorder(const IR::BFN::SignExtend *) override;
        void postorder(const IR::MAU::HashGenExpression *) override;

     public:
        explicit InsideHashGenExpr(BuildP4HashFunction &s) : self(s) {}
    };


    class OutsideHashGenExpr : public MauInspector {
        BuildP4HashFunction &self;
        bool preorder(const IR::MAU::HashGenExpression *) override;
        void postorder(const IR::Slice *) override;

     public:
        explicit OutsideHashGenExpr(BuildP4HashFunction &s) : self(s) {}
    };

    void end_apply() override;

 public:
    explicit BuildP4HashFunction(const PhvInfo &p) : phv(p) {
        addPasses({
            new InsideHashGenExpr(*this),
            new OutsideHashGenExpr(*this)
        });
    }

    P4HashFunction *func() { return _func; }
};

class AdjustIXBarExpression : public MauModifier {
    bool preorder(IR::MAU::IXBarExpression *e) override;
};

#endif /* BF_P4C_MAU_IXBAR_EXPR_H_ */
