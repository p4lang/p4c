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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_ELIMINATE_EMPTY_STATES_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_ELIMINATE_EMPTY_STATES_H_

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief Eliminates empty states.
 *
 * If before parser lowering, a state is empty and has unconditional transition
 * leaving the state, we can safely eliminate this state.
 */
struct EliminateEmptyStates : public ParserTransform {
    const CollectParserInfo &parser_info;

    explicit EliminateEmptyStates(const CollectParserInfo &pi) : parser_info(pi) {}

    bool is_empty(const IR::BFN::ParserState *state);

    const IR::BFN::Transition *get_unconditional_transition(const IR::BFN::ParserState *state);

    IR::Node *preorder(IR::BFN::Transition *transition) override;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_ELIMINATE_EMPTY_STATES_H_ */
