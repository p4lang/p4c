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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_ALLOCATE_PARSER_MATCH_REGISTER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_ALLOCATE_PARSER_MATCH_REGISTER_H_

/**
 * @ingroup LowerParserIR
 * @brief This pass performs the parser match register allocation.
 *
 * Parser performs state transitions (branching) by doing a match between the
 * match register and the lookup constants stored in the TCAM rows. The branch
 * words, coming from the input packet, need to be saved into into the match
 * register(s) in the previous, or an earlier state, depending on where the branch
 * word is seen in the input buffer.
 *
 * Therefore, we need to do this after parser states have been spilt according to
 * various resource constraints (see SplitParserState pass), so that each state has
 * a concrete view of the input buffer (we know exactly the branch words are).
 *
 * This is a classic register allocation problem (only simpler because we have no
 * spill area), and we take the classical approach. Specifically, the
 * implementation is broken down into the following steps (see inlined comments
 * from each of these passes):
 *
 *   1. Resolve out-of-buffer selects
 *   2. Use-def analysis
 *   3. Derive use-def interference
 *   4. Insert register read and write instructions in IR
 *   5. Adjust match constants
 */

struct AllocateParserMatchRegisters : public PassManager {
    explicit AllocateParserMatchRegisters(const PhvInfo &phv);

 private:
    /// Iteration count for allocation
    int iteration = 0;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_ALLOCATE_PARSER_MATCH_REGISTER_H_ */
