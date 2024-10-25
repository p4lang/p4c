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

#include "bf-p4c/phv/v2/kind_size_indexed_map.h"

namespace PHV {
namespace v2 {

int KindSizeIndexedMap::sum(const PHV::Kind &k) const {
    int rv = 0;
    for (const auto &kv : m) {
        if (kv.first.first == k) {
            rv += kv.second;
        }
    }
    return rv;
}

int KindSizeIndexedMap::sum(const PHV::Size &s) const {
    int rv = 0;
    for (const auto &kv : m) {
        if (kv.first.second == s) {
            rv += kv.second;
        }
    }
    return rv;
}

std::optional<int> KindSizeIndexedMap::get(const PHV::Kind &k, const PHV::Size &s) const {
    if (m.count({k, s})) return m.at({k, s});
    return std::nullopt;
}

int KindSizeIndexedMap::get_or(const PHV::Kind &k, const PHV::Size &s, int default_v) const {
    if (auto rv = get(k, s)) return *rv;
    return default_v;
}

}  // namespace v2
}  // namespace PHV
