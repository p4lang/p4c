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

#include "bf-p4c/common/check_for_unimplemented_features.h"
#include "lib/error_reporter.h"

std::optional<const IR::Operation_Binary*>
CheckOperations::getSrcBinop(const IR::MAU::Primitive* prim) const {
    prim = prim->apply(RemoveCasts());
    if (prim->name != "modify_field") return std::nullopt;
    if (prim->operands.size() < 2) return std::nullopt;
    if (auto* binop = prim->operands[1]->to<IR::Operation_Binary>())
        return binop;
    return std::nullopt;
}

bool CheckOperations::isModBitMask(const IR::MAU::Primitive* prim) const {
    auto* binop = getSrcBinop(prim).value_or(nullptr);
    if (!binop || binop->getStringOp() != "|") return false;
    auto leftBinop = binop->left->to<IR::Operation_Binary>();
    auto rightBinop = binop->right->to<IR::Operation_Binary>();
    if (!leftBinop || !rightBinop) return false;
    return leftBinop->getStringOp() == "&" && rightBinop->getStringOp() == "&";
}

bool CheckOperations::preorder(const IR::MAU::Primitive* prim) {
    if (isModBitMask(prim))
        error("The following operation is not yet supported: %1%", prim->srcInfo);
    return true;
}
