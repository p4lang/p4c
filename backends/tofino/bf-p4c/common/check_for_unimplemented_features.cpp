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

#include "bf-p4c/common/check_for_unimplemented_features.h"

#include "lib/error_reporter.h"

std::optional<const IR::Operation_Binary *> CheckOperations::getSrcBinop(
    const IR::MAU::Primitive *prim) const {
    prim = prim->apply(RemoveCasts());
    if (prim->name != "modify_field") return std::nullopt;
    if (prim->operands.size() < 2) return std::nullopt;
    if (auto *binop = prim->operands[1]->to<IR::Operation_Binary>()) return binop;
    return std::nullopt;
}

bool CheckOperations::isModBitMask(const IR::MAU::Primitive *prim) const {
    auto *binop = getSrcBinop(prim).value_or(nullptr);
    if (!binop || binop->getStringOp() != "|") return false;
    auto leftBinop = binop->left->to<IR::Operation_Binary>();
    auto rightBinop = binop->right->to<IR::Operation_Binary>();
    if (!leftBinop || !rightBinop) return false;
    return leftBinop->getStringOp() == "&" && rightBinop->getStringOp() == "&";
}

bool CheckOperations::preorder(const IR::MAU::Primitive *prim) {
    if (isModBitMask(prim))
        error("The following operation is not yet supported: %1%", prim->srcInfo);
    return true;
}
