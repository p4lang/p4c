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

#include "rewrite_emit_clot.h"

namespace Parde::Lowered {

bool RewriteEmitClot::preorder(IR::BFN::Deparser* deparser) {
    // Replace Emits covered in a CLOT with EmitClot

    IR::Vector<IR::BFN::Emit> newEmits;

    // The next field slices we expect to see being emitted, represented as a vector of stacks.
    // Each stack represents the field slices in a CLOT, which can differ depending on which
    // parser state extracted the CLOT. For every emit, we try to pop an element from the top of
    // all the stacks if the slice being emitted matches the top element. As long as an emit
    // causes at least 1 stack to pop its top element, the contents of the CLOT is valid; else,
    // something went wrong. Once all stacks are empty, we are done emitting that CLOT.
    //
    // For example, given the following CLOT, where each uppercase letter represents a field
    // slice that belongs to 1 or more different headers:
    //
    // CLOT 0: {
    //     state_0: AB
    //     state_1: AXY
    //     state_2: ABCDEFG
    // }
    //
    // There are many different orders in which the deparser could emit each field slice. For
    // example: ABCDEFGXY, AXYBCDEFG, AXBYCDEFG, AXBCDEFG, AXBYCDEFG, ABXYCDEFG, etc. All these
    // combinations would lead to the 3 stacks being emptied.
    std::vector<std::stack<const PHV::FieldSlice*>> expectedNextSlices;
    const Clot* lastClot = nullptr;

    LOG4("[RewriteEmitClot] Rewriting EmitFields and EmitChecksums as EmitClots for "
         << deparser->gress);
    for (auto emit : deparser->emits) {
        LOG6("  Evaluating emit: " << emit);
        auto irPovBit = emit->povBit;

        // For each derived Emit class, assign the member that represents the field being
        // emitted to "source".
        const IR::Expression* source = nullptr;
        if (auto emitField = emit->to<IR::BFN::EmitField>()) {
            source = emitField->source->field;
        } else if (auto emitChecksum = emit->to<IR::BFN::EmitChecksum>()) {
            source = emitChecksum->dest->field;
        } else if (auto emitConstant = emit->to<IR::BFN::EmitConstant>()) {
            newEmits.pushBackOrAppend(emitConstant);

            // Disallow CLOTs from being overwritten by deparser constants
            BUG_CHECK(expectedNextSlices.empty(), "CLOT slices not re-written?");

            continue;
        }

        BUG_CHECK(source, "No emit source for %1%", emit);

        auto field = phv.field(source);
        auto sliceClots = clotInfo.slice_clots(field);

        LOG5("  " << field->name << " is emitted"
                  << (sliceClots->empty() ? " from PHV" : "from CLOT"));
        if (sliceClots->empty()) {
            newEmits.pushBackOrAppend(emit);
            continue;
        }

        // If we are emitting a checksum that overwrites a CLOT, register this fact with
        // ClotInfo.
        if (auto emitCsum = emit->to<IR::BFN::EmitChecksum>()) {
            if (auto fieldClot = clotInfo.whole_field_clot(field)) {
                clotInfo.clot_to_emit_checksum()[fieldClot].push_back(emitCsum);
            } else {
                BUG_CHECK(sliceClots->empty(),
                          "Checksum field %s was partially allocated to CLOTs, but this is not"
                          "allowed",
                          field->name);
            }
        }

        // Points to the next bit in `field` that we haven't yet accounted for. This is -1 if
        // we have accounted for all bits. NB: this is little-endian to correspond to the
        // little-endianness of field slices.
        int nextFieldBit = field->size - 1;

        // Handle any slices left over from the last emitted CLOT.
        if (!expectedNextSlices.empty()) {
            // The current slice being emitted had better line up with the next expected slice.
            const PHV::FieldSlice* nextSlice = nullptr;
            for (auto& stack : expectedNextSlices) {
                if (stack.top()->field() != field) continue;

                BUG_CHECK(!(nextSlice != nullptr && *nextSlice != *stack.top()),
                          "Expected next slice of field %1% in %2% is inconsistent: "
                          "%3% != %4%",
                          field->name, *lastClot, nextSlice->shortString(),
                          stack.top()->shortString());
                nextSlice = stack.top();
                stack.pop();
            }
            BUG_CHECK(nextSlice, "Emitted field %1% does not match any slices expected next in %2%",
                      field->name, *lastClot);
            BUG_CHECK(nextSlice->range().hi == nextFieldBit,
                      "Emitted slice %1% does not line up with expected slice %2% in %3%",
                      PHV::FieldSlice(field, StartLen(0, nextFieldBit + 1)).shortString(),
                      nextSlice->shortString(), *lastClot);

            LOG6("    " << nextSlice->shortString() << " is the next expected slice in "
                        << *lastClot);
            LOG6("    It is covered by existing EmitClot: " << newEmits.back());

            nextFieldBit = nextSlice->range().lo - 1;
            // Erase empty stacks from vector of stacks.
            for (auto it = expectedNextSlices.begin(); it != expectedNextSlices.end();) {
                if (!it->empty())
                    ++it;
                else
                    it = expectedNextSlices.erase(it);
            }
        }

        // If we've covered all bits in the current field, move on to the next emit.
        if (nextFieldBit == -1) continue;

        // We haven't covered all bits in the current field yet. The next bit in the field must
        // be in a new CLOT, if it is covered by a CLOT at all. There had better not be any
        // slices left over from the last emitted CLOT.
        BUG_CHECK(expectedNextSlices.empty(),
                  "Emitted slice %1% does not match any expected next slice in %2%",
                  PHV::FieldSlice(field, StartLen(0, nextFieldBit + 1)).shortString(), *lastClot);

        for (const auto &entry : *sliceClots) {
            auto slice = entry.first;
            auto clot = entry.second;

            BUG_CHECK(nextFieldBit > -1,
                      "While processing emits for %1%, the entire field has been emitted, but "
                      "encountered an extra CLOT (tag %2%)",
                      field->name, clot->tag);

            // Ignore the CLOT if it starts before `nextFieldBit`. If this is working right,
            // we've emitted it already with a previous field.
            if (nextFieldBit < slice->range().hi) continue;

            if (slice->range().hi != nextFieldBit) {
                // The next part of the field comes from PHVs. Produce an emit of the slice
                // containing just that part.
                auto irSlice = new IR::Slice(source, nextFieldBit, slice->range().hi + 1);
                auto sliceEmit = new IR::BFN::EmitField(irSlice, irPovBit->field);
                LOG6("    " << field->name << " is partially stored in PHV");
                LOG4("  Created new EmitField: " << sliceEmit);
                newEmits.pushBackOrAppend(sliceEmit);

                nextFieldBit = slice->range().hi;
            }

            // Make sure the first slice in the CLOT lines up with the slice we're expecting.
            // Though the slices in a multiheader CLOT can differ depending on which parser
            // state it begins in, the first slice should always be the same across all states.
            auto clotSlices = clot->parser_state_to_slices();
            auto firstSlice = *clotSlices.begin()->second.begin();
            BUG_CHECK(firstSlice->field() == field, "First field in %1% is %2%, but expected %3%",
                      *clot, firstSlice->field()->name, field->name);
            BUG_CHECK(firstSlice->range().hi == nextFieldBit,
                      "First slice %1% in %2% does not match expected slice %3%",
                      firstSlice->shortString(), *clot,
                      PHV::FieldSlice(field, StartLen(0, nextFieldBit + 1)).shortString());

            // Produce an emit for the CLOT.
            auto clotEmit = new IR::BFN::EmitClot(irPovBit);
            clotEmit->clot = clot;
            clot->pov_bit = irPovBit;
            newEmits.pushBackOrAppend(clotEmit);
            lastClot = clot;
            LOG6("    " << field->name << " is the first field in " << *clot);
            LOG4("  Created new EmitClot: " << clotEmit);

            nextFieldBit = firstSlice->range().lo - 1;

            if (std::all_of(clotSlices.begin(), clotSlices.end(),
                            [](std::pair<const cstring, std::vector<const PHV::FieldSlice*>> pair) {
                                return pair.second.size() == 1;
                            }))
                continue;

            // There are more slices in the CLOT. We had better be done with the current field.
            BUG_CHECK(nextFieldBit == -1, "%1% is missing slice %2%", *clot,
                      PHV::FieldSlice(field, StartLen(0, nextFieldBit + 1)).shortString());

            for (const auto& kv : clotSlices) {
                // Don't add empty stacks to expectedNextSlices. A stack of size = 1 is
                // considered empty since the element currently being processed will
                // immediately be popped below.
                if (kv.second.size() < 2) continue;

                // Make a std::deque that is a reversed copy of
                // clot->parser_state_to_slices().second.
                std::deque<const PHV::FieldSlice*> deq;
                std::copy(kv.second.rbegin(), kv.second.rend(), std::back_inserter(deq));
                deq.pop_back();
                // The std::deque is used to construct a std::stack where the first field
                // slices in the CLOT are at the top.
                expectedNextSlices.emplace_back(deq);
            }
        }

        if (nextFieldBit != -1) {
            BUG_CHECK(expectedNextSlices.empty(), "%1% is missing slice %2%", *lastClot,
                      PHV::FieldSlice(field, StartLen(0, nextFieldBit + 1)).shortString());

            // The last few bits of the field comes from PHVs. Produce an emit of the slice
            // containing just those bits.
            auto irSlice = new IR::Slice(source, nextFieldBit, 0);
            auto sliceEmit = new IR::BFN::EmitField(irSlice, irPovBit->field);
            newEmits.pushBackOrAppend(sliceEmit);
        }
    }

    if (!expectedNextSlices.empty()) {
        std::stringstream out;
        out << *lastClot << " has extra field slices not covered by the "
            << "deparser before RewriteEmitClot: ";
        for (auto it = expectedNextSlices.rbegin(); it != expectedNextSlices.rend(); ++it) {
            if (it != expectedNextSlices.rbegin()) out << ", ";
            std::stack<const PHV::FieldSlice*>& stack = *it;
            while (!stack.empty()) {
                out << stack.top()->shortString();
                if (stack.size() > 1) out << ", ";
                stack.pop();
            }
        }
        BUG("%s", out.str());
    }
    LOG4("Rewriting complete. Deparser emits for " << deparser->gress << " are now:");
    for (const auto* emit : newEmits) LOG4("  " << emit);

    deparser->emits = newEmits;
    return false;
}

}  // namespace Parde::Lowered
