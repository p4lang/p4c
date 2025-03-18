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

#include "backends/tofino/bf-asm/jbay/phv.h"

void Target::JBay::Phv::init_regs(::Phv &phv) {
    // Allocating JBay regs so the uids map to mau register encodings
    static const struct {
        char code[2];
        unsigned size, count;
    } groups[] = {{"W", 32, 4}, {"B", 8, 4}, {"H", 16, 6}};
    static const struct {
        char code[2];
        Register::type_t type;
        unsigned count;
    } types[] = {{"", Register::NORMAL, 12}, {"M", Register::MOCHA, 4}, {"D", Register::DARK, 4}};
    unsigned uid = 0;
    unsigned byte = 0;
    unsigned deparser_id = 0;
    phv.regs.resize(280);
    for (unsigned i = 0; i < sizeof groups / sizeof *groups; i++) {
        unsigned idx[sizeof types / sizeof *types] = {0};
        for (unsigned j = 0; j < groups[i].count; j++) {
            for (unsigned k = 0; k < sizeof types / sizeof *types; k++) {
                for (unsigned l = 0; l < types[k].count; l++, idx[k]++, uid++) {
                    auto reg = new Register;
                    phv.regs[uid] = reg;
                    memset(reg->name, 0, sizeof(reg->name));
                    snprintf(reg->name, sizeof(reg->name), "%.2s%.2s%d", types[k].code,
                             groups[i].code, idx[k]);
                    reg->type = types[k].type;
                    reg->index = idx[k];
                    reg->uid = uid;
                    reg->size = groups[i].size;
                    if (reg->type == Register::DARK) {
                        reg->parser_id_ = reg->deparser_id_ = -1;
                    } else {
                        reg->parser_id_ = byte / 2U;
                        reg->deparser_id_ = deparser_id++;
                        byte += reg->size / 8U;
                    }
                    phv.names[INGRESS][reg->name][0].slice = ::Phv::Slice(*reg, 0, reg->size - 1);
                    phv.names[EGRESS][reg->name][0].slice = ::Phv::Slice(*reg, 0, reg->size - 1);
                    phv.names[GHOST][reg->name][0].slice = ::Phv::Slice(*reg, 0, reg->size - 1);
                }
            }
        }
    }
    BUG_CHECK(uid == phv.regs.size());
    BUG_CHECK(deparser_id == 224);
    BUG_CHECK(byte == 512);
}
