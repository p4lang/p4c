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

#include "selector_update.h"

#include <vector>

#include "bf-p4c/mau/resource_estimate.h"

bool AddSelectorSalu::AddSaluIfNeeded::preorder(IR::MAU::Table *tbl) {
    std::vector<const IR::MAU::StatefulAlu *> toAdd;
    for (auto &att : tbl->attached) {
        if (auto sel = att->attached->to<IR::MAU::Selector>()) {
            if (!self.sel2salu[sel]) {
                auto *salu = new IR::MAU::StatefulAlu(sel);
                salu->name = IR::ID(sel->name + "$salu");
                salu->width = 1;
                // For the driver, the size of the stateful ALU must be the exact
                // number of entries within the stateful SRAM
                int ram_lines = SelectorRAMLinesPerEntry(sel);
                ram_lines *= sel->num_pools;
                ram_lines = ((ram_lines + 1023) / 1024) * 1024;
                salu->size = ram_lines * 128;
                salu->synthetic_for_selector = true;
                self.sel2salu[sel] = salu;
            }
            toAdd.push_back(self.sel2salu[sel]);
        }
    }
    for (auto salu : toAdd) tbl->attached.push_back(new IR::MAU::BackendAttached(salu));
    return true;
}
