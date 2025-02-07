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

#ifndef BF_ASM_TOFINO_PHV_H_
#define BF_ASM_TOFINO_PHV_H_

#include "../phv.h"

class Target::Tofino::Phv : public Target::Phv {
    friend class ::Phv;
    struct Register : public ::Phv::Register {
        int parser_id() const override { return uid; }
        int mau_id() const override { return uid < FIRST_TPHV ? uid : -1; }
        int ixbar_id() const override { return uid < FIRST_TPHV ? uid : -1; }
        int deparser_id() const override { return uid; }
    };
    void init_regs(::Phv &phv) override;
    target_t type() const override { return TOFINO; }
    unsigned mau_groupsize() const override { return 16; }

 public:
    enum {
        NUM_PHV_REGS = 368,
        FIRST_8BIT_PHV = 64,
        COUNT_8BIT_PHV = 64,
        FIRST_16BIT_PHV = 128,
        COUNT_16BIT_PHV = 96,
        FIRST_32BIT_PHV = 0,
        COUNT_32BIT_PHV = 64,
        FIRST_TPHV = 256,
        FIRST_8BIT_TPHV = 288,
        COUNT_8BIT_TPHV = 32,
        FIRST_16BIT_TPHV = 320,
        COUNT_16BIT_TPHV = 48,
        FIRST_32BIT_TPHV = 256,
        COUNT_32BIT_TPHV = 32,
    };
    static const bitvec tagalong_groups[8];
};

#endif /* BF_ASM_TOFINO_PHV_H_ */
