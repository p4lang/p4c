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

/**
 *  Detects program with multiple pipelines.
 */
#include "detect_multiple_pipelines.h"

namespace BFN {

// Checks the "main"
bool DetectMultiplePipelines::preorder(const IR::Declaration_Instance *di) {
    // Check if this is specialized type with baseType reffering to "Switch"
    if (!di->type->is<IR::Type_Specialized>()) return false;
    auto type = di->type->to<IR::Type_Specialized>();
    if (!type->baseType->is<IR::Type_Name>() ||
        type->baseType->to<IR::Type_Name>()->path->name != "Switch")
        return false;
    // Check the arguments
    auto arguments = di->arguments;
    for (unsigned idx = 0; idx < di->arguments->size(); idx++) {
        if (arguments->at(idx)->expression->is<IR::PathExpression>() &&
            arguments->at(idx)->expression->to<IR::PathExpression>()->type->is<IR::Type_Package>())
            nPipelines++;
    }
    return false;
}

}  // namespace BFN
