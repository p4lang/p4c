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

#ifndef BF_P4C_MAU_TABLE_SEQDEPS_H_
#define BF_P4C_MAU_TABLE_SEQDEPS_H_

#include "mau_visitor.h"
#include "field_use.h"

using namespace P4;

class TableFindSeqDependencies : public MauModifier {
    FieldUse    uses;
    profile_t init_apply(const IR::Node *root) override;
    void postorder(IR::MAU::TableSeq *) override;

 public:
    explicit TableFindSeqDependencies(const PhvInfo& p) : uses(p) { }
};

#endif /* BF_P4C_MAU_TABLE_SEQDEPS_H_ */
