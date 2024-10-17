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

#ifndef EXTENSIONS_BF_P4C_ARCH_FROMV1_0_FIELD_LIST_H_
#define EXTENSIONS_BF_P4C_ARCH_FROMV1_0_FIELD_LIST_H_

#include "ir/ir.h"
#include "frontends/p4-14/fromv1.0/converters.h"
#include "bf-p4c/midend/path_linearizer.h"

namespace P4 {
namespace P4V1 {

class FieldListConverter {
    FieldListConverter();
    static FieldListConverter singleton;
 public:
    static const IR::Node *convertFieldList(const IR::Node *);
};

}  // namespace P4V1
}  // namespace P4

#endif /* EXTENSIONS_BF_P4C_ARCH_FROMV1_0_FIELD_LIST_H_ */
