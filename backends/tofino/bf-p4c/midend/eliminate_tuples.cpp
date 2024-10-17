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

#include "bf-p4c/midend/eliminate_tuples.h"

namespace BFN {

bool SaveHashListExpression::preorder(const IR::HashListExpression *hle) {
    auto *mce = findContext<IR::MethodCallExpression>();
    if (mce == nullptr)
        return true;
    // Method stays the same after TupleReplacement
    update_hashes[mce->method] = hle;
    return true;
}

const IR::Node* InsertHashStructExpression::preorder(IR::StructExpression *se) {
    auto *mce = findContext<IR::MethodCallExpression>();
    if (mce == nullptr)
        return se;
    if (!update_hashes->count(mce->method))
        return se;
    auto hle = (*update_hashes)[mce->method];
    auto hse = new IR::HashStructExpression(se->srcInfo,
                                            se->type,
                                            se->structType,
                                            se->components,
                                            hle->fieldListCalcName,
                                            hle->outputWidth);
    hse->fieldListNames = hle->fieldListNames;
    hse->algorithms = hle->algorithms;
    return hse;
}

}  // namespace BFN
