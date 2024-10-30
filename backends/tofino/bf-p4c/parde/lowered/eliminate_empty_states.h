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
