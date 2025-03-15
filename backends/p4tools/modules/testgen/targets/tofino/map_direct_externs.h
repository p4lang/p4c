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

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_MAP_DIRECT_EXTERNS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_MAP_DIRECT_EXTERNS_H_

#include <map>

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/// A mapping of the control plane name of extern declarations which are associated with a table.
/// Such an extern is referred to as direct extern.
/// We are using the cstring name and not an IR::Declaration pointer for the mapping because other
/// passes may clone or transform the IR::Declaration node, invalidating this mapping.
using DirectExternMap = std::map<cstring, const IR::P4Table *>;

/// A lightweight visitor, which collects all the declarations in the program then checks whether a
/// table is referencing the declaration as an direct extern. The direct externs are collected in a
/// map. Currently checks for "registers"  properties in the table.
class MapDirectExterns : public Inspector {
 private:
    /// List of the declared instances in the program.
    std::map<cstring, const IR::Declaration_Instance *> declaredExterns;

    /// Maps direct extern declarations to the table they are attached to.
    DirectExternMap directExternMap;

 public:
    bool preorder(const IR::Declaration_Instance *declInstance) override;

    bool preorder(const IR::P4Table *table) override;

    /// @returns the list of direct externs and their corresponding table.
    const DirectExternMap &getDirectExternMap();
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_MAP_DIRECT_EXTERNS_H_*/
