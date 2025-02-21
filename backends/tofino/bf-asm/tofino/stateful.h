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

#ifndef BACKENDS_TOFINO_BF_ASM_TOFINO_STATEFUL_H_
#define BACKENDS_TOFINO_BF_ASM_TOFINO_STATEFUL_H_

#include "backends/tofino/bf-asm/tables.h"
#include "backends/tofino/bf-asm/target.h"

class Target::Tofino::StatefulTable : public ::StatefulTable {
    friend class ::StatefulTable;
    StatefulTable(int line, const char *n, gress_t gr, Stage *s, int lid)
        : ::StatefulTable(line, n, gr, s, lid) {}
};

template <>
void StatefulTable::write_logging_regs(Target::Tofino::mau_regs &regs);

#endif /* BACKENDS_TOFINO_BF_ASM_TOFINO_STATEFUL_H_ */
