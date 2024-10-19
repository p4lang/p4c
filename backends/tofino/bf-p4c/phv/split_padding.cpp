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

#include "split_padding.h"

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/device.h"
#include "bf-p4c/phv/phv_fields.h"

/**
 * @brief Split padding extracts that span container boundaries
 */
const IR::Node *SplitPadding::preorder(IR::BFN::ParserState *state) {
    IR::Vector<IR::BFN::ParserPrimitive> adjusted_stmts;
    for (const auto *prim : state->statements) {
        if (const auto *extract = prim->to<IR::BFN::Extract>()) {
            const auto *dest = extract->dest->to<IR::BFN::FieldLVal>();
            if (!dest) {
                adjusted_stmts.push_back(extract);
                continue;
            }

            le_bitrange range;
            auto *field = phv.field(extract->dest->field, &range);
            if (!field->is_padding()) {
                adjusted_stmts.push_back(extract);
                continue;
            }

            PHV::FieldUse use(PHV::FieldUse::WRITE);
            auto slices = phv.get_alloc(extract->dest->field, PHV::AllocContext::PARSER, &use);
            std::set<PHV::Container> containers;
            std::for_each(slices.begin(), slices.end(),
                          [&containers](const PHV::AllocSlice &slice) {
                              containers.emplace(slice.container());
                          });

            if (containers.size() > 1) {
                LOG3("Split padding extract:\n\tOriginal: " << extract << "\n\tSplit:");
                for (auto &slice : slices) {
                    int slice_lo = slice.field_slice().lo - range.lo;
                    int slice_hi = slice.field_slice().hi - range.lo;
                    auto *newLVal = new IR::BFN::FieldLVal(
                        dest->getSourceInfo(), MakeSlice(dest->field, slice_lo, slice_hi));
                    const IR::BFN::ParserRVal *newRVal = nullptr;
                    if (const auto *bufRVal = extract->source->to<IR::BFN::InputBufferRVal>()) {
                        auto *newBufRVal = bufRVal->clone();
                        newBufRVal->range =
                            nw_bitrange(newBufRVal->range.lo + range.hi - slice.field_slice().hi,
                                        newBufRVal->range.hi - slice_lo);
                        newRVal = newBufRVal;
                    } else if (const auto *constRVal =
                                   extract->source->to<IR::BFN::ConstantRVal>()) {
                        int size = slice_hi - slice_lo + 1;
                        big_int value = (constRVal->constant->value >> slice_lo) & bigBitMask(size);
                        newRVal = new IR::BFN::ConstantRVal(
                            constRVal->getSourceInfo(),
                            new IR::Constant(IR::Type::Bits::get(size), value));
                    } else {
                        BUG("Unexpected rval %1% in state %2%", extract->source, state->name);
                    }
                    auto *new_extract =
                        new IR::BFN::Extract(extract->getSourceInfo(), newLVal, newRVal);
                    adjusted_stmts.push_back(new_extract);
                    LOG3("\t\t" << new_extract);
                }
            } else {
                adjusted_stmts.push_back(extract);
            }
        } else {
            adjusted_stmts.push_back(prim);
        }
    }
    state->statements = adjusted_stmts;

    return state;
}
