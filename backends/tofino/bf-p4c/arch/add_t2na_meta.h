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


#ifndef BF_P4C_ARCH_ADD_T2NA_META_H_
#define BF_P4C_ARCH_ADD_T2NA_META_H_

#include "ir/ir.h"

namespace BFN {

// Check T2NA metadata and add missing
class AddT2naMeta final : public Modifier {
 public:
    AddT2naMeta() { setName("AddT2naMeta"); }

    // Check T2NA metadata structures and headers and add missing fields
    void postorder(IR::Type_StructLike* typeStructLike) override;
};

}  // namespace BFN

#endif  /* BF_P4C_ARCH_ADD_T2NA_META_H_ */
