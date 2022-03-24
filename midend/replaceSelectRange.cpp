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

namespace P4 {

std::vector<const IR::Mask *>
DoReplaceSelectRange::rangeToMasks(const IR::Range *r) {
    std::vector<const IR::Mask *> masks;

    auto l = r->left->to<IR::Constant>();
    if (!l) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: Range boundaries must be a compile-time constants.",
                r->left);
        return masks;
    }
    auto left = l->value;

    auto ri = r->right->to<IR::Constant>();
    if (!ri) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: Range boundaries must be a compile-time constants.",
                r->right);
        return masks;
    }
    auto right = ri->value;
    if (right < left) {
        ::error(ErrorType::ERR_INVALID, "%1%-%2%: Range end is less than start.",
                r->left, r->right);
        return masks;
    }
    auto constType = r->left->type;
    auto base = l->base;

    int width = r->type->width_bits();
    BUG_CHECK(width > 0, "zero-width range is not allowed %1%", r->type);
    big_int min = left;
    big_int max = right;
    BUG_CHECK(min <= max, "range bounds inverted %1%..%2%", left, right);
    big_int size_mask = ((((big_int) 1) << width) - 1);

    big_int range_size_remaining = max - min + 1;

    while (range_size_remaining > 0) {
        // this generates two kinds of mask entries
        // - 0..2^N - 1 where N is the largest number such that this does not
        //              overshoot max -- to cover all numbers lower then 2^N - 1
        //              with one mask entry.
        // - M..M+2^N - 1 to cover remaining entries with masks that fix a bit
        //                prefix and leave the last N bits arbitrary.
        big_int match_stride = ((big_int) 1) << ((min == 0) ? floor_log2(max + 1) : ffs(min));

        while (match_stride > range_size_remaining)
            match_stride >>= 1;

        big_int mask = ~(match_stride - 1) & size_mask;

        auto valConst = new IR::Constant(constType, min, base, true);
        auto maskConst = new IR::Constant(constType, mask, base, true);
        masks.push_back(new IR::Mask(r->srcInfo, valConst, maskConst));

        range_size_remaining -= match_stride;
        min += match_stride;
    }

    return masks;
}

std::vector<IR::Vector<IR::Expression>>
DoReplaceSelectRange::cartesianAppend(const std::vector<IR::Vector<IR::Expression>>& vecs,
                                      const std::vector<const IR::Mask *>& masks) {
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

std::vector<IR::Vector<IR::Expression>>
DoReplaceSelectRange::cartesianAppend(const std::vector<IR::Vector<IR::Expression>>& vecs,
                                      const IR::Expression *e) {
    std::vector<IR::Vector<IR::Expression>> newVecs;

    for (auto v : vecs) {
        auto copy(v);

        copy.push_back(e);
        newVecs.push_back(copy);
    }

    return newVecs;
}

const IR::Node* DoReplaceSelectRange::postorder(IR::SelectCase* sc) {
    auto newCases = new IR::Vector<IR::SelectCase>();
    auto keySet = sc->keyset;

    if (auto r = keySet->to<IR::Range>()) {
        auto masks = rangeToMasks(r);
        if (masks.empty())
            return sc;

        for (auto mask : masks) {
            auto c = new IR::SelectCase(sc->srcInfo, mask, sc->state);
            newCases->push_back(c);
        }

        return newCases;
    } else if (auto oldList = keySet->to<IR::ListExpression>()) {
        std::vector<IR::Vector<IR::Expression>> newVectors;
        IR::Vector<IR::Expression> first;

        newVectors.push_back(first);

        for (auto key : oldList->components) {
            if (auto r = key->to<IR::Range>()) {
                auto masks = rangeToMasks(r);
                if (masks.empty())
                    return sc;

                newVectors = cartesianAppend(newVectors, masks);
            } else {
                newVectors = cartesianAppend(newVectors, key);
            }
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
                 " targets.", sc, newCases->size());
        }

        return newCases;
    }

    return sc;
}

}   // namespace P4
