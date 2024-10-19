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

#ifndef BF_P4C_MIDEND_H_
#define BF_P4C_MIDEND_H_

#include "bf-p4c/bf-p4c-options.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

struct CollectSourceInfoLogging;

namespace BFN {

class MidEnd : public PassManager {
 public:
    // These will be accurate when the mid-end completes evaluation
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    IR::ToplevelBlock *toplevel = nullptr;  // Should this be const?
    CollectSourceInfoLogging *sourceInfoLogging = nullptr;

    explicit MidEnd(BFN_Options &options);
};

bool skipRegisterActionOutput(const Visitor::Context *ctxt, const IR::Expression *);

}  // namespace BFN

#endif /* BF_P4C_MIDEND_H_ */
