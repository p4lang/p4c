/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_
#define BACKENDS_TOFINO_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_

#include "ir/ir.h"

class TofinoWriteContext : public P4WriteContext {
 public:
    /** Extend P4WriteContext::isWrite() with knowledge that the following are
     *  writes:
     *    - SaluAction.outtput_dst is a write
     *    - The first argument to an Instruction is a write
     *    - Extern args marked Out or InOut are writes
     */
    bool isWrite(bool root_value = false);

    /** Extend P4WriteContext::isRead() with knowledge that the following are
     *  reads:
     *    - ParserMatch statements
     *    - ParserState selects
     *    - Deparser emits
     *    - Any arguments to a Primitive after the first (which is written)
     *    - Extern args marked In or InOut
     */
    bool isRead(bool root_value = false);

    bool isIxbarRead(bool root_value = false);
};

#endif /* BACKENDS_TOFINO_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_ */
