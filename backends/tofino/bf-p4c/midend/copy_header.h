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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

// Whilst the original PR consolidated code, moving it out of the back-end and
// canonicalising the IR sooner, it caused ripples that caused issue to the PHV allocator.
// For now, we only partially implement the PR by setting `ENABLE_P4C3251 0`.
#define ENABLE_P4C3251 0

namespace BFN {

class CopyHeaders : public PassRepeated {
 public:
    CopyHeaders(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, P4::TypeChecking *typeChecking);
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_ */
