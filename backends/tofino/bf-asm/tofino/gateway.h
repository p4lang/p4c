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

#ifndef BACKENDS_TOFINO_BF_ASM_TOFINO_GATEWAY_H_
#define BACKENDS_TOFINO_BF_ASM_TOFINO_GATEWAY_H_

#include "backends/tofino/bf-asm/tables.h"

class Target::Tofino::GatewayTable : public ::GatewayTable {
    friend class ::GatewayTable;
    GatewayTable(int line, const char *n, gress_t gr, Stage *s, int lid)
        : ::GatewayTable(line, n, gr, s, lid) {}

    void pass1() override;
    void pass2() override;
    void pass3() override;

    bool check_match_key(MatchKey &, const std::vector<MatchKey> &, bool) override;
    int gw_memory_unit() const override { return layout[0].row * 2 + gw_unit; }
    REGSETS_IN_CLASS(Tofino, TARGET_OVERLOAD, void write_next_table_regs, (mau_regs &), override;)
};

template <class REGS>
void enable_gateway_payload_exact_shift_ovr(REGS &regs, int bus);
template <>
void enable_gateway_payload_exact_shift_ovr(Target::Tofino::mau_regs &regs, int bus);

#endif /* BACKENDS_TOFINO_BF_ASM_TOFINO_GATEWAY_H_ */
