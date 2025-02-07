/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ternary_match.h"

#include "stage.h"

void Target::Tofino::TernaryMatchTable::pass1() {
    ::TernaryMatchTable::pass1();
    // Dont allocate id (mark them as used) for empty ternary tables (keyless
    // tables). Keyless tables are marked ternary with a tind. They are setup by
    // the driver to always miss (since there is no match) and run the miss
    // action. The miss action is associated with the logical table space and
    // does not need a tcam id association. This saves tcams ids to be assigned
    // to actual ternary tables. This way we can have 8 real ternary match
    // tables within a stage and not count the keyless among them.
    // NOTE: The tcam_id is never assigned for these tables and will be set to
    // default (-1). We also disable registers associated with tcam_id for this
    // table.
    if (layout_size() != 0) {
        alloc_id("tcam", tcam_id, stage->pass1_tcam_id, TCAM_TABLES_PER_STAGE, false,
                 stage->tcam_id_use);
        physical_ids[tcam_id] = 1;
    }
    // alloc_busses(stage->tcam_match_bus_use); -- now hardwired
}

void Target::Tofino::TernaryIndirectTable::pass1() {
    ::TernaryIndirectTable::pass1();
    alloc_busses(stage->tcam_indirect_bus_use, Layout::TIND_BUS);
}

void Target::Tofino::TernaryMatchTable::check_tcam_match_bus(
    const std::vector<Table::Layout> &layout) {
    for (auto &row : layout) {
        if (row.bus.empty()) continue;
        for (auto &tcam : row.memunits)
            if (row.bus.at(Table::Layout::SEARCH_BUS) != tcam.col)
                error(row.lineno, "Tcam match bus hardwired to tcam column");
    }
}
