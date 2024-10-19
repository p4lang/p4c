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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_SPLIT_PARSER_STATE_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_SPLIT_PARSER_STATE_H_

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "parde_visitor.h"

/**
 * @ingroup LowerParserIR
 * @brief Splits parser states into multiple states to account for HW resource constraints
 *        of a single parser state.
 *
 * After PHV allocation, we have the concrete view of what size of
 * PHV container each field is allocated in. We can then finalize
 * the parser IR. Specifically, parser states may need to split into
 * multiple states to account for various resource contraints of
 * a single parser state, these are
 *
 *    - Input buffer (can only see 32 bytes of packet data in a state)
 *    - Extractors for PHV container
 *    - Constant extracts
 *    - Parser counter
 *    - Checksum calculation
 *    - CLOT issurance (JBAY)
 *    - Stage rules (FTR)
 */
struct SplitParserState : public PassManager {
    SplitParserState(const PhvInfo &phv, ClotInfo &clot, const CollectParserInfo &parser_info);
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_SPLIT_PARSER_STATE_H_ */
