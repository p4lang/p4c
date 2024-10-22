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

#ifndef BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_
#define BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_

#include <map>
#include <optional>

#include "bf-p4c/phv/phv_fields.h"

namespace PHV {
namespace v2 {

struct KindSizeIndexedMap {
    std::map<std::pair<PHV::Kind, PHV::Size>, int> m;
    int &operator[](const std::pair<PHV::Kind, PHV::Size> &ks) { return m[ks]; }
    int sum(const PHV::Kind &k) const;
    int sum(const PHV::Size &k) const;
    std::optional<int> get(const PHV::Kind &k, const PHV::Size &s) const;
    int get_or(const PHV::Kind &k, const PHV::Size &s, int default_val) const;
};
std::ostream &operator<<(std::ostream &, const KindSizeIndexedMap &);

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_ */
