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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_

#include "ir/visitor.h"

using namespace P4;

struct CountStridedHeaderRefs : public Inspector {
    std::map<cstring, std::set<unsigned>> header_stack_to_indices;

    bool preorder(const IR::HeaderStackItemRef *hs);
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_COUNT_STRIDED_HEADER_REFS_H_ */
