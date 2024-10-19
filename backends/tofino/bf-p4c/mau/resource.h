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

#ifndef BF_P4C_MAU_RESOURCE_H_
#define BF_P4C_MAU_RESOURCE_H_

/* clang-format off */

#include <map>

#include "bf-p4c/lib/autoclone.h"
#include "bf-p4c/mau/action_data_bus.h"
#include "bf-p4c/mau/instruction_memory.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "ir/ir.h"
#include "lib/safe_vector.h"

using namespace P4;

struct TableResourceAlloc {
    // TODO: Currently we only have a std::map for the UniqueId objects for Memories.  This would
    // make sense to eventually move to IXBar::Use, and even potentially
    // ActionFormat::Use/ActionDataBus::Use for the different types of allocations
    autoclone_ptr<IXBar::Use> match_ixbar, gateway_ixbar, proxy_hash_ixbar, action_ixbar,
        selector_ixbar, salu_ixbar, meter_ixbar;
    TableFormat::Use table_format;
    std::map<UniqueId, Memories::Use> memuse;
    ActionData::Format::Use action_format;
    MeterALU::Format::Use meter_format;
    autoclone_ptr<ActionDataBus::Use> action_data_xbar, meter_xbar;
    InstructionMemory::Use instr_mem;
    LayoutOption layout_option;

    // only relevant to tofino 1/2
    safe_vector<Tofino::IXBar::HashDistUse> hash_dists;

    TableResourceAlloc *clone() const { return new TableResourceAlloc(*this); }
    TableResourceAlloc *rename(const IR::MAU::Table *tbl, int stage_table = -1,
                               int logical_table = -1);

    void clear_ixbar() {
        match_ixbar.reset();
        gateway_ixbar.reset();
        proxy_hash_ixbar.reset();
        selector_ixbar.reset();
        salu_ixbar.reset();
        meter_ixbar.reset();
        hash_dists.clear();
    }
    void clear() {
        clear_ixbar();
        table_format.clear();
        memuse.clear();
        action_format.clear();
        action_data_xbar.reset();
        instr_mem.clear();
        meter_format.clear();
        meter_xbar.reset();
    }
    void toJSON(P4::JSONGenerator &json) const { json << "null"; }
    static TableResourceAlloc *fromJSON(P4::JSONLoader &) { return nullptr; }

    void merge_instr(const TableResourceAlloc *);
    bool has_tind() const;
    safe_vector<int> hash_dist_immed_units() const;
    int rng_unit() const;
    int findBytesOnIxbar(const PHV::FieldSlice &) const;

    ::IXBar::Use *find_ixbar(IXBar::Use::type_t type) const;
};

std::ostream &operator<<(std::ostream &, const TableResourceAlloc &);

/* clang-format on */

#endif /* BF_P4C_MAU_RESOURCE_H_ */
