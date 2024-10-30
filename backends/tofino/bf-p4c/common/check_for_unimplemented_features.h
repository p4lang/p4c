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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_

#include <optional>

#include "ir/ir.h"
#include "ir/pass_manager.h"

/** Check for the following unimplemented operations:
 *
 *  P4_16                                P4_14
 *  -----                                -----
 *  modify_field(x, (y ++ z) >> n)       funnel_shift_right(x, y, z, n)
 *  modify_field(x, (y ++ z) << n)       funnel_shift_left(x, y, z, n)
 *  modify_field(x, x & 85 | val & 170)  modify_field(x, val, 0xAA)
 */
class CheckOperations : public Inspector {
    // Helper that removes casts to make identifying unsupported operator
    // combinations easier.
    struct RemoveCasts : public Transform {
        IR::Node *preorder(IR::Cast *cast) override { return cast->expr->clone(); }
    };

    /// @returns BOP in modify_field(x, BOP), if BOP is a binary operation
    /// expression.
    std::optional<const IR::Operation_Binary *> getSrcBinop(const IR::MAU::Primitive *p) const;

    /// @returns true if @p is a P4_16 encoding of a P4_14 funnel shift
    /// operation, eg. modify_field(x, (y ++ z) >> n).
    bool isFunnelShift(const IR::MAU::Primitive *p) const;

    /// @returns true if @p is a P4_16 encoding of a P4_14 bitmasked modify_field
    /// operation, eg. modify_field(x, val, 0xAA).
    bool isModBitMask(const IR::MAU::Primitive *p) const;

    bool preorder(const IR::MAU::Primitive *) override;
};

class CheckForUnimplementedFeatures : public PassManager {
 public:
    CheckForUnimplementedFeatures() {
        addPasses({
            new CheckOperations(),
        });
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_ */
