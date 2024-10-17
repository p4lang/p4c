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

#ifndef BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_
#define BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_

#include <map>
#include <optional>

#include "bf-p4c/phv/phv_fields.h"

namespace PHV {
namespace v2 {

struct KindSizeIndexedMap {
    std::map<std::pair<PHV::Kind, PHV::Size>, int> m;
    int& operator[](const std::pair<PHV::Kind, PHV::Size>& ks) { return m[ks]; }
    int sum(const PHV::Kind& k) const;
    int sum(const PHV::Size& k) const;
    std::optional<int> get(const PHV::Kind& k, const PHV::Size& s) const;
    int get_or(const PHV::Kind& k, const PHV::Size& s, int default_val) const;
};
std::ostream& operator<<(std::ostream&, const KindSizeIndexedMap&);

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_KIND_SIZE_INDEXED_MAP_H_ */
