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

#include "backends/tofino/bf-asm/tofino/exact_match.h"

void Target::Tofino::ExactMatchTable::setup_ways() {
    ::ExactMatchTable::setup_ways();
    for (auto &row : layout) {
        int first_way = -1;
        for (auto &ram : row.memunits) {
            int way = way_map.at(ram).way;
            if (first_way < 0) {
                first_way = way;
            } else if (ways[way].group_xme != ways[first_way].group_xme) {
                error(row.lineno,
                      "Ways %d and %d of table %s share address bus on row %d, "
                      "but use different hash groups",
                      first_way, way, name(), row.row);
                break;
            }
        }
    }
}
