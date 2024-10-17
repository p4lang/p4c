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

#ifndef BF_P4C_PHV_SPLIT_PADDING_H_
#define BF_P4C_PHV_SPLIT_PADDING_H_

#include <ir/ir.h>

#include "ir/visitor.h"

class PhvInfo;

/**
 * @brief Splits padding after PHV allocation to prevent extracted padding spanning multiple
 * containers.
 *
 * Prevent padding from spanning multiple containers to help passes like UpdateParserWriteMode that
 * unifies the write modes for all fields in a container.
 */
class SplitPadding : public Transform {
 protected:
    const PhvInfo& phv;

    const IR::Node* preorder(IR::BFN::ParserState* state) override;

 public:
    explicit SplitPadding(const PhvInfo& phv) : phv(phv) { }
};

#endif  /* BF_P4C_PHV_SPLIT_PADDING_H_ */
