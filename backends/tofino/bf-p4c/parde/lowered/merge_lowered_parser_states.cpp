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

#include "merge_lowered_parser_states.h"

namespace Parde::Lowered {

bool MergeLoweredParserStates::compare_match_operations(const IR::BFN::LoweredParserMatch *a,
                                                        const IR::BFN::LoweredParserMatch *b) {
    return a->shift == b->shift && a->hdrLenIncFinalAmt == b->hdrLenIncFinalAmt &&
           a->extracts == b->extracts && a->saves == b->saves && a->scratches == b->scratches &&
           a->checksums == b->checksums && a->counters == b->counters &&
           a->priority == b->priority && a->partialHdrErrProc == b->partialHdrErrProc &&
           a->bufferRequired == b->bufferRequired && a->next == b->next &&
           a->offsetInc == b->offsetInc && a->loop == b->loop;
}

const IR::BFN::LoweredParserMatch *MergeLoweredParserStates::get_unconditional_match(
    const IR::BFN::LoweredParserState *state) {
    if (state->transitions.size() == 0) return nullptr;

    if (state->select->regs.empty() && state->select->counters.empty())
        return state->transitions[0];

    // Detect if all transitions actually do the same thing and branch/loop to
    // the same state.  That represents another type of unconditional match.
    auto first_transition = state->transitions[0];
    for (auto &transition : state->transitions) {
        if (!compare_match_operations(first_transition, transition)) return nullptr;
    }

    return state->transitions[0];
}

bool MergeLoweredParserStates::RightShiftPacketRVal::preorder(IR::BFN::LoweredPacketRVal *rval) {
    rval->range = rval->range.shiftedByBytes(byteDelta);
    BUG_CHECK(rval->range.lo >= 0, "Shifting extract to negative position.");
    if (rval->range.hi >= Device::pardeSpec().byteInputBufferSize()) oob = true;
    return true;
}

bool MergeLoweredParserStates::RightShiftCsumMask::preorder(IR::BFN::LoweredParserChecksum *csum) {
    if (byteDelta == 0 || oob || swapMalform) return false;
    std::set<nw_byterange> new_masked_ranges;
    for (auto m : csum->masked_ranges) {
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
    // Now we shift is by 1 byte:
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

bool MergeLoweredParserStates::can_merge(const IR::BFN::LoweredParserMatch *a,
                                         const IR::BFN::LoweredParserMatch *b) {
    if (a->hdrLenIncFinalAmt || b->hdrLenIncFinalAmt) return false;

    if (computed.dontMergeStates.count(a->next) || computed.dontMergeStates.count(b->next))
        return false;

    if (a->priority && b->priority) return false;

    if (a->offsetInc && b->offsetInc) return false;

    if ((!a->extracts.empty() || !a->saves.empty() || !a->checksums.empty() ||
         !a->counters.empty()) &&
        (!b->extracts.empty() || !b->saves.empty() || !b->checksums.empty() ||
         !b->counters.empty()))
        return false;

    // Checking partialHdrErrProc in both parser match might be a bit
    // restrictive given that two states that both have extracts
    // and/or saves will not be merged (see check above), but it
    // prevents merging two match with incompatible partial header
    // settings.
    if (a->partialHdrErrProc != b->partialHdrErrProc) return false;

    if (int(a->shift + b->shift) > Device::pardeSpec().byteInputBufferSize()) return false;

    RightShiftPacketRVal shifter(a->shift);
    RightShiftCsumMask csum_shifter(a->shift);

    for (auto e : b->extracts) e->apply(shifter);

    for (auto e : b->saves) e->apply(shifter);

    for (auto e : b->checksums) {
        e->apply(shifter);
        e->apply(csum_shifter);
    }

    for (auto e : b->counters) e->apply(shifter);

    if (shifter.oob || csum_shifter.oob || csum_shifter.swapMalform) return false;

    return true;
}

void MergeLoweredParserStates::do_merge(IR::BFN::LoweredParserMatch *match,
                                        const IR::BFN::LoweredParserMatch *next) {
    RightShiftPacketRVal shifter(match->shift);
    RightShiftCsumMask csum_shifter(match->shift);

    for (auto e : next->extracts) {
        auto s = e->apply(shifter);
        match->extracts.push_back(s->to<IR::BFN::LoweredParserPrimitive>());
    }

    for (auto e : next->saves) {
        auto s = e->apply(shifter);
        match->saves.push_back(s->to<IR::BFN::LoweredSave>());
    }

    match->scratches.insert(next->scratches.begin(), next->scratches.end());

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
    match->next = next->next;
    match->shift += next->shift;

    if (!match->priority) match->priority = next->priority;

    match->partialHdrErrProc = next->partialHdrErrProc;

    if (!match->offsetInc) match->offsetInc = next->offsetInc;
}

bool MergeLoweredParserStates::is_loopback_state(cstring state) {
    auto parser = findOrigCtxt<IR::BFN::LoweredParser>();
    if (parser_info.graph(parser).is_loopback_state(state)) return true;

    return false;
}

IR::Node *MergeLoweredParserStates::preorder(IR::BFN::LoweredParserMatch *match) {
    auto state = findOrigCtxt<IR::BFN::LoweredParserState>();
    if (computed.dontMergeStates.count(state)) {
        return match;
    }

    while (match->next) {
        // Attempt merging states if the next state is not loopback
        // and not a compiler-generated counter stall state.
        std::string next_name(match->next->name.c_str());
        if (!is_loopback_state(match->next->name) &&
            !((next_name.find("$") != std::string::npos) &&
              (next_name.find("ctr_stall") != std::string::npos) &&
              (next_name.find("$") < next_name.rfind("ctr_stall")))) {
            if (auto next = get_unconditional_match(match->next)) {
                if (can_merge(match, next)) {
                    LOG3("merge " << match->next->name << " with " << state->name << " ("
                                  << match->value << ")");

                    // Clot update must run first because do_merge changes match->next
                    if (Device::numClots() > 0 && BackendOptions().use_clot)
                        clot.merge_parser_states(state->thread(), state->name, match->next->name);
                    if (defuseInfo)
                        defuseInfo->replace_parser_state_name(
                            createThreadName(match->next->thread(), match->next->name),
                            createThreadName(state->thread(), state->name));
                    do_merge(match, next);
                    continue;
                }
            }
        }
        break;
    }
    return match;
}

}  // namespace Parde::Lowered
