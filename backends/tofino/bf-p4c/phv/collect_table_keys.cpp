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

#include "bf-p4c/phv/collect_table_keys.h"

namespace PHV {

void CollectTableKeys::end_apply() {
    for (const auto &table_prop : table_props) {
        const auto &p = table_prop.second;
        LOG3("table: " << table_prop.first->name << ", is_tcam: " << p.is_tcam
                       << ", n_entries: " << p.n_entries);
        for (const auto &fs : p.keys) {
            LOG3("\tkey: " << fs);
        }
    }
}

bool CollectTableKeys::preorder(const IR::MAU::Table *tbl) {
    if (!tbl->match_table) return true;
    for (const IR::MAU::TableKey *matchKey : tbl->match_key) {
        // if (matchKey->for_selection()) continue;
        le_bitrange bits;
        const PHV::Field *f = phv.field(matchKey->expr, &bits);
        CHECK_NULL(f);
        table_props[tbl].keys.emplace(f, bits);
        table_props[tbl].is_range |= matchKey->for_range();
    }
    table_props[tbl].n_entries = get_n_entries(tbl);
    table_props[tbl].is_tcam = get_n_entries(tbl);
    return true;
}

int CollectTableKeys::get_n_entries(const IR::MAU::Table *tbl) const {
    int entries = 512;  // default number of entries

    if (tbl->layout.no_match_data()) entries = 1;
    if (tbl->layout.pre_classifier)
        entries = tbl->layout.pre_classifer_number_entries;
    else if (auto k = tbl->match_table->getConstantProperty("size"_cs))
        entries = k->asInt();
    else if (auto k = tbl->match_table->getConstantProperty("min_size"_cs))
        entries = k->asInt();

    if (tbl->layout.exact) {
        if (tbl->layout.ixbar_width_bits < ceil_log2(entries))
            entries = 1 << tbl->layout.ixbar_width_bits;
    }
    return entries;
}

}  // namespace PHV
