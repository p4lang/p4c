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

#ifndef BF_P4C_ARCH_REWRITE_ACTION_SELECTOR_H_
#define BF_P4C_ARCH_REWRITE_ACTION_SELECTOR_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace BFN {

/**
 * Rewrite legacy use case of TNA ActionSelector
 *
 * ActionSelector(size, hash, mode); to
 *
 * ActionProfile(size) ap;
 * ActionSelector(ap, hash, mode, group_size, num_groups);
 *
 * This class is only required to migrate to existing code to the new
 * ActionSelector constructor.
 */
class RewriteActionSelector : public Transform {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    RewriteActionSelector(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}

    const IR::Node *postorder(IR::Declaration_Instance *inst) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *cce) override;
    const IR::Node *postorder(IR::ExpressionValue *ev) override;
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_REWRITE_ACTION_SELECTOR_H_ */
