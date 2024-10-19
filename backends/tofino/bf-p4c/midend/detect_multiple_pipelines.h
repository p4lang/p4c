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
