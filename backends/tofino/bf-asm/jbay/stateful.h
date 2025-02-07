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

#ifndef BF_ASM_JBAY_STATEFUL_H_
#define BF_ASM_JBAY_STATEFUL_H_

#include <tables.h>
#include <target.h>

#if HAVE_JBAY

// FIXME -- should be a namespace somwhere?  Or in class StatefulTable
/* for jbay counter mode, we may need both a push and a pop mode, as well as counter_function,
 * so we pack them all into an int with some shifts and masks */
enum {
    PUSHPOP_BITS = 5,
    PUSHPOP_MASK = 0xf,
    PUSHPOP_ANY = 0x10,
    PUSH_MASK = PUSHPOP_MASK,
    PUSH_ANY = PUSHPOP_ANY,
    POP_MASK = PUSHPOP_MASK << PUSHPOP_BITS,
    POP_ANY = PUSHPOP_ANY << PUSHPOP_BITS,
    PUSH_MISS = 1,
    PUSH_HIT = 2,
    PUSH_GATEWAY = 3,
    PUSH_ALL = 4,
    PUSH_GW_ENTRY = 5,
    POP_MISS = PUSH_MISS << PUSHPOP_BITS,
    POP_HIT = PUSH_HIT << PUSHPOP_BITS,
    POP_GATEWAY = PUSH_GATEWAY << PUSHPOP_BITS,
    POP_ALL = PUSH_ALL << PUSHPOP_BITS,
    POP_GW_ENTRY = PUSH_GW_ENTRY << PUSHPOP_BITS,
    FUNCTION_SHIFT = 2 * PUSHPOP_BITS,
    FUNCTION_LOG = 1 << FUNCTION_SHIFT,
    FUNCTION_FIFO = 2 << FUNCTION_SHIFT,
    FUNCTION_STACK = 3 << FUNCTION_SHIFT,
    FUNCTION_FAST_CLEAR = 4 << FUNCTION_SHIFT,
    FUNCTION_MASK = 0xf << FUNCTION_SHIFT,
};

int parse_jbay_counter_mode(const value_t &v);
template <>
void StatefulTable::write_logging_regs(Target::JBay::mau_regs &regs);

#endif /* HAVE_JBAY ||  ||  */
#endif /* BF_ASM_JBAY_STATEFUL_H_ */
