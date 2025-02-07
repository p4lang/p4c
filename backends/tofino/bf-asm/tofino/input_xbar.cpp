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

#include "backends/tofino/bf-asm/tofino/input_xbar.h"

template <>
void InputXbar::write_galois_matrix(Target::Tofino::mau_regs &regs, HashTable id,
                                    const std::map<int, HashCol> &mat) {
    int parity_col = -1;
    BUG_CHECK(id.type == HashTable::EXACT, "not an exact hash table %d", id.type);
    if (hash_table_parity.count(id) && !options.disable_gfm_parity) {
        parity_col = hash_table_parity[id];
    }
    auto &hash = regs.dp.xbar_hash.hash;
    std::set<int> gfm_rows;
    for (auto &col : mat) {
        int c = col.first;
        // Skip parity column encoding, if parity is set overall parity is
        // computed later below
        if (c == parity_col) continue;
        const HashCol &h = col.second;
        for (int word = 0; word < 4; word++) {
            unsigned data = h.data.getrange(word * 16, 16);
            unsigned valid = (h.valid >> word * 2) & 3;
            if (data == 0 && valid == 0) continue;
            auto &w = hash.galois_field_matrix[id.index * 4 + word][c];
            w.byte0 = data & 0xff;
            w.byte1 = (data >> 8) & 0xff;
            w.valid0 = valid & 1;
            w.valid1 = (valid >> 1) & 1;
            gfm_rows.insert(id.index * 4 + word);
        }
    }
    // A GFM row can be shared by multiple tables. In most cases the columns are
    // non overlapping but if they are overlapping the GFM encodings must be the
    // same (e.g. ATCAM tables). The input xbar has checks to determine which
    // cases are valid.
    // The parity must be computed for all columns within the row and set into
    // the parity column.
    if (parity_col >= 0) {
        for (auto r : gfm_rows) {
            int hp_byte0 = 0, hp_byte1 = 0;
            int hp_valid0 = 0, hp_valid1 = 0;
            for (auto c = 0; c < 52; c++) {
                if (c == parity_col) continue;
                auto &w = hash.galois_field_matrix[r][c];
                hp_byte0 ^= w.byte0;
                hp_byte1 ^= w.byte1;
                hp_valid0 ^= w.valid0;
                hp_valid1 ^= w.valid1;
            }
            auto &w_hp = hash.galois_field_matrix[r][parity_col];
            w_hp.byte0.rewrite();
            w_hp.byte1.rewrite();
            w_hp.valid0.rewrite();
            w_hp.valid1.rewrite();
            w_hp.byte0 = hp_byte0;
            w_hp.byte1 = hp_byte1;
            w_hp.valid0 = hp_valid0;
            w_hp.valid1 = hp_valid1;
        }
    }
}

template void InputXbar::write_galois_matrix(Target::Tofino::mau_regs &regs, HashTable id,
                                             const std::map<int, HashCol> &mat);
