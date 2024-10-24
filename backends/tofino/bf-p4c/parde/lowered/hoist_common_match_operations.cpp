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

#include "hoist_common_match_operations.h"

#include "bf-p4c/ir/ir_enums.h"
#include "bf-p4c/parde/common/allocators.h"
#include "bf-p4c/parde/common/match_reducer.h"

namespace Parde::Lowered {

Visitor::profile_t HoistCommonMatchOperations::init_apply(const IR::Node *root) {
    auto rv = Transform::init_apply(root);
    original_to_modified.clear();
    return rv;
}

bool HoistCommonMatchOperations::compare_match_operations(const IR::BFN::LoweredParserMatch *a,
                                                          const IR::BFN::LoweredParserMatch *b) {
    return a->shift == b->shift && a->hdrLenIncFinalAmt == b->hdrLenIncFinalAmt &&
           a->extracts == b->extracts && a->saves == b->saves && a->scratches == b->scratches &&
           a->checksums == b->checksums && a->counters == b->counters &&
           a->priority == b->priority && a->partialHdrErrProc == b->partialHdrErrProc &&
           a->bufferRequired == b->bufferRequired && a->offsetInc == b->offsetInc &&
           a->loop == b->loop;
}

const IR::BFN::LoweredParserMatch *HoistCommonMatchOperations::get_unconditional_match(
    const IR::BFN::LoweredParserState *state) {
    if (state->transitions.size() == 0) return nullptr;

    if (state->select->regs.empty() && state->select->counters.empty())
        return state->transitions[0];

    // Detect if all transitions actually do the same thing and branch/loop to
    // the same state.  That represents another type of unconditional match.
    std::vector<match_t> matches;
    for (const auto &transition : state->transitions) {
        if (auto match = transition->value->to<IR::BFN::ParserConstMatchValue>()) {
            matches.push_back(match->value);
        } else {
            return nullptr;
        }
    }
    HasFullMatchCoverage hfmc(matches);
    if (!hfmc.rv) return nullptr;

    auto first_transition = state->transitions[0];
    for (auto &transition : state->transitions) {
        if (!compare_match_operations(first_transition, transition)) return nullptr;
    }

    return state->transitions[0];
}

struct RightShiftPacketRVal : public Modifier {
    int byteDelta = 0;
    bool oob = false;
    explicit RightShiftPacketRVal(int byteDelta) : byteDelta(byteDelta) {}
    bool preorder(IR::BFN::LoweredPacketRVal *rval) override {
        rval->range = rval->range.shiftedByBytes(byteDelta);
        BUG_CHECK(rval->range.lo >= 0, "Shifting extract to negative position.");
        if (rval->range.hi >= Device::pardeSpec().byteInputBufferSize()) oob = true;
        return true;
    }
};

// Checksum also needs the masks to be shifted
struct RightShiftCsumMask : public Modifier {
    int byteDelta = 0;
    bool oob = false;
    bool swapMalform = false;

    explicit RightShiftCsumMask(int byteDelta) : byteDelta(byteDelta) {}

    bool preorder(IR::BFN::LoweredParserChecksum *csum) override {
        if (byteDelta == 0 || oob || swapMalform) return false;
        std::set<nw_byterange> new_masked_ranges;
        for (const auto &m : csum->masked_ranges) {
            nw_byterange new_m(m.lo, m.hi);
            new_m = m.shiftedByBytes(byteDelta);
            BUG_CHECK(new_m.lo >= 0, "Shifting csum mask to negative position.");
            if (new_m.hi >= Device::pardeSpec().byteInputBufferSize()) {
                oob = true;
                return false;
            }
            new_masked_ranges.insert(new_m);
        }
        csum->masked_ranges = new_masked_ranges;
        // We can only shift fullzero or fullones swap by odd number of bytes
        // Let's assume we have the following csum computation:
        // mask: [ 0, 1, 2..3, ...]
        // swap: 0b0...01
        // This essentially means that the actual computation should work with
        // the following order of the bytes:
        // 1 0 2 3
        // Now we shift it by 1 byte:
        // mask: [ 1, 2, 3..4, ...]
        // This gives us the following order of bytes that we need:
        // 2 1 3 4
        // This means that byte 2 needs to be taken as the top byte, but
        // also the byte 3 needs to be taken as the top byte, which is impossible.
        // swap: 0b1...101 uses the wrong position of byte 3
        // swap: 0b1...111 uses the wrong position of byte 2
        if ((byteDelta % 2) && csum->swap != 0 && csum->swap != ~(unsigned)0) {
            swapMalform = true;
            return false;
        }
        // Update swap vector (it needs to be shifted by half of byteDelta)
        csum->swap = csum->swap << byteDelta / 2;
        // Additionally if we shifted by an odd ammount we need to negate the swap
        if (byteDelta % 2) csum->swap = ~csum->swap;
        if (csum->end) {
            BUG_CHECK(-byteDelta <= (int)csum->end_pos, "Shifting csum end to negative position.");
            csum->end_pos += byteDelta;
            if (csum->end_pos >= (unsigned)Device::pardeSpec().byteInputBufferSize()) {
                oob = true;
                return false;
            }
        }
        return true;
    }
};

std::set<PHV::Container> HoistCommonMatchOperations::get_extract_constant_dests(
    const IR::BFN::LoweredParserMatch *match) {
    std::set<PHV::Container> dests;
    for (const auto *extract : match->extracts) {
        const auto *extract_phv = extract->to<IR::BFN::LoweredExtractPhv>();
        if (!extract_phv) continue;
        if (!extract_phv->source->is<IR::BFN::LoweredConstantRVal>()) continue;

        dests.insert(extract_phv->dest->container);
    }
    return dests;
}

std::set<PHV::Container> HoistCommonMatchOperations::get_extract_inbuf_dests(
    const IR::BFN::LoweredParserMatch *match) {
    std::set<PHV::Container> dests;
    for (const auto *extract : match->extracts) {
        const auto *extract_phv = extract->to<IR::BFN::LoweredExtractPhv>();
        if (!extract_phv) continue;
        if (!extract_phv->source->is<IR::BFN::LoweredInputBufferRVal>()) continue;

        dests.insert(extract_phv->dest->container);
    }
    return dests;
}

std::map<unsigned, unsigned> HoistCommonMatchOperations::extractors_used(
    const IR::BFN::LoweredParserMatch *match) {
    std::map<unsigned, unsigned> extractors_used = {{8, 0}, {16, 0}, {32, 0}};
    for (const auto *extract : match->extracts) {
        if (const auto *extract_phv = extract->to<IR::BFN::LoweredExtractPhv>()) {
            const auto *dest = extract_phv->dest;
            extractors_used[dest->container.size()]++;
        }
    }
    for (const auto *checksum : match->checksums) {
        if (auto *phv_dest = checksum->phv_dest) {
            extractors_used[phv_dest->container.size()]++;
        }
        if (auto *csum_err = checksum->csum_err) {
            if (!csum_err->container) continue;
            extractors_used[csum_err->container->container.size()]++;
        }
    }

    if (Device::currentDevice() == Device::JBAY) {
        unsigned &b8s_used = extractors_used[8];
        unsigned &b32s_used = extractors_used[32];

        extractors_used[16] += (b8s_used + (2 - 1)) / 2 + b32s_used * 2;
        extractors_used.erase(8);
        extractors_used.erase(32);
    }
    return extractors_used;
}

bool HoistCommonMatchOperations::can_hoist(const IR::BFN::LoweredParserMatch *a,
                                           const IR::BFN::LoweredParserMatch *b) {
    if (a->hdrLenIncFinalAmt || b->hdrLenIncFinalAmt) return false;

    if (computed.dontMergeStates.count(a->next) || computed.dontMergeStates.count(b->next))
        return false;

    if (a->priority && b->priority) return false;

    if (a->offsetInc && b->offsetInc) return false;

    // Checking partialHdrErrProc in both parser match might be a bit
    // restrictive given that two states that both have extracts
    // and/or saves will not be merged (see check above), but it
    // prevents merging two matches with incompatible partial header
    // settings.
    if (a->partialHdrErrProc != b->partialHdrErrProc) return false;

    if (int(a->shift + b->shift) > Device::pardeSpec().byteInputBufferSize()) return false;

    if (b->loop) return false;

    RightShiftPacketRVal shifter(a->shift);
    RightShiftCsumMask csum_shifter(a->shift);

    IR::BFN::LoweredParserMatch *a_clone = a->clone();

    for (auto extract : b->extracts) {
        extract->apply(shifter);
        a_clone->extracts.push_back(extract->to<IR::BFN::LoweredParserPrimitive>());
    }

    // Merge ExtractConstants that have the same dest container
    ordered_map<PHV::Container, ordered_set<const IR::BFN::LoweredParserPrimitive *>>
        container_to_parser_primitive;
    ordered_map<PHV::Container, uint32_t> container_to_constant;
    for (auto extract : a_clone->extracts) {
        const auto *extract_phv = extract->to<IR::BFN::LoweredExtractPhv>();
        if (!extract_phv) continue;

        const auto *constant_rval = extract_phv->source->to<IR::BFN::LoweredConstantRVal>();
        if (!constant_rval) continue;

        PHV::Container container = extract_phv->dest->container;
        container_to_parser_primitive[container].push_back(extract);
        container_to_constant[container] |= constant_rval->constant;
    }

    for (const auto &[container, parser_primitives] : container_to_parser_primitive) {
        if (parser_primitives.size() < 2) continue;
        if (std::any_of(parser_primitives.cbegin(), parser_primitives.cend(),
                        [](const IR::BFN::LoweredParserPrimitive *parser_primitive) {
                            const auto *extract_phv =
                                parser_primitive->to<IR::BFN::LoweredExtractPhv>();
                            return extract_phv->write_mode != IR::BFN::ParserWriteMode::BITWISE_OR;
                        }))
            return false;

        bool is_merged = false;
        for (const auto *parser_primitive : parser_primitives) {
            auto it =
                std::find(a_clone->extracts.begin(), a_clone->extracts.end(), parser_primitive);
            it = a_clone->extracts.erase(it);
            if (!is_merged)
                it = a_clone->extracts.insert(
                    it, new IR::BFN::LoweredExtractPhv(
                            container,
                            new IR::BFN::LoweredConstantRVal(container_to_constant[container])));
            is_merged = true;
        }
    }

    // Cannot hoist if both matches save to the same match registers
    std::set<MatchRegister> save_dests;
    for (auto save : a->saves) save_dests.insert(save->dest);
    for (auto save : b->saves) {
        if (!save_dests.insert(save->dest).second) return false;
        save->apply(shifter);
        a_clone->saves.push_back(save->to<IR::BFN::LoweredSave>());
    }

    a_clone->scratches.insert(b->scratches.begin(), b->scratches.end());

    std::vector<const IR::BFN::LoweredParserChecksum *> checksums;
    for (auto checksum : a->checksums) {
        checksums.push_back(checksum);
    }
    for (auto checksum : b->checksums) {
        if (auto it = std::find_if(
                checksums.begin(), checksums.end(),
                [&checksum](auto a_checksum) { return a_checksum->unit_id == checksum->unit_id; });
            it != checksums.end())
            return false;
        checksum->apply(shifter);
        checksum->apply(csum_shifter);
        a_clone->checksums.push_back(checksum->to<IR::BFN::LoweredParserChecksum>());
    }

    for (auto counter : b->counters) {
        counter->apply(shifter);
        a_clone->counters.push_back(counter->to<IR::BFN::ParserCounterPrimitive>());
    }
    if (a_clone->counters.size() > 1) return false;
    for (const auto *counter : a_clone->counters) {
        if (const auto *load_pkt = counter->to<IR::BFN::ParserCounterLoadPkt>()) {
            for (const auto &pair : load_pkt->source->reg_slices) {
                if (std::any_of(save_dests.begin(), save_dests.end(),
                                [&pair](auto save_dest) { return save_dest == pair.first; }))
                    return false;
            }
        }
    }

    LoweredParserMatchAllocator alloc(a_clone, clot_info);
    delete a_clone;
    if (!alloc.spilled_statements.empty()) return false;

    if (shifter.oob || csum_shifter.oob || csum_shifter.swapMalform) return false;

    std::set<PHV::Container> constant_dests = get_extract_constant_dests(a);
    std::set<PHV::Container> b_constant_dests = get_extract_constant_dests(b);
    constant_dests.insert(b_constant_dests.begin(), b_constant_dests.end());
    if (constant_dests.size() > 2) return false;

    std::set<PHV::Container> a_inbuf_dests = get_extract_inbuf_dests(a);
    std::set<PHV::Container> b_inbuf_dests = get_extract_inbuf_dests(b);

    // Cannot multiwrite both a constant and an inbuf value to the same PHV
    std::set<PHV::Container> all_dests;
    all_dests.merge(constant_dests);
    all_dests.merge(a_inbuf_dests);
    all_dests.merge(b_inbuf_dests);
    if (!a_inbuf_dests.empty() || !b_inbuf_dests.empty()) return false;

    auto extactors_required = extractors_used(a);
    auto b_extractors = extractors_used(b);
    extactors_required = std::accumulate(b_extractors.begin(), b_extractors.end(),
                                         extactors_required, [](auto &map, const auto &pair) {
                                             map[pair.first] += pair.second;
                                             return map;
                                         });
    for (const auto &[size, max] : Device::pardeSpec().extractorSpec())
        if (extactors_required[size] > max) return false;

    // Do not hoist a into b if result would contain more than 2 CLOTs or if b contains a spilled
    // CLOT extract from a
    if (Device::numClots() > 0 && BackendOptions().use_clot) {
        assoc::hash_set<const Clot *> clots;
        for (auto e : a->extracts) {
            if (auto extract_clot = e->to<IR::BFN::LoweredExtractClot>()) {
                clots.insert(extract_clot->dest);
            }
        }
        for (auto e : b->extracts) {
            if (auto extract_clot = e->to<IR::BFN::LoweredExtractClot>()) {
                bool spilled_clot = !clots.insert(extract_clot->dest).second;
                if (spilled_clot) return false;
            }
        }
        if (clots.size() > Device::pardeSpec().maxClotsPerState()) return false;
    }

    return true;
}

void HoistCommonMatchOperations::do_hoist(IR::BFN::LoweredParserMatch *match,
                                          const IR::BFN::LoweredParserMatch *next) {
    RightShiftPacketRVal shifter(match->shift);
    RightShiftCsumMask csum_shifter(match->shift);

    for (auto e : next->extracts) {
        auto s = e->apply(shifter);
        match->extracts.push_back(s->to<IR::BFN::LoweredParserPrimitive>());
    }

    // Merge ExtractConstants that have the same dest container
    ordered_map<PHV::Container, ordered_set<const IR::BFN::LoweredParserPrimitive *>>
        container_to_parser_primitive;
    ordered_map<PHV::Container, uint32_t> container_to_constant;
    for (auto e : match->extracts) {
        if (const auto *extract_phv = e->to<IR::BFN::LoweredExtractPhv>()) {
            if (const auto *constant_rval =
                    extract_phv->source->to<IR::BFN::LoweredConstantRVal>()) {
                PHV::Container container = extract_phv->dest->container;
                container_to_parser_primitive[container].push_back(e);
                container_to_constant[container] |= constant_rval->constant;
            }
        }
    }

    for (const auto &[container, parser_primitives] : container_to_parser_primitive) {
        if (parser_primitives.size() < 2) continue;

        bool is_merged = false;
        for (const auto *parser_primitive : parser_primitives) {
            auto it = std::find(match->extracts.begin(), match->extracts.end(), parser_primitive);
            it = match->extracts.erase(it);
            if (!is_merged)
                it = match->extracts.insert(
                    it, new IR::BFN::LoweredExtractPhv(
                            container,
                            new IR::BFN::LoweredConstantRVal(container_to_constant[container])));
            is_merged = true;
        }
    }

    for (auto e : next->saves) {
        auto s = e->apply(shifter);
        match->saves.push_back(s->to<IR::BFN::LoweredSave>());
    }

    for (auto e : next->checksums) {
        auto s = e->apply(shifter);
        auto cs = s->apply(csum_shifter);
        match->checksums.push_back(cs->to<IR::BFN::LoweredParserChecksum>());
    }

    for (auto e : next->counters) {
        auto s = e->apply(shifter);
        match->counters.push_back(s->to<IR::BFN::ParserCounterPrimitive>());
    }

    match->loop = next->loop;

    match->shift += next->shift;

    if (!match->priority) match->priority = next->priority;

    match->partialHdrErrProc = next->partialHdrErrProc;

    if (!match->offsetInc) match->offsetInc = next->offsetInc;
}

// do not merge loopback state for now, need to maintain loopback pointer TODO
bool HoistCommonMatchOperations::is_loopback_state(cstring state) {
    auto parser = findOrigCtxt<IR::BFN::LoweredParser>();
    if (parser_info.graph(parser).is_loopback_state(state)) return true;

    return false;
}

struct ClearLoweredParserMatch : public Modifier {
    const IR::BFN::LoweredParserMatch *match_to_clear = nullptr;
    explicit ClearLoweredParserMatch(const IR::BFN::LoweredParserMatch *match)
        : match_to_clear(match) {}
    bool preorder(IR::BFN::LoweredParserMatch *match) override {
        auto original = getOriginal<IR::BFN::LoweredParserMatch>();
        if (original != match_to_clear) return true;

        match->extracts.clear();
        match->saves.clear();
        match->counters.clear();
        match->checksums.clear();
        match->shift = 0;
        return false;
    }
};

IR::Node *HoistCommonMatchOperations::preorder(IR::BFN::LoweredParserMatch *match) {
    const auto *original_match = getOriginal<IR::BFN::LoweredParserMatch>();
    const auto *parser = findOrigCtxt<IR::BFN::LoweredParser>();
    const auto *state = findOrigCtxt<IR::BFN::LoweredParserState>();

    if (computed.dontMergeStates.count(state)) return match;
    if (!match->next) return match;

    std::string next_name(match->next->name.c_str());
    if (is_loopback_state(match->next->name) || ((next_name.find("$") != std::string::npos) &&
                                                 (next_name.find("stall") != std::string::npos) &&
                                                 (next_name.find("$") < next_name.rfind("stall"))))
        return match;

    const IR::BFN::LoweredParserState *original_next_state = original_match->next;
    const IR::BFN::LoweredParserState *next_state = match->next;
    const IR::BFN::LoweredParserMatch *child_match = get_unconditional_match(next_state);
    if (!child_match) return match;

    auto parent_matches = parser_info.graph(parser).transitions_to(original_next_state);
    parent_matches.insert(match);
    for (const auto *parent_match : parent_matches) {
        if (!can_hoist(parent_match, child_match)) return match;
    }

    if (Device::numClots() > 0 && BackendOptions().use_clot) {
        clot_info.merge_parser_states(state->thread(), state->name, match->next->name);
    }

    do_hoist(match, child_match);

    if (original_to_modified.count(next_state)) {
        match->next = original_to_modified.at(next_state);
    } else {
        const IR::BFN::LoweredParserState *modified_next_state = next_state;
        for (const auto *child_match : next_state->transitions) {
            ClearLoweredParserMatch clearer(child_match);
            const IR::Node *modified = modified_next_state->apply(clearer);
            modified_next_state = modified->to<IR::BFN::LoweredParserState>();
        }
        match->next = modified_next_state;
        original_to_modified[next_state] = modified_next_state;
    }

    return match;
}

}  // namespace Parde::Lowered
