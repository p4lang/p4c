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

#include "bf-p4c/midend/check_register_actions.h"

#include "bf-p4c/lib/safe_width.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {

bool CheckRegisterActions::preorder(const IR::Declaration_Instance *di) {
    if (di->arguments->empty()) return false;

    const auto specType = di->type->to<IR::Type_Specialized>();
    if (!specType) return false;

    const auto baseNameType = specType->baseType->to<IR::Type_Name>();
    if (!baseNameType) return false;

    const cstring &name = baseNameType->path->name.name;
    if (!name.startsWith("RegisterAction")) return false;

    if (specType->arguments->size() < 2) return false;

    const IR::Type *regActionEntryType = specType->arguments->at(0);
    const IR::Type *regActionIndexType = specType->arguments->at(1);

    if (!regActionEntryType || !regActionIndexType) return false;
    if (regActionEntryType->is<IR::Type_Dontcare>()) return false;
    if (regActionIndexType->is<IR::Type_Dontcare>()) return false;

    int regActionEntryWidth =
        safe_width_bits(typeMap->getTypeType(regActionEntryType->getNode(), /*notNull=*/true));
    int regActionIndexWidth =
        safe_width_bits(typeMap->getTypeType(regActionIndexType->getNode(), /*notNull=*/true));

    auto regArgExpr = di->arguments->at(0)->expression->to<IR::PathExpression>();
    const IR::Vector<IR::Type> *regTypeArgs = nullptr;
    if (auto regSpecType = regArgExpr->type->to<IR::Type_Specialized>()) {
        regTypeArgs = regSpecType->arguments;
    } else if (auto regSpecCanType = regArgExpr->type->to<IR::Type_SpecializedCanonical>()) {
        regTypeArgs = regSpecCanType->arguments;
    }

    if (!regTypeArgs || regTypeArgs->size() < 2) return false;
    const IR::Type *regEntryType = regTypeArgs->at(0);
    const IR::Type *regIndexType = regTypeArgs->at(1);
    if (!regEntryType || !regIndexType) return false;

    int regEntryWidth =
        safe_width_bits(typeMap->getTypeType(regEntryType->getNode(), /*notNull=*/true));
    int regIndexWidth =
        safe_width_bits(typeMap->getTypeType(regIndexType->getNode(), /*notNull=*/true));

    if (regActionEntryWidth == regEntryWidth)
        // All ok.
        return false;

    // Check if the widths are at least equal when index widths are accounted for
    unsigned indexDiff = abs(regActionIndexWidth - regIndexWidth);

    bool emitTypeError;
    if (regIndexWidth >= regActionIndexWidth)
        emitTypeError = (regEntryWidth * (1LL << indexDiff)) != regActionEntryWidth;
    else
        emitTypeError = (regActionEntryWidth * (1LL << indexDiff)) != regEntryWidth;

    if (emitTypeError)
        error("RegisterAction %1% does not match the type of register it uses", di->name);

    return false;
}

}  // namespace BFN
