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

#ifndef BF_P4C_COMMON_CHECK_HEADER_REFS_H_
#define BF_P4C_COMMON_CHECK_HEADER_REFS_H_

#include "bf-p4c/common/utils.h"
#include "ir/ir.h"

using namespace P4;

/**
 * Once the CopyHeaderEliminator and HeaderPushPo passes have run, HeaderRef
 * objects should no longer exist in the IR as arguments, only as children of
 * Member nodes.  This pass checks and throws a BUG if this property does not
 * hold.
 */
class CheckForHeaders final : public Inspector {
    bool preorder(const IR::Member *) { return false; }
    bool preorder(const IR::HeaderRef *h) {
        if (h->toString() == "ghost::gh_intr_md") return false;
        BUG("Header present in IR not under Member: %s", h->toString());
    }
};

#endif /* BF_P4C_COMMON_CHECK_HEADER_REFS_H_ */
