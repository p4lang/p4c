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

#ifndef BF_P4C_ARCH_ADD_T2NA_META_H_
#define BF_P4C_ARCH_ADD_T2NA_META_H_

#include "ir/ir.h"

namespace BFN {

// Check T2NA metadata and add missing
class AddT2naMeta final : public Modifier {
 public:
    AddT2naMeta() { setName("AddT2naMeta"); }

    // Check T2NA metadata structures and headers and add missing fields
    void postorder(IR::Type_StructLike *typeStructLike) override;
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_ADD_T2NA_META_H_ */
