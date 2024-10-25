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
