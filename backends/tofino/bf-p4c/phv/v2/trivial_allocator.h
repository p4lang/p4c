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

#ifndef BF_P4C_PHV_V2_TRIVIAL_ALLOCATOR_H_
#define BF_P4C_PHV_V2_TRIVIAL_ALLOCATOR_H_

#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_base.h"
#include "bf-p4c/phv/v2/kind_size_indexed_map.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

/// TrivialAllocator allocates PHV fields to an infinite long PHV. It is in two cases:
/// (1) trivial allocation before table placement.
/// (2) check whether all constraints can be satisfied.
class TrivialAllocator : public AllocatorBase {
 public:
    /// PhvStatus bookkeeper for containers.
    class PhvStatus {
        std::unordered_map<PHV::Size, int> next_container_idx;

     public:
        PhvStatus();
        PHV::Container next_container(PHV::Size) const;
        void inc_next_container(PHV::Size);
    };

    /// PartialAllocResult contains the allocation result and updated phv status if we slice
    /// and allocate a super cluster.
    struct PartialAllocResult {
        const AllocError *err = nullptr;
        std::vector<AllocSlice> alloc_slices;
        PhvStatus phv_status;
        bool ok() const { return err == nullptr; };
        PartialAllocResult(const std::vector<AllocSlice> &alloc_slices, PhvStatus phv_status)
            : alloc_slices(alloc_slices), phv_status(phv_status) {}
        explicit PartialAllocResult(const AllocError *err) : err(err) {}
    };

 private:
    // const PhvKit& kit_i;
    PhvInfo &phv_i;
    const int pipe_id_i;  /// used for logging purposes

    /// @returns a list of container groups that same-size groups are merged into one group.
    ContainerGroupsBySize make_container_groups_merged_by_size() const;

    /// gen_alloc_slices_from_tx extract allocation results from tx and generate allocation of
    /// fieldslices to *new* phv containers. New phv containers are requested from @p phv_status
    /// and phv_status will be updated.
    std::vector<PHV::AllocSlice> gen_alloc_slices_from_tx(const PHV::Transaction &tx,
                                                          PhvStatus &phv_status) const;

    /// gen_alloc_slices_from_tx extract allocation results from tx and generate allocation of
    /// fieldslices to *new* phv containers. New phv containers are requested from @p phv_status
    /// and phv_status will be updated.
    void bind_alloc_slices(const std::vector<PHV::AllocSlice> &slices);

    /// diagnose_invalid_cluster returns an AllocError that contains detailed error message of
    /// why we cannot allocate @p sc.
    const AllocError *diagnose_invalid_cluster(const Allocation &empty_alloc,
                                               const PHV::SuperCluster *sc,
                                               const ContainerGroupsBySize &container_groups,
                                               AllocatorMetrics &alloc_metrics) const;

    /// @returns user-friendly error msg.
    cstring make_error_msg(const SuperCluster *sc, const AllocError *err) const;

 public:
    TrivialAllocator(const PhvKit &kit, PhvInfo &phv, int pipe_id);

    /// @returns a PartialAllocResult that contains alloc_slices of an updated phv status when
    /// allocation succeeded. If allocation failed, PartialResult of an error that contains
    /// the best effort diagnose result will be returned.
    const PartialAllocResult *slice_and_allocate_sc(
        const Allocation &empty_alloc, const PHV::SuperCluster *sc, PhvStatus phv_status,
        const ContainerGroupsBySize &container_groups, AllocatorMetrics &alloc_metrics,
        bool homogeneous_sizes = false, bool minimal_packing_slicing = true,
        const int max_slicings = 128, std::ostream *history = nullptr) const;

    /// run trivial PHV allocator to allocate all @p clusters and update phv_i.
    bool allocate(const std::list<PHV::SuperCluster *> &clusters, AllocatorMetrics &alloc_metrics);

    /// @returns true if @p sc can be allocated to @p empty_alloc, assuming there are infinite
    /// containers. Use this verify whether there is any unsat constraint in @p sc.
    bool can_be_allocated(const Allocation &empty_alloc, const PHV::SuperCluster *sc,
                          AllocatorMetrics &alloc_metrics, const int max_slicings = 128) const;

    /// result of pre-slicing will always have a set of sliced super cluster. If any sliced
    /// super cluster failed to pass can_be_allocated verification, the first one will be saved
    /// to invalid.
    struct PreSlicingResult {
        std::list<SuperCluster *> sliced;
        const SuperCluster *invalid = nullptr;
        ordered_map<SuperCluster *, KindSizeIndexedMap> baseline_cont_req;
    };

    /// @returns result of sliced cluster from @p sc with the minimal number of slicing
    /// (try to maximize packing). This function will try to ensure that all the returned super
    /// clusters can be allocated, without violating any constraints, by calling can_be_allocated
    /// to verify before return. However, there can by cases that either we are not lucky enough
    /// or there are conflicting constraints, the @p sc cannot be split to allocatable clusters.
    /// In those cases, we will just return the last slicing we found.
    PreSlicingResult pre_slice(const Allocation &empty_alloc, SuperCluster *sc,
                               AllocatorMetrics &alloc_metrics, const int n_max_slicing = 128,
                               bool baseline_mode = false) const;
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_TRIVIAL_ALLOCATOR_H_ */
