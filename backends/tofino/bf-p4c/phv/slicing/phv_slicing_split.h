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

#ifndef BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_
#define BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_

#include <optional>
#include <utility>

#include "bf-p4c/phv/utils/utils.h"
#include "lib/bitvec.h"
#include "lib/ordered_map.h"

/** PHV Slicing Split functions
 * Basic functions to split a supercluster.
 */

namespace PHV {
namespace Slicing {

// Helper type
using ListClusterPair = std::pair<SuperCluster::SliceList *, const RotationalCluster *>;
using SplitSchema = ordered_map<SuperCluster::SliceList *, bitvec>;

/// Split a SuperCluster with slice lists according to @split_schema. Apply this function
/// on SuperCluster with slice_lists only. The @p schema maps slice lists to the split point.
std::optional<std::list<SuperCluster *>> split(const SuperCluster *sc, const SplitSchema &schema);

/// Split the RotationalCluster in a SuperCluster without a slice list
/// according to @split_schema. Because there is no slice lists in the super cluster
/// the @p split_schema is a bitvec that a `1` on n-th bit means to split before the
/// the n-th bit.
std::optional<std::list<PHV::SuperCluster *>> split_rotational_cluster(const PHV::SuperCluster *sc,
                                                                       bitvec split_schema,
                                                                       int max_aligment = 0);

}  // namespace Slicing
}  // namespace PHV

std::ostream &operator<<(std::ostream &out, const PHV::Slicing::ListClusterPair &pair);
std::ostream &operator<<(std::ostream &out, const PHV::Slicing::ListClusterPair *pair);
std::ostream &operator<<(std::ostream &out, const PHV::Slicing::SplitSchema &schema);

#endif /* BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_ */
