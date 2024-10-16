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

#include "slice.h"
#include "bf-p4c/common/ir_utils.h"

static IR::Constant* sliceConstant(const IR::Constant* k, int lo, int hi) {
    int size = hi-lo+1;
    big_int value = (k->value >> lo) & bigBitMask(size);
    return new IR::Constant(IR::Type::Bits::get(size), value);
}

std::pair<int, int> getSliceLoHi(const IR::Expression *e) {
    if (auto sl = e->to<IR::Slice>())
        return std::make_pair(sl->getL(), sl->getH());
    else
        return std::make_pair(0, e->type->width_bits() - 1);
}

const IR::Expression *MakeSliceDestination(const IR::Expression *e, int lo, int hi) {
    LOG6("\tExpression: " << e);
    LOG6("\tLo: " << lo << ", Hi: " << hi);
    if (auto s = e->to<IR::Slice>())
        LOG6("\tOriginal Expression: " << s->e0);
    if (e->is<IR::MAU::MultiOperand>())
        return new IR::Slice(e, hi, lo);
    if (auto k = e->to<IR::Constant>()) {
        auto rv = sliceConstant(k, lo, hi);
        LOG6("\tReturning a constant: " << rv);
        return rv;
    }
    std::pair<int, int> slLoHi = getSliceLoHi(e);
    LOG6("\tFor slice, lo: " << slLoHi.first << ", hi: " << slLoHi.second);
    if ((lo - slLoHi.first) >= e->type->width_bits()) {
        LOG6("\tReturning 0's");
        return new IR::Constant(IR::Type::Bits::get(hi-lo+1), 0); }
    if (hi > slLoHi.second) {
        LOG6("\tShrink hi bit to within the slice");
        hi = slLoHi.second; }
    if (lo == slLoHi.first && hi == slLoHi.second) {
        LOG6("\tSame width as the expression itself");
        return e; }
    if (auto sl = e->to<IR::Slice>()) {
        LOG6("Encountered a slice [" << sl->getL() << ", " << sl->getH() << "]");
        LOG6("Original expression: " << sl->e0);
        BUG_CHECK(lo >= int(sl->getL()) && hi <= int(sl->getH()),
                  "MakeSlice slice on slice type mismatch");
        e = sl->e0; }
    return new IR::Slice(e, hi, lo);
}

const IR::Expression *MakeSliceSource(const IR::Expression *read, int lo, int hi, const
        IR::Expression *write) {
    LOG6("\tMakeSliceSource Expression: " << read);
    LOG6("\tLo: " << lo << ", Hi: " << hi);
    LOG6("\tMakeSliceSource Destination: " << write);

    // Extract the lo and hi slice bits for both the source and destination expressions
    std::pair<int, int> readLoHi = getSliceLoHi(read);
    std::pair<int, int> writeLoHi = getSliceLoHi(write);

    if (read->is<IR::MAU::MultiOperand>())
        return new IR::Slice(read, hi, lo);
    if (auto k = read->to<IR::Constant>()) {
        auto sliceLo = lo - writeLoHi.first;
        auto sliceHi = hi - lo + sliceLo;
        LOG6("\tSliceLo: " << sliceLo << ", SliceHi: " << sliceHi);
        auto rv = sliceConstant(k, sliceLo, sliceHi);
        LOG6("\tReturning a constant :" << rv);
        return rv;
    }

    LOG6("\tsrc_lo : " << readLoHi.first << ", src_hi : " << readLoHi.second);
    LOG6("\tdest_lo: " << writeLoHi.first << ", dest_hi: " << writeLoHi.second);
    lo = lo - (writeLoHi.first - readLoHi.first);
    hi = hi - (writeLoHi.first - readLoHi.first);

    if ((lo - readLoHi.first) >= read->type->width_bits()) {
        LOG6("\tReturning 0's");
        return new IR::Constant(IR::Type::Bits::get(hi-lo+1), 0); }
    if (hi > readLoHi.second) {
        LOG6("\thi : " << hi << " readLoHi.second: " << readLoHi.second);
        LOG6("\tShrink hi bit to within the read slice");
        hi = readLoHi.second; }
    if (lo == readLoHi.first && hi == readLoHi.second) {
        LOG6("\tRequested same slice as the read expression");
        return read; }
    if (auto sl = read->to<IR::Slice>()) {
        LOG6("\tEncountered a slice [" << sl->getL() << ", " << sl->getH() << "]");
        LOG6("\tOriginal expression: " << sl->e0);
        BUG_CHECK(lo >= int(sl->getL()) && hi <= int(sl->getH()),
                  "MakeSlice slice on slice type mismatch");
        read = sl->e0; }
    LOG6("\tReturn " << read << " lo: " << lo << " hi: " << hi);
    return new IR::Slice(read, hi, lo);
}

const IR::Expression *MakeSlice(const IR::Expression *e, int lo, int hi) {
    BUG_CHECK(hi >= lo, "Invalid args to MakeSlice(%s, %d, %d", e, lo, hi);
    if (lo == 0 && hi == e->type->width_bits() - 1)
        return e;
    if (e->is<IR::MAU::MultiOperand>())
        return new IR::Slice(e, hi, lo);
    if (auto k = e->to<IR::Constant>()) {
        auto rv = sliceConstant(k, lo, hi);
        return rv;
    }
    if (auto a = e->to<IR::BAnd>())
        return new IR::BAnd(e->srcInfo, MakeSlice(a->left, lo, hi), MakeSlice(a->right, lo, hi));
    if (auto o = e->to<IR::BOr>())
        return new IR::BOr(e->srcInfo, MakeSlice(o->left, lo, hi), MakeSlice(o->right, lo, hi));
    if (auto o = e->to<IR::BXor>())
        return new IR::BXor(e->srcInfo, MakeSlice(o->left, lo, hi), MakeSlice(o->right, lo, hi));
    if (auto cc = e->to<IR::Concat>()) {
        int rsize = cc->right->type->width_bits();
        if (hi < rsize) return MakeSlice(cc->right, lo, hi);
        if (lo >= rsize) return MakeSlice(cc->left, lo - rsize, hi - rsize);
        return new IR::Concat(cc->srcInfo, MakeSlice(cc->left, 0, hi - rsize),
                                           MakeSlice(cc->right, lo, rsize - 1)); }
    if (lo >= e->type->width_bits()) {
        return new IR::Constant(IR::Type::Bits::get(hi-lo+1), 0); }
    if (hi >= e->type->width_bits()) {
        hi = e->type->width_bits() - 1; }
    if (auto sl = e->to<IR::Slice>()) {
        lo += sl->getL();
        hi += sl->getL();
        BUG_CHECK(lo >= int(sl->getL()) && hi <= int(sl->getH()),
                  "MakeSlice slice on slice type mismatch");
        e = sl->e0; }
    if (auto *neg = e->to<IR::Neg>())
        return new IR::Neg(e->srcInfo, MakeSlice(neg->expr, lo, hi));
    return new IR::Slice(e, hi, lo);
}

safe_vector<const IR::Expression *> convertMaskToSlices(const IR::Mask *mask) {
    BUG_CHECK(mask != nullptr, "Cannot convert nullptr IR::Mask");
    safe_vector<const IR::Expression *> slice_vector;

    auto value = mask->right->to<IR::Constant>()->value;
    auto expr = mask->left;
    unsigned zero_pos = 0;
    unsigned one_pos = Util::scan1(value, zero_pos);
    while (one_pos < static_cast<size_t>(expr->type->width_bits())) {
        // clamp at bitwidth to avoid overflow for negative numbers which might
        // not have any more zeroes in higher bits
        zero_pos = std::min(unsigned(expr->type->width_bits()), Util::scan0(value, one_pos));
        auto slice = MakeSlice(mask->left, one_pos, zero_pos - 1);
        slice_vector.push_back(slice);
        one_pos = Util::scan1(value, zero_pos);
    }
    return slice_vector;
}

const IR::Expression *MakeWrappedSlice(const IR::Expression *e, int lo, int hi, int wrap_size) {
    BUG_CHECK(wrap_size > lo && lo > hi, "The creation of this wrap slice is incorrect");
    if (e->is<IR::MAU::MultiOperand>() || e->is<IR::MAU::HashDist>()
        || e->is<IR::MAU::RandomNumber>()) {
        return new IR::MAU::WrappedSlice(e, hi, lo, wrap_size);
    } else {
        BUG("Cannot create wrapped slice on anything that is not a MultiOperand/HashDist/Random");
    }
    return nullptr;
}
