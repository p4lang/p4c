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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_DEBUG_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_DEBUG_INFO_H_

#include <vector>

#include "lib/cstring.h"

using namespace P4;

namespace BFN {

/// A tracker for debugging information. This is used for informational purposes
/// only; the information can be included in generated assembly or log files,
/// but it has no effect on correctness.
struct DebugInfo {
    /// Merge the contents of another DebugInfo object into this one.
    void mergeWith(const DebugInfo &other) {
        info.insert(info.end(), other.info.begin(), other.info.end());
    }

    std::vector<cstring> info;
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_DEBUG_INFO_H_ */
