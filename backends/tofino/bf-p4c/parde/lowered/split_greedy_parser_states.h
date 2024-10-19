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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_SPLIT_GREEDY_PARSER_STATES_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_SPLIT_GREEDY_PARSER_STATES_H_

#include "bf-p4c/parde/clot/clot_info.h"
#include "ir/visitor.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief This pass is used to ensure that there will be no conflicting partial_hdr_err_proc
 *        at the time the LoweredParserMatch is created.
 *
 * This pass makes sure that each parser state respects the following constraints:
 *
 *      1. A state includes only extract statements in which PacketRVal have same
 *         partial_hdr_err_proc;
 *
 *      2. When a state includes both extracts and transitions, the partial_hdr_err_proc
 *         from the extracts (which are all the same according to 1.) are the same
 *         as the PacketRVal in select() statements from the next states.
 *
 * Constraint 1 is pretty simple; since there is only a single partial_hdr_err_proc
 * in the LoweredParserMatch node, two extracts must have the same setting in order
 * not to conflict.
 *
 * Constraint 2 exists because load operations issued from select statements in next
 * states are typically moved up to the current state to prepare the data for the
 * match that will occur in the next state.  The field that is referenced in these
 * select statements must have the same partial_hdr_err_proc as the extracts in the
 * current state in order not to conflict.
 *
 * States that do not respect these constraints are split in states that do, starting
 * from the ones that are further down the tree and going upwards.  When a select is
 * moved from the current state to a new state, to reduce the number of states created,
 * extracts that are compatible with that select are moved also to that new state
 * as long as the original extract order is respected.  In other words, if the compatible
 * extracts are the last ones in the current state, before the select.
 *
 */
struct SplitGreedyParserStates : public Transform {
    SplitGreedyParserStates() {}

 private:
    /**
     * @brief Creates a greedy stall state based on partial_hdr_err_proc settings
     *        in the statements, selects and transitions.
     *
     * The state that is created comes after the state provided in input and is
     * garanteed to include partial_hdr_err_proc settings that are compatible
     * with each other.
     *
     * Checking that state has incompatible partial_hdr_err_proc settings
     * has to be done prior to calling this function.
     *
     */
    IR::BFN::ParserState *split_state(IR::BFN::ParserState *state, cstring new_state_name);

    /**
     * @brief Split statements from vector "in" such that the "last" vector
     *        contains only statements that have partial_hdr_err_proc settings
     *        that are the same with each other.  Does not modify the statements
     *        order.
     *
     * On return:
     *
     *      first: the statements from vector "in" which partial_hdr_err_proc are
     *             not compatible with the ones returned in vector "last".  This
     *             vector can include statements with conflicting partial_hdr_err_proc
     *             values.
     *
     *      last: the statements from vector "in" with partial_hdr_err_proc that are
     *            compatible with the last extract in the state.  All statements in
     *            this vector are no-conflicting with each other.
     *
     *      last_statements_shift_bit: total shift in bits of the statements included
     *                                 in vector "last".
     *
     *      last_statements_partial_proc: the setting of partial_hdr_err_proc of the statements
     *                                    included in vector "last".  Set to std::nullopt if
     *                                    no extract with partial_hdr_err_proc were found
     *                                    in vector "in".
     *
     */
    void split_statements(const IR::Vector<IR::BFN::ParserPrimitive> in,
                          IR::Vector<IR::BFN::ParserPrimitive> &first,
                          IR::Vector<IR::BFN::ParserPrimitive> &last,
                          int &last_statements_shift_bit,
                          std::optional<bool> &last_statements_partial_proc);

    /**
     * @brief Checks that the partial_hdr_err_proc setting is the
     *        same for all vector content provided in input.  Skip
     *        check for vectors which pointer is set to nullptr.
     *
     * On return:
     *      false:   partial_hdr_err_proc are conflicting.
     *      true:    partial_hdr_err_proc are non-conflicting.
     *
     *      partial_proc_value:  when returned value is true (i.e. all partial_hdr_err_proc are
     *                           non-conflicting), contains the value of those
     *                           partial_hdr_err_proc.  Returns std::nullopt if none of the
     *                           statements contained any partial_hdr_err_proc field.
     *
     */
    bool partial_hdr_err_proc_verify(const IR::Vector<IR::BFN::ParserPrimitive> *statements,
                                     const IR::Vector<IR::BFN::Select> *selects,
                                     const IR::Vector<IR::BFN::Transition> *transitions,
                                     std::optional<bool> *partial_proc_value);

    /**
     * @brief Checks if the state's statement, selects and transitions
     *        include only non-conflicting partial_hdr_err_proc values.
     *
     */
    bool state_pkt_too_short_verify(const IR::BFN::ParserState *state,
                                    bool &select_args_incompatible);

    IR::BFN::ParserState *postorder(IR::BFN::ParserState *state) override;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_SPLIT_GREEDY_PARSER_STATES_H_ */
