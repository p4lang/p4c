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

#ifndef BF_P4C_COMMON_SLICE_H_
#define BF_P4C_COMMON_SLICE_H_

#include "ir/ir.h"
#include "lib/safe_vector.h"

using namespace P4;

/** @returns the lo and hi bits for a give expression
 */
std::pair<int, int> getSliceLoHi(const IR::Expression *e);

/**
 *  MakeSlice -- slice an expression.
 *  Create a slice extracting the bits lo..hi from an expression.
 *  This may be an IR:Slice or not.  Any slices on slices or slices on
 *  constants are folded to the simplest form.  Slices that extract off
 *  the top of an expression (past the msb) are implicitly zero-extended
 *  Also strength-reduce slices on bitwise operations
 */
const IR::Expression *MakeSlice(const IR::Expression *e, int lo, int hi);
inline const IR::Expression *MakeSlice(const IR::Expression *e, le_bitrange slice) {
    return MakeSlice(e, slice.lo, slice.hi);
}

/**
 * MakeSliceDestination -- slice an expression used as the destination in an instruction.  Creates
 * a slice extracting the bits lo..hi from an expression. This may be an IR:Slice or not.  Any
 * slices on slices or slices on constants are folded to the simplest form. Slices that extract
 * off the top of an expression (past the msb) are implicitly zero-extended.
 */
const IR::Expression *MakeSliceDestination(const IR::Expression *e, int lo, int hi);

/**
 * MakeSliceSource -- slice an expression used as the destination in an instruction.  Creates a
 * slice extracting the bits lo..hi from an expression. This may be an IR:Slice or not.  Any slices
 * on slices or slices on constants are folded to the simplest form. Slices that extract off the
 * top of an expression (past the msb) are implicitly zero-extended. It also accounts for the case
 * where the destination used with this read is not aligned with the read.
 */
const IR::Expression *MakeSliceSource(const IR::Expression *read, int lo, int hi,
                                      const IR::Expression *write);

/**
 *  Changes an IR::Mask to a list of IR::Slices.  The phv.field function cannot interpret
 *  IR::Masks, as they can be non-contiguous, so this will convert that to a bunch of slices.
 */
safe_vector<const IR::Expression *> convertMaskToSlices(const IR::Mask *m);

/**
 * MakeWrappedSlice - slice an expression around a boundary
 * This is necessary if a source goes around the boundary in a deposit-field.  Currently
 * the only allowed Expression to be used in this expression is a MultiOperand
 */
const IR::Expression *MakeWrappedSlice(const IR::Expression *e, int lo, int hi, int wrap_size);

#endif /* BF_P4C_COMMON_SLICE_H_ */
