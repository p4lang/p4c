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

#include "backends/tofino/bf-asm/tofino/phv.h"

void Target::Tofino::Phv::init_regs(::Phv &phv) {
    // Allocating Tofino registers so the uids map to register encodings
    static const struct {
        char code[4];
        unsigned size, count;
    } sizes[] = {{"W", 32, 64},  {"B", 8, 64},  {"H", 16, 96}, {"", 0, 32},
                 {"TW", 32, 32}, {"TB", 8, 32}, {"TH", 16, 48}};
    unsigned uid = 0;
    phv.regs.resize(NUM_PHV_REGS);
    for (unsigned i = 0; i < sizeof sizes / sizeof *sizes; i++) {
        for (unsigned j = 0; j < sizes[i].count; j++, uid++) {
            auto reg = phv.regs[uid] = new Register;
            memset(reg->name, 0, sizeof(reg->name));
            reg->type = (uid >= FIRST_TPHV) ? Register::TAGALONG : Register::NORMAL;
            reg->index = j;
            reg->uid = uid;
            reg->size = sizes[i].size;
            if (sizes[i].size) {
                char buf[8];
                snprintf(buf, sizeof(buf), "R%d", uid);
                phv.names[INGRESS][buf][0].slice = ::Phv::Slice(*reg, 0, sizes[i].size - 1);
                phv.names[EGRESS][buf][0].slice = ::Phv::Slice(*reg, 0, sizes[i].size - 1);
                snprintf(reg->name, sizeof(reg->name), "%.2s%d", sizes[i].code, j);
                phv.names[INGRESS][reg->name][0].slice = ::Phv::Slice(*reg, 0, sizes[i].size - 1);
                phv.names[EGRESS][reg->name][0].slice = ::Phv::Slice(*reg, 0, sizes[i].size - 1);
            }
        }
    }
    BUG_CHECK(uid == phv.regs.size());
}

static bitvec tagalong_group(int n) {
    bitvec rv;
    rv.setrange(
        Target::Tofino::Phv::FIRST_8BIT_TPHV + n * (Target::Tofino::Phv::COUNT_8BIT_TPHV / 8),
        Target::Tofino::Phv::COUNT_8BIT_TPHV / 8);
    rv.setrange(
        Target::Tofino::Phv::FIRST_16BIT_TPHV + n * (Target::Tofino::Phv::COUNT_16BIT_TPHV / 8),
        Target::Tofino::Phv::COUNT_16BIT_TPHV / 8);
    rv.setrange(
        Target::Tofino::Phv::FIRST_32BIT_TPHV + n * (Target::Tofino::Phv::COUNT_32BIT_TPHV / 8),
        Target::Tofino::Phv::COUNT_32BIT_TPHV / 8);
    return rv;
}
const bitvec Target::Tofino::Phv::tagalong_groups[8] = {
    tagalong_group(0), tagalong_group(1), tagalong_group(2), tagalong_group(3),
    tagalong_group(4), tagalong_group(5), tagalong_group(6), tagalong_group(7)};
