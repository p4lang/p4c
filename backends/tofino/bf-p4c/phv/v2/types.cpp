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

#include "bf-p4c/phv/v2/types.h"

namespace PHV {
namespace v2 {

std::ostream &operator<<(std::ostream &out, const FieldSliceAllocStartMap &fs) {
    std::string sep = "";
    for (const auto &kv : fs) {
        const auto &index = kv.second;
        const auto &fs = kv.first;
        out << sep << index << ":" << fs.field()->name << "[" << fs.range().lo << ":"
            << fs.range().hi << "]";
        sep = ", ";
    }
    return out;
}

}  // namespace v2
}  // namespace PHV
