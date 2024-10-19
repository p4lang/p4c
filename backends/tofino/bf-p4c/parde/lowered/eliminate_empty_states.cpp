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

#include "eliminate_empty_states.h"

#include "bf-p4c/device.h"

namespace Parde::Lowered {

bool EliminateEmptyStates::is_empty(const IR::BFN::ParserState *state) {
    if (!state->selects.empty()) return false;

    if (!state->statements.empty()) return false;

    for (auto t : state->transitions) {
        if (!t->saves.empty()) return false;
    }

    if (state->name.find(".$ctr_stall"))  // compiler generated stall
        return false;

    if (state->name.find(".$oob_stall"))  // compiler generated stall
        return false;

    if (state->name.endsWith("$hdr_len_stop_stall"))  // compiler generated stall
        return false;

    auto parser = findOrigCtxt<IR::BFN::Parser>();
    // do not merge loopback state for now, need to maitain loopback pointer TODO
    if (parser_info.graph(parser).is_loopback_state(state->name)) return false;

    return true;
}

const IR::BFN::Transition *EliminateEmptyStates::get_unconditional_transition(
    const IR::BFN::ParserState *state) {
    if (state->transitions.size() == 1) {
        auto t = state->transitions[0];
        if (auto match = t->value->to<IR::BFN::ParserConstMatchValue>()) {
            if (match->value == match_t()) {
                return t;
            }
        }
    }

    return nullptr;
}

IR::Node *EliminateEmptyStates::preorder(IR::BFN::Transition *transition) {
    if (transition->next && is_empty(transition->next)) {
        if (auto next = get_unconditional_transition(transition->next)) {
            if (int(transition->shift + next->shift) <= Device::pardeSpec().byteInputBufferSize()) {
                LOG3("elim empty state " << transition->next->name);
                transition->next = next->next;
                transition->shift += next->shift;
            }
        }
    }

    return transition;
}

}  // namespace Parde::Lowered
