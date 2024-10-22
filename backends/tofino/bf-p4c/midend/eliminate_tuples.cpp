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

#include "bf-p4c/midend/eliminate_tuples.h"

namespace BFN {

bool SaveHashListExpression::preorder(const IR::HashListExpression *hle) {
    auto *mce = findContext<IR::MethodCallExpression>();
    if (mce == nullptr) return true;
    // Method stays the same after TupleReplacement
    update_hashes[mce->method] = hle;
    return true;
}

const IR::Node *InsertHashStructExpression::preorder(IR::StructExpression *se) {
    auto *mce = findContext<IR::MethodCallExpression>();
    if (mce == nullptr) return se;
    if (!update_hashes->count(mce->method)) return se;
    auto hle = (*update_hashes)[mce->method];
    auto hse = new IR::HashStructExpression(se->srcInfo, se->type, se->structType, se->components,
                                            hle->fieldListCalcName, hle->outputWidth);
    hse->fieldListNames = hle->fieldListNames;
    hse->algorithms = hle->algorithms;
    return hse;
}

}  // namespace BFN
