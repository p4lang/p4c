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

#ifndef BF_P4C_ARCH_FROMV1_0_PARSER_COUNTER_H_
#define BF_P4C_ARCH_FROMV1_0_PARSER_COUNTER_H_

#include "bf-p4c/arch/fromv1.0/v1_converters.h"

namespace BFN {
namespace V1 {

class ParserCounterConverter : public StatementConverter {
    void cannotFit(const IR::AssignmentStatement *stmt, const char *what);

 public:
    explicit ParserCounterConverter(ProgramStructure *structure) : StatementConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::AssignmentStatement *node) override;
};

class ParserCounterSelectionConverter : public PassManager {
 public:
    ParserCounterSelectionConverter();
};

}  // namespace V1
}  // namespace BFN

#endif /* BF_P4C_ARCH_FROMV1_0_PARSER_COUNTER_H_ */
