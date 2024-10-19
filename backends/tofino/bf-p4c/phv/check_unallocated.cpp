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

#include "bf-p4c/phv/check_unallocated.h"

#include <initializer_list>

CheckForUnallocatedTemps::CheckForUnallocatedTemps(const PhvInfo &phv, PhvUse &uses,
                                                   const ClotInfo &clot,
                                                   PHV_AnalysisPass *phv_alloc)
    : phv(phv), uses(uses), clot(clot) {
    const bool &limit = phv_alloc->get_limit_tmp_creation();
    addPasses({&uses, new VisitFunctor([this]() { collect_unallocated_fields(); }),
               new PassIf([this]() { return !unallocated_fields.empty(); },
                          {phv_alloc->make_incremental_alloc_pass(unallocated_fields),
                           // TODO: even if temp var allocation does not introduce
                           // a container conflict, the table layout does not have the
                           // correct action data as this temp var was not allocated during last TP.
                           // So we need to redo table placement.
                           new VisitFunctor([&]() {
                               throw TablePlacement::FinalRerunTablePlacementTrigger(limit);
                           })})});
}

void CheckForUnallocatedTemps::collect_unallocated_fields() {
    unallocated_fields.clear();
    for (auto &field : phv) {
        if (!uses.is_referenced(&field)) continue;

        if (clot.fully_allocated(&field)) continue;

        // TODO: padding does not need phv allocation
        if (field.is_ignore_alloc()) continue;
        if (field.is_avoid_alloc()) continue;

        // If this field is an alias source, then we need to check the allocation of the alias
        // destination too. If that destination is allocated to CLOTs, we will not find an
        // allocation for that field here. So, we need a special check here.
        auto *aliasDest = phv.getAliasDestination(&field);
        if (aliasDest != nullptr)
            if (clot.fully_allocated(aliasDest)) continue;

        bitvec allocatedBits;
        field.foreach_alloc([&](const PHV::AllocSlice &slice) {
            bitvec sliceBits(slice.field_slice().lo, slice.width());
            allocatedBits |= sliceBits;
        });

        // Account for bits allocated to CLOTs, both for this field and for its alias (if any).
        for (auto f : {(const PHV::Field *)&field, aliasDest}) {
            if (!f) continue;
            for (auto entry : *clot.slice_clots(f)) {
                auto slice = entry.first;
                auto range = slice->range();
                bitvec sliceBits(range.lo, range.size());
                allocatedBits |= sliceBits;
            }
        }

        bitvec allBitsInField(0, field.size);
        if (allocatedBits != allBitsInField) unallocated_fields.insert(&field);
    }
    if (unallocated_fields.size() > 0 && LOGGING(3)) {
        LOG3("unallocted PHV fields after table placement: ");
        for (const auto &f : unallocated_fields) {
            LOG3("unalloated: " << f);
        }
    }
}
