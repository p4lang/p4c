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

#include "split_greedy_parser_states.h"

namespace Parde::Lowered {

IR::BFN::ParserState* SplitGreedyParserStates::split_state(IR::BFN::ParserState* state,
                                                           cstring new_state_name) {
    LOG4("  Greedy split for state " << state->name << " --> " << new_state_name);
    IR::Vector<IR::BFN::ParserPrimitive> first_statements;
    IR::Vector<IR::BFN::ParserPrimitive> last_statements;
    std::optional<bool> last_statements_partial_proc = std::nullopt;
    int last_statements_shift_bits = 0;

    // Create "first" and "last" vectors and retrieve the last_statements_partial_proc
    // which will be used to decide how to split the state.
    //
    split_statements(state->statements, first_statements, last_statements,
                     last_statements_shift_bits, last_statements_partial_proc);

    // Check if transitions refer to states that all have compatible
    // partial_hdr_err_proc values.
    //
    std::optional<bool> transitions_partial_proc = std::nullopt;
    auto transitions_verify = partial_hdr_err_proc_verify(nullptr, nullptr, &state->transitions,
                                                          &transitions_partial_proc);

    IR::Vector<IR::BFN::ParserPrimitive> next_state_statements;
    IR::Vector<IR::BFN::ParserPrimitive> new_state_statements;
    int new_transitions_shift_bits = 0;

    if (!transitions_verify || (last_statements_partial_proc && transitions_partial_proc &&
                                *transitions_partial_proc != *last_statements_partial_proc)) {
        // Here, either:
        //
        //  - the transition partial_hdr_err_proc values are incompatible with each other, or;
        //  - the transition partial_hdr_err_proc values are all compatible with each other,
        //    but incompatible with the last extract in the statements.
        //
        // In this case create a next state with only the select and the transitions.
        // Keep all statements in the new state that will replace the current state.
        //
        next_state_statements = IR::Vector<IR::BFN::ParserPrimitive>();
        new_state_statements = state->statements;
        new_transitions_shift_bits = state->transitions.at(0)->shift << 3;
    } else {
        // Here, all transitions are compatible with each other and they are compatible
        // with the last extract in the statements.
        //
        // Create a next state with the last statements, the select and the transitions.
        // Add the first statements to the new state that will replace the current state.
        //
        next_state_statements = last_statements;
        new_state_statements = first_statements;
        new_transitions_shift_bits =
            (state->transitions.at(0)->shift << 3) - last_statements_shift_bits;
    }

    // Create next_state guaranteed to have partial_hdr_err_proc compatible with
    // each other.
    //
    IR::BFN::ParserState* next_state = new IR::BFN::ParserState(
        new_state_name, state->gress, IR::Vector<IR::BFN::ParserPrimitive>(),
        IR::Vector<IR::BFN::Select>(), IR::Vector<IR::BFN::Transition>());

    // Add statements to next state, while adjusting range values
    // when required.
    for (auto& statement : next_state_statements) {
        if (auto* extract = statement->to<IR::BFN::Extract>()) {
            if (auto* pkt_rval = extract->source->to<IR::BFN::PacketRVal>()) {
                LOG4("  Adjusting extract from ["
                     << pkt_rval->range.lo << ".." << pkt_rval->range.hi << "] to ["
                     << pkt_rval->range.lo - new_transitions_shift_bits << ".."
                     << pkt_rval->range.hi - new_transitions_shift_bits << "]");
                nw_bitrange next_range(pkt_rval->range.lo - new_transitions_shift_bits,
                                       pkt_rval->range.hi - new_transitions_shift_bits);
                auto next_pkt_rval =
                    new IR::BFN::PacketRVal(next_range, pkt_rval->partial_hdr_err_proc);
                auto next_extract = new IR::BFN::Extract(extract->dest, next_pkt_rval);
                next_state->statements.push_back(next_extract);
            } else
                next_state->statements.push_back(statement);
        } else
            next_state->statements.push_back(statement);
    }

    // Add selects to next state and adjust ranges.
    for (auto select : state->selects) {
        if (auto* saved = select->source->to<IR::BFN::SavedRVal>()) {
            if (auto* pkt_rval = saved->source->to<IR::BFN::PacketRVal>()) {
                LOG4("  Adjusting select from ["
                     << pkt_rval->range.lo << ".." << pkt_rval->range.hi << "] to ["
                     << pkt_rval->range.lo - new_transitions_shift_bits << ".."
                     << pkt_rval->range.hi - new_transitions_shift_bits << "]");
                nw_bitrange next_range(pkt_rval->range.lo - new_transitions_shift_bits,
                                       pkt_rval->range.hi - new_transitions_shift_bits);
                auto next_pkt_rval =
                    new IR::BFN::PacketRVal(next_range, pkt_rval->partial_hdr_err_proc);
                auto next_select = new IR::BFN::Select(new IR::BFN::SavedRVal(next_pkt_rval));
                next_state->selects.push_back(next_select);
            } else {
                next_state->selects.push_back(select);
            }
        } else
            next_state->selects.push_back(select);
    }

    // Add transitions to next state and adjust shifts.
    for (auto transition : state->transitions) {
        auto next_transition = new IR::BFN::Transition(
            transition->value, transition->shift - (new_transitions_shift_bits >> 3),
            transition->next);
        next_state->transitions.push_back(next_transition);
    }

    // Create new_state that replaces state that is provided in input and that includes
    // the statements that were not moved to next_state.  The partial_hdr_err_proc
    // might not be all compatible at this point, if that's the case this is going
    // to be fixed on the next iteration.
    //
    IR::BFN::Transition* new_transition =
        new IR::BFN::Transition(match_t(), new_transitions_shift_bits >> 3, next_state);
    IR::BFN::ParserState* new_state = new IR::BFN::ParserState(
        state->name, state->gress, new_state_statements, IR::Vector<IR::BFN::Select>(),
        IR::Vector<IR::BFN::Transition>(new_transition));
    return new_state;
}

void SplitGreedyParserStates::split_statements(
    const IR::Vector<IR::BFN::ParserPrimitive> in, IR::Vector<IR::BFN::ParserPrimitive>& first,
    IR::Vector<IR::BFN::ParserPrimitive>& last, int& last_statements_shift_bit,
    std::optional<bool>& last_statements_partial_proc) {
    std::optional<bool> curr_partial_proc = std::nullopt;
    int curr_statements_shift_bit = 0;

    first.clear();
    last.clear();
    last_statements_shift_bit = 0;
    last_statements_partial_proc = std::nullopt;

    for (auto statement : in) {
        if (auto* extract = statement->to<IR::BFN::Extract>()) {
            // Look for change in partial_hdr_err_proc compared to the previous value.
            if (auto* pkt_rval = extract->source->to<IR::BFN::PacketRVal>()) {
                bool extract_partial_hdr_proc = pkt_rval->partial_hdr_err_proc;
                if (!curr_partial_proc || (extract_partial_hdr_proc != *curr_partial_proc)) {
                    first.append(last);
                    last.clear();
                    curr_statements_shift_bit = pkt_rval->range.lo;
                    curr_partial_proc = extract_partial_hdr_proc;
                }
                last_statements_shift_bit = pkt_rval->range.hi - curr_statements_shift_bit;
            }
        }
        last.push_back(statement);
    }
    last_statements_partial_proc = curr_partial_proc;

    if (last_statements_shift_bit) last_statements_shift_bit += 1;

    if (LOGGING(4)) {
        LOG4("  Greedy statements split");
        LOG4("    First: ");
        for (auto s : first) {
            LOG4("      " << s);
        }
        LOG4("    Last: ");
        for (auto s : last) {
            LOG4("      " << s);
        }
        LOG4("    Last primitive partial_hdr_err_proc is "
             << (last_statements_partial_proc ? std::to_string(*last_statements_partial_proc)
                                              : "(nil)")
             << " last statements shift in bits is " << last_statements_shift_bit);
    }
}

bool SplitGreedyParserStates::partial_hdr_err_proc_verify(
    const IR::Vector<IR::BFN::ParserPrimitive>* statements,
    const IR::Vector<IR::BFN::Select>* selects, const IR::Vector<IR::BFN::Transition>* transitions,
    std::optional<bool>* partial_proc_value = nullptr) {
    std::optional<bool> value = std::nullopt;
    if (partial_proc_value) *partial_proc_value = std::nullopt;

    if (statements) {
        for (auto statement : *statements) {
            if (auto* extract = statement->to<IR::BFN::Extract>())
                if (auto* pkt_rval = extract->source->to<IR::BFN::PacketRVal>()) {
                    auto v = pkt_rval->partial_hdr_err_proc;
                    if (value && *value != v) return false;
                    value = v;
                }
        }
    }

    if (selects) {
        for (auto select : *selects) {
            if (auto* saved = select->source->to<IR::BFN::SavedRVal>())
                if (auto* pkt_rval = saved->source->to<IR::BFN::PacketRVal>()) {
                    auto v = pkt_rval->partial_hdr_err_proc;
                    if (value && *value != v) return false;
                    value = v;
                }
        }
    }

    if (transitions) {
        for (auto transition : *transitions) {
            if (auto next_state = transition->next) {
                std::optional<bool> v = std::nullopt;
                if (partial_hdr_err_proc_verify(nullptr, &next_state->selects, nullptr, &v)) {
                    if (v) {
                        if (value && *value != *v) return false;
                        value = *v;
                    }
                } else
                    return false;
            }
        }
    }

    if (partial_proc_value) *partial_proc_value = value;

    return true;
}

bool SplitGreedyParserStates::state_pkt_too_short_verify(const IR::BFN::ParserState* state,
                                                         bool& select_args_incompatible) {
    select_args_incompatible = false;
    if (state->selects.size()) {
        // Look for incompatibility in select arguments.
        // This needs specific processing.
        //
        if (!partial_hdr_err_proc_verify(nullptr, &state->selects, nullptr)) {
            select_args_incompatible = true;
            LOG4("state_pkt_too_short_verify(" << state->name
                                               << "): incompatible select arguments");
            return false;
        }
    }

    if (!partial_hdr_err_proc_verify(&state->statements, nullptr, &state->transitions)) {
        LOG4("state_pkt_too_short_verify(" << state->name
                                           << "): incompatible statements and transitions");
        return false;
    }

    LOG4("state_pkt_too_short_verify(" << state->name << "): ok");
    return true;
}

IR::BFN::ParserState* SplitGreedyParserStates::postorder(IR::BFN::ParserState* state) {
    LOG4("SplitGreedyParserStates(" << state->name << ")");

    int cnt = 0;
    bool select_args_incompatible = false;

    while (!state_pkt_too_short_verify(state, select_args_incompatible)) {
        if (select_args_incompatible) {
            error(ErrorType::ERR_UNSUPPORTED,
                    "Mixing non-greedy and greedy extracted fields in select "
                    "statement is unsupported.");
        } else {
            cstring new_name = state->name + ".$greedy_"_cs + cstring::to_cstring(cnt++);
            state = split_state(state, new_name);
        }
    }

    return state;
}

}  // namespace Parde::Lowered
