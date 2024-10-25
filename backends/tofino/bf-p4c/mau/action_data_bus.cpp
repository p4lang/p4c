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

#include "action_data_bus.h"

#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/tofino/action_data_bus.h"

void ActionDataBus::clear() {
    allocated_attached.clear();
    reduction_or_mapping.clear();
}

/** The update procedure for all previously allocated tables in a stage.  This fills in the
 *  bitvecs correctly in order to be tested against in the allocation of the current table.
 */
void ActionDataBus::update(cstring name, const Use &alloc) {
    for (auto &rs : alloc.action_data_locs) {
        update(name, rs);
    }

    for (auto &rs : alloc.clobber_locs) {
        update(name, rs);
    }
}

void ActionDataBus::update_action_data(cstring name, const TableResourceAlloc *alloc,
                                       const IR::MAU::Table *tbl) {
    const IR::MAU::ActionData *ad = nullptr;
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        ad = at->to<IR::MAU::ActionData>();
        if (ad != nullptr) break;
    }
    if (ad) {
        if (allocated_attached.count(ad) > 0) return;
    }

    if (alloc->action_data_xbar) {
        update(name, *alloc->action_data_xbar);
        if (ad) allocated_attached.emplace(ad, *alloc->action_data_xbar);
    }
}

void ActionDataBus::update_meter(cstring name, const TableResourceAlloc *alloc,
                                 const IR::MAU::Table *tbl) {
    const IR::MAU::AttachedMemory *am = nullptr;
    const IR::MAU::StatefulAlu *salu = nullptr;
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        auto mtr = at->to<IR::MAU::Meter>();
        salu = at->to<IR::MAU::StatefulAlu>();
        if ((mtr && mtr->alu_output()) || salu) {
            am = at;
            break;
        }
    }

    if (am) {
        if (allocated_attached.count(am) > 0) return;
    } else {
        return;
    }

    if (salu && salu->reduction_or_group && reduction_or_mapping.count(salu->reduction_or_group))
        return;

    // Only update if the meter/stateful alu use is not previously accounted for
    if (alloc->meter_xbar) {
        update(name + "$" + am->name, *alloc->meter_xbar);
        if (salu && salu->reduction_or_group) {
            reduction_or_mapping.emplace(salu->reduction_or_group, *alloc->meter_xbar);
        }
        allocated_attached.emplace(am, *alloc->meter_xbar);
    }
}

void ActionDataBus::update(cstring name, const TableResourceAlloc *alloc,
                           const IR::MAU::Table *tbl) {
    update_action_data(name, alloc, tbl);
    update_meter(name, alloc, tbl);
}

void ActionDataBus::update(const IR::MAU::Table *tbl) { update(tbl->name, tbl->resources, tbl); }

ActionDataBus *ActionDataBus::create() { return new Tofino::ActionDataBus; }

std::ostream &operator<<(std::ostream &out, const ActionDataBus::Use &u) {
    out << "[" << "action_data_locs size: " << u.action_data_locs.size()
        << " clobber_locs size: " << u.clobber_locs.size() << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionDataBus &adb) {
    out << "[" << "allocated_attached size: " << adb.allocated_attached.size()
        << " reduction_or_mapping: " << adb.reduction_or_mapping.size() << "]";
    return out;
}

int ActionDataBus::getAdbSize() { return Tofino::ActionDataBus::ADB_BYTES; }
