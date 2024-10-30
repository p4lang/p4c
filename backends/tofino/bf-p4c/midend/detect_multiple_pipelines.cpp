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
