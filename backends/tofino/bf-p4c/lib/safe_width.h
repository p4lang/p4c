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
