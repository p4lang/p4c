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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

// Whilst the original PR consolidated code, moving it out of the back-end and
// canonicalising the IR sooner, it caused ripples that caused issue to the PHV allocator.
// For now, we only partially implement the PR by setting `ENABLE_P4C3251 0`.
#define ENABLE_P4C3251 0

namespace BFN {

class CopyHeaders : public PassRepeated {
 public:
    CopyHeaders(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, P4::TypeChecking *typeChecking);
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_COPY_HEADER_H_ */
