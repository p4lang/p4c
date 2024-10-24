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
 *  Detects multiple pipelines in a program
 */
#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_DETECT_MULTIPLE_PIPELINES_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_DETECT_MULTIPLE_PIPELINES_H_

#include "ir/ir.h"

namespace BFN {

// This is the visitor class for detecting multiple pipelines
class DetectMultiplePipelines : public Inspector {
    unsigned nPipelines = 0;
    // Checks the "main"
    bool preorder(const IR::Declaration_Instance *) override;

 public:
    // Constructor that adds all of the passes
    DetectMultiplePipelines() {}
    // Returns true if multiple pipelines were found
    bool hasMultiplePipelines() { return (nPipelines > 1); }
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_DETECT_MULTIPLE_PIPELINES_H_
