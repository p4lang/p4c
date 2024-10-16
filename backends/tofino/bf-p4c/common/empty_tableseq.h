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

#ifndef BF_P4C_COMMON_EMPTY_TABLESEQ_H_
#define BF_P4C_COMMON_EMPTY_TABLESEQ_H_

#include "bf-p4c/mau/mau_visitor.h"

using namespace P4;

/// Adds empty table sequences to implicit fall-through paths in the program. For example, when
/// an 'if' statement has no 'else', this adds an empty table sequence to the 'else' branch.
class AddEmptyTableSeqs : public MauModifier {
    void postorder(IR::BFN::Pipe* pipe) override;
    void postorder(IR::MAU::Table* tbl) override;

 public:
    AddEmptyTableSeqs() {}
};

#endif /* BF_P4C_COMMON_EMPTY_TABLESEQ_H_ */
