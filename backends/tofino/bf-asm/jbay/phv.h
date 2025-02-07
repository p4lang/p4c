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

#ifndef BF_ASM_JBAY_PHV_H_
#define BF_ASM_JBAY_PHV_H_

#include "../phv.h"

class Target::JBay::Phv : public Target::Phv {
    friend class ::Phv;
    struct Register : public ::Phv::Register {
        short parser_id_, deparser_id_;
        int parser_id() const override { return parser_id_; }
        int mau_id() const override { return uid < 280 ? uid : -1; }
        int ixbar_id() const override {
            static const int ixbar_permute[16] = {0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, -6, -6, 0, 0};
            return deparser_id_ + ixbar_permute[deparser_id_ & 0xf];
        }
        int deparser_id() const override { return deparser_id_; }
    };
    void init_regs(::Phv &phv) override;
    target_t type() const override { return JBAY; }
    unsigned mau_groupsize() const override { return 20; }
};

class Target::Tofino2H::Phv : public Target::JBay::Phv {
    target_t type() const override { return TOFINO2H; }
};

class Target::Tofino2M::Phv : public Target::JBay::Phv {
    target_t type() const override { return TOFINO2M; }
};

class Target::Tofino2U::Phv : public Target::JBay::Phv {
    target_t type() const override { return TOFINO2U; }
};

class Target::Tofino2A0::Phv : public Target::JBay::Phv {
    target_t type() const override { return TOFINO2A0; }
};

#endif /* BF_ASM_JBAY_PHV_H_ */
