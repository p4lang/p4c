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

#ifndef PAD_ALIGNMENT_H_
#define PAD_ALIGNMENT_H_

#include "lib/algorithm.h"

static inline int getAlignment(int bitSize) {
    int nextByteBoundary = 8 * ROUNDUP(bitSize, 8);
    return (nextByteBoundary - bitSize);
}

#endif /* PAD_ALIGNMENT_H_ */
