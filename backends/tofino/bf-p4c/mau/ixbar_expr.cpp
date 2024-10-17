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

#include "common/slice.h"
#include "input_xbar.h"
#include "ixbar_expr.h"
#include "resource.h"

void P4HashFunction::slice(le_bitrange hash_slice) {
    le_bitrange shifted_hash_bits = hash_slice.shiftedByBits(hash_bits.lo);
    BUG_CHECK(hash_bits.contains(shifted_hash_bits), "Slice over hash function is not able "
            "to be resolved");
    hash_bits = shifted_hash_bits;
}

bool P4HashFunction::equiv_dyn(const P4HashFunction *func) const {
    return dyn_hash_name == func->dyn_hash_name;
}

/**
 * Much tighter than the actual constraint, as things like identity functions could check
 * for better overlaps as well as random functions wouldn't matter in terms of order, but this
 * can be expanded much later on.  This will work quickly for dynamic hashing
 */
bool P4HashFunction::equiv_inputs_alg(const P4HashFunction *func) const {
    if (algorithm != func->algorithm)
        return false;
    if (inputs.size() != func->inputs.size())
        return false;
    for (size_t i = 0; i < inputs.size(); i++) {
        if (!inputs[i]->equiv(*func->inputs[i]))
            return false;
    }

    if (symmetrically_hashed_inputs != func->symmetrically_hashed_inputs)
        return false;
    return true;
}

bool P4HashFunction::equiv(const P4HashFunction *func) const {
    if (!equiv_dyn(func))
        return false;
    if (!equiv_inputs_alg(func))
        return false;
    if (hash_bits != func->hash_bits)
        return false;
    return true;
}

bool P4HashFunction::is_next_bit_of_hash(const P4HashFunction *func) const {
    if (size() != 1)
        return false;
    if (hash_bits.lo != func->hash_bits.hi + 1)
        return false;
    return is_dynamic() || func->is_dynamic() ? equiv_dyn(func) : equiv_inputs_alg(func);
}

/**
 * This hash function overlap is based on the equiv function which is much more strict
 * than an actual equiv function.  This may be further improved at a later time, but will
 * also work quickly with anything dynamic
 */
bool P4HashFunction::overlap(const P4HashFunction *func, le_bitrange *my_overlap,
        le_bitrange *hash_overlap) const {
    bool equal_inputs = is_dynamic() || func->is_dynamic() ? equiv_dyn(func)
                                                           : equiv_inputs_alg(func);
    if (!equal_inputs)
        return false;

    auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                        (hash_bits.intersectWith(func->hash_bits));

    if (boost_sl == std::nullopt)
        return false;
    le_bitrange overlap = *boost_sl;

    if (my_overlap) {
        auto ov_boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                               (hash_bits.intersectWith(overlap));
        *my_overlap = *ov_boost_sl;
        *my_overlap = my_overlap->shiftedByBits(-1 * hash_bits.lo);
    }

    if (hash_overlap) {
        auto ov_boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                               (func->hash_bits.intersectWith(overlap));
        *hash_overlap = *ov_boost_sl;
        *hash_overlap = hash_overlap->shiftedByBits(-1 * func->hash_bits.lo);
    }
    return true;
}

void P4HashFunction::dbprint(std::ostream &out) const {
    out << "hash " << name() << "(" << inputs << ", " << algorithm << ")" << hash_bits;
}

bool verifySymmetricHashPairs(
    const PhvInfo &phv,
    safe_vector<const IR::Expression *> &field_list,
    const IR::Annotations *annotations,
    gress_t gress,
    const IR::MAU::HashFunction& hf,
    LTBitMatrix *sym_pairs) {

    bool contains_symmetric = false;
    ordered_set<PHV::FieldSlice> symmetric_fields;

    for (auto annot : annotations->annotations) {
        if (annot->name != "symmetric") continue;

        if (!(annot->expr.size() == 2 && annot->expr.at(0)->is<IR::StringLiteral>()
              && annot->expr.at(1)->is<IR::StringLiteral>())) {
            error("%1%: The symmetric annotation requires two string inputs", annot->srcInfo);
            continue;
        }

        const auto *sl0 = annot->expr.at(0)->to<IR::StringLiteral>();
        const auto *sl1 = annot->expr.at(1)->to<IR::StringLiteral>();

        cstring gress_str = (gress == INGRESS ? "ingress::"_cs : "egress::"_cs);
        auto field0 = phv.field(gress_str + sl0->value);
        auto field1 = phv.field(gress_str + sl1->value);

        if (field0 == nullptr || field1 == nullptr) {
            error("%1%: The key %2% in the symmetric annotation is not recognized as a PHV "
                    "field", annot->srcInfo, ((field0 == nullptr) ? sl0->value : sl1->value));
            continue;
        }

        if (field0 == field1) {
            error("%1%: A field %2% cannot be symmetric to itself", annot->srcInfo, sl0->value);
            continue;
        }

        le_bitrange bits0 = { 0, field0->size - 1 };
        le_bitrange bits1 = { 0, field1->size - 1 };

        PHV::FieldSlice fs0(field0, bits0);
        PHV::FieldSlice fs1(field1, bits1);

        le_bitrange bits_in_list0 = { 0, 0 };
        le_bitrange bits_in_list1 = { 0, 0 };

        auto key_pos0 = field_list.end();
        auto key_pos1 = field_list.end();

        int bits_seen = 0;
        for (auto it = field_list.begin(); it != field_list.end(); it++) {
            auto a_fs = PHV::AbstractField::create(phv, *it);
            if (a_fs->field() == field0 && a_fs->range() == bits0) {
                key_pos0 = it;
                bits_in_list0 = { bits_seen, bits_seen + a_fs->size() - 1 };
            }
            if (a_fs->field() == field1 && a_fs->range() == bits1) {
                key_pos1 = it;
                bits_in_list1 = { bits_seen, bits_seen + a_fs->size() - 1 };
            }
            bits_seen += a_fs->size();
        }

        if (key_pos0 == field_list.end() || key_pos1 == field_list.end()) {
            error("%1%: The key %2% in the symmetric annotation does not appear within the "
                "field list", annot->srcInfo,
                ((key_pos0 == field_list.end()) ? sl0->value : sl1->value));
            continue;
        }

        if (!(symmetric_fields.count(fs0) == 0 && symmetric_fields.count(fs1) == 0)) {
            error("%1%: The key %2% in the symmetric annotation has already been declared "
                    "symmetric with another field.  Please use it only once", annot->srcInfo,
                    (symmetric_fields.count(fs0) == 0) ? sl0->value : sl1->value);
            continue;
        }

        if (bits0.size() != bits1.size()) {
            error("%1%: The two symmetric fields are not the same size", annot->srcInfo);
            continue;
        }
        symmetric_fields.insert(fs0);
        symmetric_fields.insert(fs1);

        contains_symmetric = true;

        int index0 = key_pos0 - field_list.begin();
        int index1 = key_pos1 - field_list.begin();
        if (sym_pairs)
            (*sym_pairs)[std::max(index0, index1)][std::min(index0, index1)] = 1;
    }

    if (contains_symmetric) {
        if (hf.type != IR::MAU::HashFunction::CRC) {
            error("%1%: Currently in p4c, symmetric hash is only supported to work with CRC "
                    "algorithms", annotations->srcInfo);
            return false;
        }
    }

    return contains_symmetric;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::MAU::HashGenExpression *) {
    state = State::INSIDE;
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::MAU::FieldListExpression *fle) {
    sym_fields = fle->symmetric_keys;
    state = State::FIELD_LIST;
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::MAU::ActionArg *aa) {
    error("%s: Action Data Argument %s cannot be used in a hash generation expression.  "
            "Data plane values and constants only", aa->srcInfo, aa->name);
    return false;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::Constant *con) {
    fields.emplace_back(con);
    return false;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::Mask *mask) {
    BUG("%s: Masks not supported by Tofino Backend for hash functions", mask->srcInfo);
    return false;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::Cast *) {
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::Concat *) {
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::ListExpression*) {
    /* -- Conversion from P4_14 can create nested tuples. As we construct the hash
     *    field list in the same order as the nested tuple, we can bravely enter
     *    inside. */
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::StructExpression*) {
    return true;
}

bool BuildP4HashFunction::InsideHashGenExpr::preorder(const IR::Expression *expr) {
    if (state == State::OUTSIDE)
        return true;

    /* -- expression attached to a PHV container is always accepted in the hash expression */
    if (self.phv.field(expr)) {
        fields.emplace_back(expr);
        return false;
    }

    if (state == State::FIELD_LIST) {
        /* -- The field list must contain only items attached to PHV containers
         *    as the hashing engine takes inputs from the PHV crossbar. The midend
         *    NormalizeHashList pass should force replacement of expressions
         *    to temporary variables and this invariant should be true. */
        BUG("%s: Hasher's field list must contain only expressions attached with PHV containers.",
            expr->srcInfo);
        return false;
    }

    /* -- We are not in a field list -> the expression must be an ixbar expression */
    if (state == State::INSIDE) {
        state = State::IXBAR_EXPR;
        BUG_CHECK(
            CanBeIXBarExpr(expr),
            "%s: Hashing expression cannot be implemented at the input crossbar",
            expr->srcInfo);
    }

    return true;
}

void BuildP4HashFunction::InsideHashGenExpr::postorder(const IR::BFN::SignExtend *se) {
    BUG_CHECK(!fields.empty(), "SignExtend on nonexistant field");
    auto expr = se->expr;
    while (auto c = expr->to<IR::Cast>())
        expr = c->expr;

    BUG_CHECK(fields.back() == expr, "SignExtend mismatch");
    int size = expr->type->width_bits();
    for (int i = se->type->width_bits(); i > size; --i) {
        fields.insert(fields.end() - 1, MakeSlice(expr, size - 1, size - 1));
    }
}

void BuildP4HashFunction::InsideHashGenExpr::postorder(const IR::MAU::HashGenExpression *hge) {
    state = State::OUTSIDE;
    BUG_CHECK(!self._func, "Multiple HashGenExpressions in a single BuildP4HashFunction");
    self._func = new P4HashFunction();
    self._func->inputs = fields;
    self._func->hash_bits = { 0, hge->type->width_bits() - 1 };
    self._func->algorithm = hge->algorithm;
    self._func->symmetrically_hashed_inputs = sym_fields;
    self._func->hash_gen_expr = hge->expr;
    if (hge->dynamic)
        self._func->dyn_hash_name = hge->id.name;
}


bool BuildP4HashFunction::OutsideHashGenExpr::preorder(const IR::MAU::HashGenExpression *) {
    return false;
}

void BuildP4HashFunction::OutsideHashGenExpr::postorder(const IR::Slice *sl) {
    if (self._func == nullptr)
        return;

    le_bitrange slice_bits = { static_cast<int>(sl->getL()), static_cast<int>(sl->getH()) };
    slice_bits = slice_bits.shiftedByBits(self._func->hash_bits.lo);

    BUG_CHECK(self._func->hash_bits.contains(slice_bits), "%s: Slice over hash function is "
        "not able to be resolved");
    self._func->hash_bits = slice_bits;
}



/**
 * FIXME: This is a crappy hack for dynamic hash to work.  If a table has repeated values, then
 * the dynamic hash function is not correct.  We should just fail, however, just failing would
 * cause the dyn_hash test to not compile, as these field lists get combined into one large
 * field list in the p4-14 to p4-16 converter.  This is a temporary workaround
 */
void BuildP4HashFunction::end_apply() {
    bool repeats_allowed = !_func->is_dynamic() &&
                            _func->algorithm.type != IR::MAU::HashFunction::CRC;
    if (repeats_allowed)
        return;
    std::map<cstring, bitvec> field_list_check;
    for (auto it = _func->inputs.begin(); it != _func->inputs.end(); it++) {
        le_bitrange bits;
        auto field = phv.field(*it, &bits);
        if (field == nullptr)
            continue;

        bitvec used_bits(bits.lo, bits.size());

        bitvec overlap;
        if (field_list_check.count(field->name) > 0)
            overlap = field_list_check.at(field->name) & used_bits;
        if (!overlap.empty()) {
            if (overlap == used_bits) {
                it = _func->inputs.erase(it);
                it--;
            } else {
                error("Overlapping field %s in hash not supported with the hashing algorithm",
                    field->name);
            }
        }
        field_list_check[field->name] |= used_bits;
    }
}

bool AdjustIXBarExpression::preorder(IR::MAU::IXBarExpression *e) {
    auto *tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for ixbar expr - %1%", e);

    if (!tbl->resources) {
        // no allocation for table -- can happen it TablePlacement failed
        return false; }
    for (auto &ce : tbl->resources->salu_ixbar->hash_computed_expressions()) {
        if (e->expr->equiv(*ce.second)) {
            e->bit = ce.first;
            return false; } }
    if (findContext<IR::MAU::HashDist>())
         return false;
    // can get here if TablePlacmenet failed
    // BUG("Can't find %s in the ixbar allocation for %s", e->expr, tbl);
    return false;
}

