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

#ifndef BACKENDS_TOFINO_BF_ASM_TOFINO_COUNTER_H_
#define BACKENDS_TOFINO_BF_ASM_TOFINO_COUNTER_H_

#include "backends/tofino/bf-asm/tables.h"

class Target::Tofino::CounterTable : public ::CounterTable {
    friend class ::CounterTable;
    CounterTable(int line, const char *n, gress_t gr, Stage *s, int lid)
        : ::CounterTable(line, n, gr, s, lid) {}
};

template <>
void CounterTable::setup_teop_regs(Target::Tofino::mau_regs &, int) {
    BUG();  // no teop on tofino
}

template <>
void CounterTable::write_alu_vpn_range(Target::Tofino::mau_regs &) {
    BUG();  // not available on tofino
}

#endif /* BACKENDS_TOFINO_BF_ASM_TOFINO_COUNTER_H_ */
