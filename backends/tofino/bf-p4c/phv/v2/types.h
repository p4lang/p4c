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

#ifndef BF_P4C_PHV_V2_TYPES_H_
#define BF_P4C_PHV_V2_TYPES_H_

#include <functional>

#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_metrics.h"

namespace PHV {
namespace v2 {

/// a function type that true if phv allocation is possible for the cluster.
using AllocVerifier = std::function<bool(const SuperCluster *, AllocatorMetrics &)>;

/// @returns true if two field slices cannot be packed into one container.
using HasPackConflict = std::function<bool(const PHV::FieldSlice &fs1, const PHV::FieldSlice &fs2)>;

/// map field slices to their starting position in a container.
using FieldSliceStart = std::pair<PHV::FieldSlice, int>;
using FieldSliceAllocStartMap = ordered_map<PHV::FieldSlice, int>;
std::ostream &operator<<(std::ostream &out, const FieldSliceAllocStartMap &fs);

/// type of container groups grouped by sizes.
using ContainerGroupsBySize = std::map<PHV::Size, std::vector<ContainerGroup>>;

/// Map container to container status.
using TxContStatus = ordered_map<PHV::Container, Transaction::ContainerStatus>;

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_TYPES_H_ */
