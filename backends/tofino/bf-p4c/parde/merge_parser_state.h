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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_MERGE_PARSER_STATE_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_MERGE_PARSER_STATE_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"

/**
 * @ingroup parde
 * @brief Merges a chain of states into a large state (before parser lowering).
 * Find the longest chain that:
 * Forall states expect for the last tail state:
 *   1. Only has one default branch.
 * Forall states incluing the last:
 *   2. Is not a merge point of multiple states.
 *   3. Does not have match resiger def/use dependency.
 */
struct MergeParserStates : public PassManager {
    MergeParserStates();
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_MERGE_PARSER_STATE_H_ */
