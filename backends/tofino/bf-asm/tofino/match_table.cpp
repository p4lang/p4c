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

/* mau table template specializations for tofino -- #included directly in match_tables.cpp */

template <>
void MatchTable::write_next_table_regs(Target::Tofino::mau_regs &regs, Table *tbl) {
    auto &merge = regs.rams.match.merge;
    // Copies the values directly from the hit map provided by the compiler directly into the
    // map
    if (!tbl->get_hit_next().empty()) {
        merge.next_table_map_en |= (1U << logical_id);
        auto &mp = merge.next_table_map_data[logical_id];
        ubits<8> *map_data[8] = {&mp[0].next_table_map_data0, &mp[0].next_table_map_data1,
                                 &mp[0].next_table_map_data2, &mp[0].next_table_map_data3,
                                 &mp[1].next_table_map_data0, &mp[1].next_table_map_data1,
                                 &mp[1].next_table_map_data2, &mp[1].next_table_map_data3};
        int index = 0;
        for (auto &n : tbl->get_hit_next()) *map_data[index++] = n.next_table_id();
    }

    merge.next_table_format_data[logical_id].match_next_table_adr_mask = next_table_adr_mask;

    /**
     * Unfortunately for the compiler/driver integration, this register is both required
     * to be owned by the compiler and the driver.  The driver is responsible for programming
     * this register when the default action of a table is specified.  The value written
     * is the next_table_full of that particular action.
     *
     * However, the compiler owns this register in the following scenarios:
     *     1. For match_with_no_key tables, where the pathway is through the hit pathway,
     *        the driver does not touch this register, as the values are actually reversed
     *     2. For a table that is split into multiple tables, the driver only writes the
     *        last value.  Thus the compiler now sets up this register for all tables
     *        before this.
     */
    merge.next_table_format_data[logical_id].match_next_table_adr_miss_value =
        tbl->get_miss_next().next_table_id();
    /**
     * The next_table_format_data register is built up of three values:
     *     - match_next_table_adr_miss_value - Configurable at runtime
     *     - match_next_table_adr_mask - Static Config
     *     - match_next_table_adr_default - Static Config
     *
     * In order to reprogram the register at runtime, the driver must have all three values to
     * not require a hardware read, even though only one is truly programmable.  Thus in the
     * context JSON, we provide the two extra values in an extremely poorly named JSON
     *
     * ERROR: Driver doesn't read the match_next_table_adr_default
     * "default_next_table_mask" - match_next_table_adr_mask
     * "default_next_table" - Only required if a table has no default_action specified, which is
     *      only a Glass value.  This could always be 0.  Perhaps we can remove from Brig through
     *      compiler version?
     *
     */
}

template <>
void MatchTable::write_regs(Target::Tofino::mau_regs &regs, int type, Table *result) {
    write_common_regs<Target::Tofino>(regs, type, result);
}
