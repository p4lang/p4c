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

#ifndef BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_
#define BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_

#include <utility>
#include <vector>

#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

struct FieldPacking;

using ResubmitSources = IR::Vector<IR::Expression>;
using ResubmitExtracts = std::map<unsigned, std::pair<cstring, const ResubmitSources *>>;

class FixupResubmitMetadata : public PassManager {
    ResubmitExtracts fieldExtracts;

 public:
    FixupResubmitMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_FROMV1_0_RESUBMIT_H_ */
