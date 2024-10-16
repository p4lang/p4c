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

#include "bf-p4c/phv/v2/types.h"

namespace PHV {
namespace v2 {

std::ostream& operator<<(std::ostream& out, const FieldSliceAllocStartMap& fs) {
    std::string sep = "";
    for (const auto& kv : fs) {
        const auto& index = kv.second;
        const auto& fs = kv.first;
        out << sep << index << ":" << fs.field()->name << "[" << fs.range().lo << ":"
            << fs.range().hi << "]";
        sep = ", ";
    }
    return out;
}


}  // namespace v2
}  // namespace PHV
