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
