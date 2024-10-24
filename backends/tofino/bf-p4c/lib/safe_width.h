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

#ifndef BF_P4C_LIB_SAFE_WIDTH_H_
#define BF_P4C_LIB_SAFE_WIDTH_H_

#include "ir/ir.h"

/// Get the width of a type, or 0 if the type has no width
inline int safe_width_bits(const IR::Type *type) {
    int width = 0;
    if (!type->is<IR::Type_Enum>() && !type->is<IR::Type_Error>() &&
        !type->is<IR::Type_Unknown>() && !type->is<IR::Type_Dontcare>())
        width = type->width_bits();
    return width;
}

/// Get the width of a type, or 0 if the type has no width
inline int safe_width_bits(const IR::Expression *e) { return safe_width_bits(e->type); }

#endif /* BF_P4C_LIB_SAFE_WIDTH_H_ */
