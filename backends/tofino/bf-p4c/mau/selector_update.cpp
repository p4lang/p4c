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
