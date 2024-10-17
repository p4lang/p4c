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

#ifndef EXTENSIONS_BF_P4C_PARDE_COALESCE_LEARNING_H_
#define EXTENSIONS_BF_P4C_PARDE_COALESCE_LEARNING_H_

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

#endif /* EXTENSIONS_BF_P4C_PARDE_COALESCE_LEARNING_H_ */


