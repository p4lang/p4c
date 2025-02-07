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

#ifndef BACKENDS_TOFINO_BF_ASM_TOP_LEVEL_H_
#define BACKENDS_TOFINO_BF_ASM_TOP_LEVEL_H_

#include "backends/tofino/bf-asm/json.h"
#include "backends/tofino/bf-asm/target.h"

template <class REGSET>
class TopLevelRegs;

class TopLevel {
 protected:
    TopLevel();

 public:
    static TopLevel *all;
    virtual ~TopLevel();
    virtual void output(json::map &) = 0;
    static void output_all(json::map &ctxtJson) { all->output(ctxtJson); }
    template <class T>
    static TopLevelRegs<typename T::register_type> *regs();
#define SET_MAU_STAGE(TARGET)                                                         \
    virtual void set_mau_stage(int, const char *, Target::TARGET::mau_regs *, bool) { \
        BUG_CHECK(!"register mismatch");                                              \
    }
    FOR_ALL_REGISTER_SETS(SET_MAU_STAGE)
};

template <class REGSET>
class TopLevelRegs : public TopLevel, public REGSET::top_level_regs {
 public:
    TopLevelRegs();
    ~TopLevelRegs();

    void output(json::map &);
    void set_mau_stage(int stage, const char *file, typename REGSET::mau_regs *regs,
                       bool egress_only);
};

template <class T>
TopLevelRegs<typename T::register_type> *TopLevel::regs() {
    return dynamic_cast<TopLevelRegs<typename T::register_type> *>(all);
}

#endif /* BACKENDS_TOFINO_BF_ASM_TOP_LEVEL_H_ */
