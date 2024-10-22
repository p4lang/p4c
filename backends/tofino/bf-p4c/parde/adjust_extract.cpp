/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/parde/adjust_extract.h"

#include <map>
#include <vector>

#include "bf-p4c/device.h"

void AdjustExtract::postorder(IR::BFN::ParserState *state) {
    if (!hasMarshaled(state)) return;

    BUG_CHECK(phv.alloc_done(), "AdjustExtract must be run after phv allocation.");

    // more_bits[i] = x means that on the input buffer, there are x more bits right before
    // the i-th position, so the the original data on i-th, is now as (i + x)th. It is used
    // to calculate the aggregated bit shifting.
    std::map<int, int> more_bits;
    for (const auto *prim : state->statements) {
        if (const auto *extract = prim->to<IR::BFN::Extract>()) {
            if (extract->marshaled_from) {
                auto &marshaled_from = *extract->marshaled_from;
                gress_t gress = marshaled_from.gress;
                cstring field_name = marshaled_from.field_name;
                size_t pre_padding = marshaled_from.pre_padding;
                LOG3("Found Marshaled, marshaled_from = " << marshaled_from);

                cstring full_name = cstring::to_cstring(gress) + "::"_cs + field_name;
                auto *field = phv.field(full_name);
                BUG_CHECK(field, "Can not find field that is marshaled: %1%", full_name);

                size_t actual_pre_padding, actual_post_padding;
                std::tie(actual_pre_padding, actual_post_padding) = calcPrePadding(field);
                if (actual_pre_padding != pre_padding || actual_post_padding != 0) {
                    BUG_CHECK(actual_pre_padding >= pre_padding,
                              "actual prepending less than minimum, "
                              "actual: %1%, min: %2%, for field: %3%",
                              actual_pre_padding, pre_padding, field);
                    BUG_CHECK(extract->source->is<IR::BFN::PacketRVal>(),
                              "marshaled data not from input buffer?");
                    nw_bitrange extract_range = extract->source->to<IR::BFN::PacketRVal>()->range;
                    more_bits[extract_range.lo] += (actual_pre_padding - pre_padding);
                    more_bits[extract_range.hi + 1] += actual_post_padding;
                    LOG3("[Extra padding] pre: "
                         << actual_pre_padding - pre_padding << ", at " << extract_range.lo
                         << " post: " << actual_post_padding << ", at " << extract_range.hi + 1);
                }
            }
        }
    }

    // calculate the largest bit.
    int largest_bit = 0;
    for (const auto *prim : state->statements) {
        if (const auto *extract = prim->to<IR::BFN::Extract>()) {
            if (auto *buf = extract->source->to<IR::BFN::PacketRVal>()) {
                largest_bit = std::max(largest_bit, buf->range.hi + 1);
            }
        }
    }

    for (const auto *transition : state->transitions) {
        for (const auto *save : transition->saves) {
            if (auto *buf = save->source->to<IR::BFN::PacketRVal>()) {
                largest_bit = std::max(largest_bit, buf->range.hi + 1);
            }
        }
    }

    // aggregated[i] means that, the data on wire that should show up at i, is actually
    // at i + aggregated[i].
    int sum_more_bits = 0;
    std::vector<int> aggregated;
    for (int i = 0; i <= largest_bit; i++) {
        int inc = more_bits.count(i) ? more_bits.at(i) : 0;
        sum_more_bits += inc;
        aggregated.push_back(sum_more_bits);
    }

    // Recreate this state by shifting all extract and saves by sum of more_bits
    // `before or equal` it's position
    IR::Vector<IR::BFN::ParserPrimitive> adjusted_stmts;
    for (const auto *prim : state->statements) {
        if (const auto *extract = prim->to<IR::BFN::Extract>()) {
            auto *old_source = extract->source->to<IR::BFN::PacketRVal>();
            if (!old_source) {
                adjusted_stmts.push_back(extract);
                continue;
            }
            nw_bitrange old_source_range = old_source->range;
            BUG_CHECK(old_source_range.hi <= largest_bit, "largest bit of %1% calculation is wrong",
                      state->name);
            BUG_CHECK(aggregated[old_source_range.lo] == aggregated[old_source_range.hi],
                      "marshaled field with padding in the middle? %1%, %2%, %3%", extract,
                      aggregated[old_source_range.lo], aggregated[old_source_range.hi]);
            size_t n_paddings = aggregated[old_source_range.lo];
            auto *adjusted_extract = extract->clone();
            adjusted_extract->marshaled_from = std::nullopt;
            adjusted_extract->source = new IR::BFN::PacketRVal(
                nw_bitrange(old_source_range.lo + n_paddings, old_source_range.hi + n_paddings),
                old_source->partial_hdr_err_proc);
            adjusted_stmts.push_back(adjusted_extract);
            LOG3("Adjust [OLD]:" << extract << "\n"
                                 << " to [NEW]:" << adjusted_extract);
        } else {
            BUG("%1% use checksum in mirrored/resubmit state, not supported", state->name);
        }
    }
    state->statements = adjusted_stmts;

    // adjust saves and shifts.
    IR::Vector<IR::BFN::Transition> adjusted_transitions;
    for (const auto *transition : state->transitions) {
        auto *adjusted_transition = transition->clone();
        BUG_CHECK(aggregated.back() % 8 == 0, "newly added padding is not byte-sized? %1%",
                  aggregated.back());
        adjusted_transition->shift = transition->shift + (aggregated.back() / 8);

        IR::Vector<IR::BFN::SaveToRegister> adjusted_saves;
        for (const auto *save : transition->saves) {
            auto *adjusted_save = save->clone();
            if (auto *buf = save->source->to<IR::BFN::PacketRVal>()) {
                nw_bitrange old_source_range = buf->range;
                BUG_CHECK(old_source_range.hi <= largest_bit,
                          "largest bit of %1% calculation is wrong", state->name);
                BUG_CHECK(aggregated[old_source_range.lo] == aggregated[old_source_range.hi],
                          "marshaled field with padding in the middle? %1%", save);
                size_t n_paddings = aggregated[old_source_range.lo];
                adjusted_save->source = new IR::BFN::PacketRVal(
                    nw_bitrange(old_source_range.lo + n_paddings, old_source_range.hi + n_paddings),
                    buf->partial_hdr_err_proc);
            }
            adjusted_saves.push_back(save);
        }
        adjusted_transition->saves = adjusted_saves;
        adjusted_transitions.push_back(adjusted_transition);
    }
    state->transitions = adjusted_transitions;
}

std::pair<size_t, size_t> AdjustExtract::calcPrePadding(const PHV::Field *field) {
    std::optional<size_t> post_padding = std::nullopt;
    std::optional<size_t> pre_padding = std::nullopt;
    auto alloc = phv.get_alloc(field);
    for (auto &slice : alloc) {
        if (slice.width() != int(slice.container().size())) {
            // pre/post in nw_order, so it is opposite to container's order.
            size_t pre = slice.container().msb() - slice.container_slice().hi;
            size_t post = slice.container_slice().lo;
            if (pre != 0) {
                BUG_CHECK(!pre_padding, "More than one marshaled field slices with prepadding: %1%",
                          field);
                pre_padding = pre;
            }
            if (post != 0) {
                BUG_CHECK(!post_padding, "More than one marshaled field slices with post: %1%",
                          field);
                post_padding = post;
            }
        }
    }
    return {(pre_padding ? *pre_padding : 0), (post_padding ? *post_padding : 0)};
}

bool AdjustExtract::hasMarshaled(const IR::BFN::ParserState *state) {
    bool has_marshaled_field = false;
    forAllMatching<IR::BFN::Extract>(&state->statements, [&](const IR::BFN::Extract *extract) {
        if (extract->marshaled_from) {
            has_marshaled_field = true;
        }
    });
    return has_marshaled_field;
}
