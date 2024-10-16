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

#ifndef BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_
#define BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_

#include <utility>

#include <optional>

#include "lib/bitvec.h"
#include "lib/ordered_map.h"

#include "bf-p4c/phv/utils/utils.h"

/** PHV Slicing Split functions
 * Basic functions to split a supercluster.
 */

namespace PHV {
namespace Slicing {

// Helper type
using ListClusterPair = std::pair<SuperCluster::SliceList*, const RotationalCluster*>;
using SplitSchema = ordered_map<SuperCluster::SliceList*, bitvec>;

/// Split a SuperCluster with slice lists according to @split_schema. Apply this function
/// on SuperCluster with slice_lists only. The @p schema maps slice lists to the split point.
std::optional<std::list<SuperCluster*>> split(const SuperCluster* sc, const SplitSchema& schema);

/// Split the RotationalCluster in a SuperCluster without a slice list
/// according to @split_schema. Because there is no slice lists in the super cluster
/// the @p split_schema is a bitvec that a `1` on n-th bit means to split before the
/// the n-th bit.
std::optional<std::list<PHV::SuperCluster*>> split_rotational_cluster(
    const PHV::SuperCluster* sc, bitvec split_schema, int max_aligment = 0);

}  // namespace Slicing
}  // namespace PHV

std::ostream& operator<<(std::ostream& out, const PHV::Slicing::ListClusterPair& pair);
std::ostream& operator<<(std::ostream& out, const PHV::Slicing::ListClusterPair* pair);
std::ostream& operator<<(std::ostream& out, const PHV::Slicing::SplitSchema& schema);

#endif /* BF_P4C_PHV_SLICING_PHV_SLICING_SPLIT_H_ */
