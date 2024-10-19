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
