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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_BUFFER_REQUIREMENTS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_BUFFER_REQUIREMENTS_H_

#include "bf-p4c/parde/parde_visitor.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParser
 * @brief Computes the number of bytes which must be available for each parser match
 *        to avoid a stall.
 */
class ComputeBufferRequirements : public ParserModifier {
    void postorder(IR::BFN::LoweredParserMatch *match) override;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_BUFFER_REQUIREMENTS_H_ */
