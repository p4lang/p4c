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

#ifndef BACKENDS_TOFINO_BF_ASM_HASH_DIST_H_
#define BACKENDS_TOFINO_BF_ASM_HASH_DIST_H_

#include <vector>

#include "asm-types.h"

class Stage;
class Table;

/* config for a hash distribution unit in match central.
 * FIXME -- need to abstract this away rather than have it be explicit
 * FIXME -- in the asm code */

struct HashDistribution {
    // FIXME -- need less 'raw' data for this */
    Table *tbl = 0;
    int lineno = -1;
    int hash_group = -1, id = -1;
    int shift = 0, mask = 0, expand = -1;
    bool meter_pre_color = false;
    int meter_mask_index = 0;
    enum {
        IMMEDIATE_HIGH = 1 << 0,
        IMMEDIATE_LOW = 1 << 1,
        METER_ADDRESS = 1 << 2,
        STATISTICS_ADDRESS = 1 << 3,
        ACTION_DATA_ADDRESS = 1 << 4,
        HASHMOD_DIVIDEND = 1 << 5
    };
    unsigned xbar_use = 0;
    enum delay_type_t { SELECTOR = 0, OTHER = 1 };
    delay_type_t delay_type = SELECTOR;
    bool non_linear = false;
    HashDistribution(int id, value_t &data, unsigned u = 0);
    static void parse(std::vector<HashDistribution> &out, const value_t &v, unsigned u = 0);
    bool compatible(HashDistribution *a);
    void pass1(Table *tbl, delay_type_t dt, bool nl);
    void pass2(Table *tbl);
    template <class REGS>
    void write_regs(REGS &regs, Table *);
};

#endif /* BACKENDS_TOFINO_BF_ASM_HASH_DIST_H_ */
