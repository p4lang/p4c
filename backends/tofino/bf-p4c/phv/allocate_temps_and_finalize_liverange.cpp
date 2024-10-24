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

#include "bf-p4c/phv/allocate_temps_and_finalize_liverange.h"

#include <sstream>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/mau/table_placement.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/finalize_physical_liverange.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/utils/slice_alloc.h"

namespace PHV {

struct TempVarAllocResult {
    bool ok = true;
    std::stringstream err;
};

struct MiniAlloc {
    ordered_map<Container, ordered_set<AllocSlice>> cont_slices;
    ordered_map<Container, gress_t> cont_gress;

    /// add a new alloc slice.
    void add_slice(const AllocSlice &slice) {
        const auto &c = slice.container();
        cont_slices[c].insert(slice);
        BUG_CHECK(!cont_gress.count(c) || cont_gress.at(c) == slice.field()->gress,
                  "container %1% has AllocSlices of different gress", c);
        cont_gress[c] = slice.field()->gress;
    }

    /// @returns true if @p c does not have any allocated slice within @p lr.
    bool is_empty_at(const Container &c, const LiveRange &lr) const {
        if (!cont_slices.count(c)) return true;
        for (const auto &slice : cont_slices.at(c)) {
            if (!lr.is_disjoint({slice.getEarliestLiveness(), slice.getLatestLiveness()})) {
                return false;
            }
        }
        return true;
    }

    /// @returns gress assignment for @p c.
    std::optional<gress_t> get_gress(const Container &c) const {
        const PhvSpec &phv_spec = Device::phvSpec();
        if (phv_spec.ingressOnly()[phv_spec.containerToId(c)]) {
            return INGRESS;
        } else if (phv_spec.egressOnly()[phv_spec.containerToId(c)]) {
            return EGRESS;
        }
        if (cont_gress.count(c)) {
            return cont_gress.at(c);
        }
        return std::nullopt;
    }

    /// @returns true if @p f of @p lr can be allocated to @p c.
    bool satisfies_constraints(const PHV::Field *f, const LiveRange &lr, const Container &c) const {
        if (c.type().kind() != PHV::Kind::normal) return false;
        if (int(c.size()) < f->size) return false;
        auto gress = get_gress(c);
        if (gress.has_value() && *gress != f->gress) return false;
        if (!is_empty_at(c, lr)) return false;
        return true;
    }
};

/// find empty container for @p temp_vars and allocate them.
TempVarAllocResult allocate_temp_vars(PhvInfo &phv,
                                      const ordered_map<const PHV::Field *, LiveRange> &temp_vars) {
    MiniAlloc alloc;
    for (const auto &f : phv) {
        for (const auto &slice : f.get_alloc()) {
            alloc.add_slice(slice);
        }
    }
    TempVarAllocResult rst;
    const PhvSpec &phv_spec = Device::phvSpec();
    const bitvec containers = phv_spec.physicalContainers();
    for (auto &kv : temp_vars) {
        LOG3("Try to alloc temp var: " << kv.first);
        Field *f = phv.field(kv.first->id);
        const LiveRange lr = kv.second;
        bool found = false;
        for (const auto &cont_id : containers) {
            Container cont = phv_spec.idToContainer(cont_id);
            if (alloc.satisfies_constraints(f, lr, cont)) {
                AllocSlice alloc_slice(f, cont, StartLen(0, f->size), StartLen(0, f->size));
                alloc_slice.setIsPhysicalStageBased(true);
                alloc_slice.setLiveness(lr.start, lr.end);
                alloc.add_slice(alloc_slice);
                f->add_alloc(alloc_slice);
                phv.add_container_to_field_entry(cont, f);
                LOG1("Allocated: " << alloc_slice);
                found = true;
                break;
            }
        }
        if (!found) {
            LOG1("Failed to find allocation for : " << f << " " << lr);
            rst.err << "cannot find allocation for " << f << " " << lr << "\n";
            rst.ok = false;
        }
    }
    return rst;
}

void UpdateDeparserStage::end_apply() {
    for (auto &f : phv_i) {
        for (auto &slice : f.get_alloc()) {
            if (slice.getLatestLiveness().first > PhvInfo::getDeparserStage()) {
                slice.setPhysicalDeparserStageExceeded(true);
            }
        }
    }
}

AllocateTempsAndFinalizeLiverange::AllocateTempsAndFinalizeLiverange(PhvInfo &phv,
                                                                     const ClotInfo &clot,
                                                                     const FieldDefUse &defuse,
                                                                     const ::TableSummary &ts)
    : phv_i(phv), clot_i(clot), defuse_i(defuse), ts_i(ts) {
    auto *tb_mutex = new TablesMutuallyExclusive();
    auto pragma_no_init = new PragmaNoInit(phv);
    auto *finalize_lr =
        new PHV::FinalizePhysicalLiverange(phv, clot, *tb_mutex, defuse, *pragma_no_init);
    const auto get_temp_vars = [finalize_lr]() {
        return finalize_lr->unallocated_temp_var_live_ranges();
    };
    addPasses({pragma_no_init, tb_mutex,
               new PassIf([this] { return ts_i.getActualState() == State::FAILURE; },
                          {new UpdateDeparserStage(phv_i)}),
               finalize_lr,
               new PassIf([get_temp_vars]() { return !get_temp_vars().empty(); },
                          {
                              new VisitFunctor([get_temp_vars, &phv]() {
                                  auto rst = allocate_temp_vars(phv, get_temp_vars());
                                  if (!rst.ok) {
                                      error("Failed to allocated temp vars: %1%", rst.err.str());
                                  }
                              }),
                              // TODO: even if temp var allocation does not introduce
                              // a container conflict, the table layout does not have the
                              // correct action data as this temp var was not allocated during last
                              // TP. So we need to redo table placement.
                              new VisitFunctor([]() {
                                  throw TablePlacement::FinalRerunTablePlacementTrigger(false);
                              }),
                          })});
}

}  // namespace PHV
