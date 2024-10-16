/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_
#define EXTENSIONS_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_

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

#endif /* EXTENSIONS_BF_P4C_IR_TOFINO_WRITE_CONTEXT_H_ */
