/*
 * Copyright 2020, MNK Labs & Consulting
 * http://mnkcg.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "replaceSelectRange.h"

#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/bitwise.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"

namespace P4 {

// expands subranges that do not cross over zero
static void expandRange(const IR::Range *r, std::vector<const IR::Mask *> *masks,
                        const IR::Type *maskType, big_int min, big_int max) {
    int width = r->type->width_bits();
    BUG_CHECK(width > 0, "zero-width range is not allowed %1%", r->type);
    big_int size_mask = ((((big_int)1) << width) - 1);
    auto base = r->left->to<IR::Constant>()->base;

    BUG_CHECK((min >= 0) == (max >= 0), "Wrong subrange %1%..%2% (going over zero)", min, max);
    if (min < 0) {
        // convert negative range to bit-corresponding positive range
        min = size_mask + min + 1;
        max = size_mask + max + 1;
    }

    BUG_CHECK(min <= max, "range bounds inverted %1%..%2%", min, max);

    big_int range_size_remaining = max - min + 1;

    while (range_size_remaining > 0) {
        // this generates two kinds of mask entries
        // - 0..2^N - 1 where N is the largest number such that this does not
        //              overshoot max -- to cover all numbers lower then 2^N - 1
        //              with one mask entry.
        // - M..M+2^N - 1 to cover remaining entries with masks that fix a bit
        //                prefix and leave the last N bits arbitrary.
        big_int match_stride = ((big_int)1) << ((min == 0) ? floor_log2(max + 1) : ffs(min));

        while (match_stride > range_size_remaining) match_stride >>= 1;

        big_int mask = ~(match_stride - 1) & size_mask;

        auto valConst = new IR::Constant(maskType, min, base, true);
        auto maskConst = new IR::Constant(maskType, mask, base, true);
        masks->push_back(new IR::Mask(r->srcInfo, valConst, maskConst));

        range_size_remaining -= match_stride;
        min += match_stride;
    }
}

std::vector<const IR::Mask *> *DoReplaceSelectRange::rangeToMasks(const IR::Range *r,
                                                                  size_t keyIndex) {
    auto l = r->left->to<IR::Constant>();
    if (!l) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: Range boundaries must be a compile-time constants.", r->left);
        return nullptr;
    }
    auto left = l->value;

    auto ri = r->right->to<IR::Constant>();
    if (!ri) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: Range boundaries must be a compile-time constants.", r->right);
        return nullptr;
    }

    auto masks = new std::vector<const IR::Mask *>();
    auto right = ri->value;
    if (right < left) {
        ::warning(ErrorType::WARN_INVALID,
                  "%1%-%2%: Range with end less than start is "
                  "treated as an empty range",
                  r->left, r->right);
        return masks;
    }

    auto inType = r->left->type->to<IR::Type_Bits>();
    BUG_CHECK(inType != nullptr, "Range type %1% is not fixed-width integer", r->left->type);
    bool isSigned = inType->isSigned;
    auto maskType = isSigned ? new IR::Type_Bits(inType->srcInfo, inType->size, false) : inType;

    if (isSigned) {
        signedIndicesToReplace.emplace(keyIndex);
    }

    if (isSigned && left < 0 && right >= 0) {
        expandRange(r, masks, maskType, left, (big_int)-1);
        expandRange(r, masks, maskType, (big_int)0, right);
    } else {
        expandRange(r, masks, maskType, left, right);
    }

    return masks;
}

std::vector<IR::Vector<IR::Expression>> DoReplaceSelectRange::cartesianAppend(
    const std::vector<IR::Vector<IR::Expression>> &vecs,
    const std::vector<const IR::Mask *> &masks) {
    std::vector<IR::Vector<IR::Expression>> newVecs;

    for (auto v : vecs) {
        for (auto mask : masks) {
            auto copy(v);

            copy.push_back(mask);
            newVecs.push_back(copy);
        }
    }

    return newVecs;
}

std::vector<IR::Vector<IR::Expression>> DoReplaceSelectRange::cartesianAppend(
    const std::vector<IR::Vector<IR::Expression>> &vecs, const IR::Expression *e) {
    std::vector<IR::Vector<IR::Expression>> newVecs;

    for (auto v : vecs) {
        auto copy(v);

        copy.push_back(e);
        newVecs.push_back(copy);
    }

    return newVecs;
}

const IR::Node *DoReplaceSelectRange::postorder(IR::SelectExpression *e) {
    BUG_CHECK(findContext<IR::SelectExpression>() == nullptr, "A select nested in select: %1%", e);
    if (!signedIndicesToReplace.empty()) {
        IR::Vector<IR::Expression> newSelectList;
        size_t idx = 0;
        for (auto *expr : e->select->components) {
            if (signedIndicesToReplace.count(idx)) {
                auto eType = expr->type->to<IR::Type_Bits>();
                BUG_CHECK(eType,
                          "Cannot handle select on types other then fixed-width integeral "
                          "types: %1%",
                          expr->type);
                auto unsignedType = new IR::Type_Bits(eType->srcInfo, eType->size, false);

                newSelectList.push_back(new IR::Cast(expr->srcInfo, unsignedType, expr));
            } else {
                newSelectList.push_back(expr);
            }
            ++idx;
        }
        signedIndicesToReplace.clear();
        return new IR::SelectExpression(e->srcInfo, e->type,
                                        new IR::ListExpression(e->select->srcInfo, newSelectList),
                                        e->selectCases);
    }
    return e;
}

const IR::Node *DoReplaceSelectRange::postorder(IR::SelectCase *sc) {
    BUG_CHECK(findContext<IR::SelectExpression>() != nullptr,
              "A lone select case not inside select: %1%", sc);

    auto newCases = new IR::Vector<IR::SelectCase>();
    auto keySet = sc->keyset;

    if (auto r = keySet->to<IR::Range>()) {
        auto masks = rangeToMasks(r, 0);
        if (!masks) return sc;

        for (auto mask : *masks) {
            auto c = new IR::SelectCase(sc->srcInfo, mask, sc->state);
            newCases->push_back(c);
        }

        return newCases;
    } else if (auto oldList = keySet->to<IR::ListExpression>()) {
        std::vector<IR::Vector<IR::Expression>> newVectors;
        IR::Vector<IR::Expression> first;

        newVectors.push_back(first);

        size_t idx = 0;
        for (auto key : oldList->components) {
            if (auto r = key->to<IR::Range>()) {
                auto masks = rangeToMasks(r, idx);
                if (!masks) return sc;

                newVectors = cartesianAppend(newVectors, *masks);
            } else {
                newVectors = cartesianAppend(newVectors, key);
            }
            ++idx;
        }

        for (auto v : newVectors) {
            auto le = new IR::ListExpression(oldList->srcInfo, v);
            newCases->push_back(new IR::SelectCase(sc->srcInfo, le, sc->state));
        }
        if (newCases->size() > MAX_CASES) {
            warn(ErrorType::ERR_OVERLIMIT,
                 "select key set expression with a range expands into %2% "
                 "ternary key set expressions, which may lead to run-time "
                 "performance or parser configuration space issues in some"
                 " targets.",
                 sc, newCases->size());
        }

        return newCases;
    }

    return sc;
}

}  // namespace P4
