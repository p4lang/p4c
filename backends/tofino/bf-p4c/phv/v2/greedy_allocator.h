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

#ifndef BF_P4C_PHV_V2_GREEDY_ALLOCATOR_H_
#define BF_P4C_PHV_V2_GREEDY_ALLOCATOR_H_

#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_base.h"
#include "bf-p4c/phv/v2/kind_size_indexed_map.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

class GreedyAllocator : public AllocatorBase {
    // const PhvKit& kit_i;
    PhvInfo &phv_i;
    /// current pipe, used for logging.
    const int pipe_id_i;
    /// the total number of slicing that we will try for a super cluster.
    const int max_slicing_tries_i = 256;

 private:
    /// @returns a map from PHV::Size to container groups.
    ContainerGroupsBySize make_container_groups_by_size() const;

    struct PreSliceResult {
        std::list<SuperCluster *> clusters;
        ordered_map<const SuperCluster *, KindSizeIndexedMap> baseline_cont_req;
    };

    /// @returns pre-sliced super clusters, that was sliced with minimum splits.
    /// If there are any clusters that cannot pass the allocation test, they will be
    /// saved into invalid_clusters.
    PreSliceResult pre_slice_all(const Allocation &empty_alloc,
                                 const std::list<SuperCluster *> &clusters,
                                 ordered_set<const SuperCluster *> &invalid_clusters,
                                 AllocatorMetrics &alloc_metrics) const;

    /// RefinedSuperClusterSet classifies clusters to
    /// (1) normal pre-sliced and sorted cluster
    /// (2) deparser_zero optimization-targeting clusters.
    /// (3) strided cluster.
    struct RefinedSuperClusterSet {
        std::list<PHV::SuperCluster *> normal;
        std::list<PHV::SuperCluster *> deparser_zero;
        std::list<PHV::SuperCluster *> strided;

        /// @returns normal clusters in a deque with its original index.
        std::deque<std::pair<int, const PHV::SuperCluster *>> normal_sc_que() const;
    };

    /// sort normal (not deparser-zero, nor strided) clusters based on our heuristics.
    void sort_normal_clusters(std::list<PHV::SuperCluster *> &clusters) const;

    /// @returns a set of super clusters that has been classified by the way they will be
    /// allocated.
    RefinedSuperClusterSet prepare_refined_set(const std::list<SuperCluster *> &clusters) const;

    /// AllocResult of the slice_and_allocate_sc function with details including:
    /// (1) how did it slice the cluster.
    /// (2) score.
    /// (2) transaction of each sliced cluster.
    struct AllocResultWithSlicingDetails {
        AllocResult rst;
        TxScore *best_score = nullptr;
        std::list<PHV::SuperCluster *> best_slicing;
        int best_slicing_idx = 0;  // valid index starts from 1.
        /// NOTE: clot_allocated and deparser_zero_candidate sliced sc will not have transaction.
        ordered_map<const PHV::SuperCluster *, TxContStatus> *sliced_tx = nullptr;
        explicit AllocResultWithSlicingDetails(const Transaction &tx) : rst(tx) {}
        explicit AllocResultWithSlicingDetails(AllocError *err) : rst(err) {}
    };
    friend std::ostream &operator<<(std::ostream &, const AllocResultWithSlicingDetails &);

    /// Try slicing and allocate @p sc.
    AllocResultWithSlicingDetails slice_and_allocate_sc(
        const ScoreContext &ctx, const Allocation &alloc, const PHV::SuperCluster *sc,
        const ContainerGroupsBySize &container_groups, AllocatorMetrics &alloc_metrics,
        const int max_slicings = 128) const;

 public:
    GreedyAllocator(const PhvKit &kit, PhvInfo &phv, int pipe_id)
        : AllocatorBase(kit), phv_i(phv), pipe_id_i(pipe_id){};

    /// @returns false if allocation failed.
    /// allocate all @p clusters to phv_i. This function will directly print out errors
    /// when allocation failed.
    bool allocate(std::list<SuperCluster *> clusters, AllocatorMetrics &alloc_metrics);
};

std::ostream &operator<<(std::ostream &, const GreedyAllocator::AllocResultWithSlicingDetails &);

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_GREEDY_ALLOCATOR_H_ */
