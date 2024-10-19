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

#include "bf-p4c/phv/v2/phv_allocation_v2.h"

#include "bf-p4c/phv/v2/allocator_base.h"
#include "bf-p4c/phv/v2/greedy_allocator.h"
#include "bf-p4c/phv/v2/smart_packing.h"
#include "bf-p4c/phv/v2/trivial_allocator.h"
#include "lib/error.h"

namespace PHV {
namespace v2 {

const IR::Node *PhvAllocation::apply_visitor(const IR::Node *root_, const char *) {
    Log::TempIndent indent;
    LOG1("Starting PHV V2 Allocation" << indent);
    BUG_CHECK(root_->is<IR::BFN::Pipe>(), "IR root is not a BFN::Pipe: %s", root_);
    const auto *root = root_->to<IR::BFN::Pipe>();
    pipe_id_i = root->canon_id();

    // clear allocation result to create an empty concrete allocation.
    PhvKit::clear_slices(phv_i);
    PHV::ConcreteAllocation alloc =
        ConcreteAllocation(phv_i, kit_i.uses, kit_i.settings.trivial_alloc);

    std::list<PHV::SuperCluster *> clusters = kit_i.make_superclusters();
    LOG2("Made " << clusters.size() << " superclusters");

    // remove singleton unreferenced fields
    clusters = PhvKit::remove_unref_clusters(kit_i.uses, clusters);
    clusters = PhvKit::remove_clot_allocated_clusters(kit_i.clot, clusters);
    clusters = PhvKit::remove_singleton_metadata_slicelist(clusters);
    LOG2("Removed unreferenced, clot allocated and singleton metadata clusters");

    const MauBacktracker *mau = kit_i.settings.physical_stage_trivial ? &kit_i.mau : nullptr;

    AllocatorMetrics trivial_alloc_metrics("TrivialAllocator"_cs);
    AllocatorMetrics greedy_alloc_metrics("GreedyAllocator"_cs);
    AllocatorMetrics ixbar_pack_metrics("IXbarPackingTrivialAllocator"_cs);

    // apply table-layout-friendly packing on super clusters.
    auto trivial_allocator = new PHV::v2::TrivialAllocator(kit_i, phv_i, pipe_id_i);
    const auto alloc_verifier = [&](const PHV::SuperCluster *sc, AllocatorMetrics &alloc_metrics) {
        return trivial_allocator->can_be_allocated(alloc.makeTransaction(), sc, alloc_metrics);
    };
    IxbarFriendlyPacking packing(phv_i, kit_i.tb_keys, kit_i.table_mutex, kit_i.defuse, kit_i.deps,
                                 kit_i.get_has_pack_conflict(), kit_i.parser_packing_validator,
                                 alloc_verifier, mau, ixbar_pack_metrics);
    LOG2("Packing " << clusters.size() << " clusters");
    clusters = packing.pack(clusters);
    // clusters = get_packed_cluster_group(clusters, kit_i.table_pack_opt, alloc_verifier, phv_i);

    if (kit_i.settings.trivial_alloc) {
        if (!trivial_allocator->allocate(clusters, trivial_alloc_metrics)) {
            LOG1("Trivial allocation failed.");
        }
    } else {
        GreedyAllocator greedy_allocator(kit_i, phv_i, pipe_id_i);
        if (!greedy_allocator.allocate(clusters, greedy_alloc_metrics)) {
            LOG1("Greedy allocation failed.");
        }
    }
    LOG1("Ending PHV V2 Allocation");
    return root;
}

}  // namespace v2
}  // namespace PHV
