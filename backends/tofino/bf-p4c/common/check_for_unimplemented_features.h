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

#ifndef EXTENSIONS_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_
#define EXTENSIONS_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_

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
        IR::Node* preorder(IR::Cast* cast) override {
            return cast->expr->clone();
        }
    };

    /// @returns BOP in modify_field(x, BOP), if BOP is a binary operation
    /// expression.
    std::optional<const IR::Operation_Binary*> getSrcBinop(const IR::MAU::Primitive* p) const;

    /// @returns true if @p is a P4_16 encoding of a P4_14 funnel shift
    /// operation, eg. modify_field(x, (y ++ z) >> n).
    bool isFunnelShift(const IR::MAU::Primitive* p) const;

    /// @returns true if @p is a P4_16 encoding of a P4_14 bitmasked modify_field
    /// operation, eg. modify_field(x, val, 0xAA).
    bool isModBitMask(const IR::MAU::Primitive* p) const;

    bool preorder(const IR::MAU::Primitive*) override;
};

class CheckForUnimplementedFeatures : public PassManager {
 public:
    CheckForUnimplementedFeatures() {
        addPasses({
            new CheckOperations(),
        });
    }
};

#endif  /* EXTENSIONS_BF_P4C_COMMON_CHECK_FOR_UNIMPLEMENTED_FEATURES_H_ */
