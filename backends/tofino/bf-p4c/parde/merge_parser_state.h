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
