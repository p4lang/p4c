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

#ifndef BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_
#define BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

namespace P4 {
class TypeMap;
}  // namespace P4

namespace BFN {

/**
 * \ingroup midend
 * \brief Check for unsupported features in the backend compiler.
 */
class CheckUnsupported final : public Inspector {
    bool preorder(const IR::PathExpression* path_expression) override;
    void postorder(const IR::P4Table *) override;
    bool preorder(const IR::Declaration_Instance *instance) override;

 public:
    explicit CheckUnsupported(P4::ReferenceMap *, P4::TypeMap*) {}
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_ */
