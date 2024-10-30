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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_COALESCE_LEARNING_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_COALESCE_LEARNING_H_

#include "ir/ir.h"
#include "parde_visitor.h"

/**
 * \ingroup LowerDeparserIR
 *
 * \brief Replaces consecutive extracts of the same container with a single extract.
 *
 * Digests cannot extract from the same 16 or 32 bit PHV consecutively, but for learning
 * digests, since we control the layout completely, we can set it up to just extract the
 * container once, and use the data from that container more than once.
 *
 * This pass detects consecutive extracts of the same container and replaces them with
 * a single extract, rewriting the control-plane info to match.  In theory we could combine
 * any extracts from the same container (not just adjacent ones) to reduce the number of
 * digest bytes needed
 */
class CoalesceLearning : public DeparserModifier {
    bool preorder(IR::BFN::LearningTableEntry *) override;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_COALESCE_LEARNING_H_ */
