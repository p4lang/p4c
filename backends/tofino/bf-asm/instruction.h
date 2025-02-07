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

#ifndef BF_ASM_INSTRUCTION_H_
#define BF_ASM_INSTRUCTION_H_

#include <config.h>

#include <functional>

#include "tables.h"

struct Instruction : public IHasDbPrint {
    int lineno;
    int slot;
    explicit Instruction(int l) : lineno(l), slot(-1) {}
    virtual ~Instruction() {}
    virtual Instruction *pass1(Table *, Table::Actions::Action *) = 0;
    virtual std::string name() = 0;
    virtual void pass2(Table *, Table::Actions::Action *) = 0;
    virtual void dbprint(std::ostream &) const = 0;
    virtual bool equiv(Instruction *a) = 0;
    bool equiv(const std::unique_ptr<Instruction> &a) { return equiv(a.get()); }
    virtual bool salu_output() const { return false; }
    virtual bool salu_alu() const { return false; }
    virtual bool phvRead(std::function<void(const Phv::Slice &sl)>) = 0;
    bool phvRead() {
        return phvRead([](const Phv::Slice &sl) {});
    }
#define VIRTUAL_TARGET_METHODS(TARGET) \
    virtual void write_regs(Target::TARGET::mau_regs &, Table *, Table::Actions::Action *) = 0;
    FOR_ALL_REGISTER_SETS(VIRTUAL_TARGET_METHODS)
#undef VIRTUAL_TARGET_METHODS
#define DECLARE_FORWARD_VIRTUAL_INSTRUCTION_WRITE_REGS(TARGET)                               \
    void write_regs(Target::TARGET::mau_regs &regs, Table *tbl, Table::Actions::Action *act) \
        override;
    static Instruction *decode(Table *, const Table::Actions::Action *, const VECTOR(value_t) &);

    enum instruction_set_t { VLIW_ALU = 0, STATEFUL_ALU = 1, NUM_SETS = 2 };
    struct Decode {
        static std::multimap<std::string, Decode *> opcode[NUM_SETS];
        bool type_suffix;
        unsigned targets;
        explicit Decode(const char *name, int set = VLIW_ALU, bool ts = false);
        Decode(const char *name, target_t target, int set = VLIW_ALU, bool ts = false);
        Decode(const char *name, std::set<target_t> target, int set = VLIW_ALU, bool ts = false);
        const Decode &alias(const char *name, int set = VLIW_ALU, bool ts = false) {
            opcode[set].emplace(name, this);
            return *this;
        }
        virtual Instruction *decode(Table *tbl, const Table::Actions::Action *act,
                                    const VECTOR(value_t) & op) const = 0;
    };
};

namespace VLIW {
std::unique_ptr<Instruction> genNoopFill(Table *tbl, Table::Actions::Action *act, const char *op,
                                         int slot);
}

#endif /* BF_ASM_INSTRUCTION_H_ */
