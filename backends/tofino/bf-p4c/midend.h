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
