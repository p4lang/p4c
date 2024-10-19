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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_

#include "ir/visitor.h"

using namespace P4;

struct CountStridedHeaderRefs : public Inspector {
    std::map<cstring, std::set<unsigned>> header_stack_to_indices;

    bool preorder(const IR::HeaderStackItemRef *hs);
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_ */
