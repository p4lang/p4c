/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/map_direct_externs.h"

namespace P4::P4Tools::P4Testgen::Tofino {

bool MapDirectExterns::preorder(const IR::Declaration_Instance *declInstance) {
    declaredExterns[declInstance->name] = declInstance;
    return true;
}

bool MapDirectExterns::preorder(const IR::P4Table *table) {
    // Try to either get registers from the table.
    const auto *impl = table->properties->getProperty("registers");
    if (impl == nullptr) {
        return false;
    }
    // Cannot map a temporary mapped direct extern without a register.
    const auto *selectorExpr = impl->value->checkedTo<IR::ExpressionValue>();
    if (selectorExpr->expression->is<IR::ConstructorCallExpression>()) {
        return true;
    }
    // Try to find the extern in the declared instances.
    const auto *implPath = selectorExpr->expression->to<IR::PathExpression>();
    auto implementationLabel = implPath->path->name.name;

    // If the extern is not in the list of declared instances, move on.
    auto it = declaredExterns.find(implementationLabel);
    if (it == declaredExterns.end()) {
        return true;
    }
    // Tofino does not support direct externs attached to multiple tables.
    const auto *declInstance = it->second;
    if (directExternMap.find(declInstance->controlPlaneName()) != directExternMap.end()) {
        FATAL_ERROR(
            "Direct extern was already mapped to a table. It can not be attached to two tables. ");
        return true;
    }
    directExternMap.emplace(declInstance->controlPlaneName(), table);
    return true;
}

const DirectExternMap &MapDirectExterns::getDirectExternMap() { return directExternMap; }

}  // namespace P4::P4Tools::P4Testgen::Tofino
