/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
