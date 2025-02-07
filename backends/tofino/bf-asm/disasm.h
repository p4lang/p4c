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

#ifndef DISASM_H_
#define DISASM_H_

#include "target.h"

class Disasm {
 public:
    FOR_ALL_TARGETS(DECLARE_TARGET_CLASS)
    virtual void input_binary(uint64_t addr, char tag, uint32_t *data, size_t len) = 0;
    static Disasm *create(std::string target);
};

#define DECLARE_DISASM_TARGET(TARGET, ...)        \
    class Disasm::TARGET : public Disasm {        \
     public:                                      \
        typedef ::Target::TARGET Target;          \
        Target::top_level_regs regs;              \
        TARGET() { declare_registers(&regs); }    \
        ~TARGET() { undeclare_registers(&regs); } \
        TARGET(const TARGET &) = delete;          \
        __VA_ARGS__                               \
    };

FOR_ALL_TARGETS(
    DECLARE_DISASM_TARGET, void input_binary(uint64_t addr, char tag, uint32_t *data, size_t len) {
        if (tag == 'D') {
            regs.mem_top.input_binary(addr, tag, data, len);
        } else {
            regs.reg_top.input_binary(addr, tag, data, len);
        }
    })

#endif /* DISASM_H_ */
