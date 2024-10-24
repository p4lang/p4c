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
