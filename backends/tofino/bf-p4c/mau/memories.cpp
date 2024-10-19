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

#include "bf-p4c/mau/memories.h"

#include "bf-p4c/device.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/tofino/memories.h"
#include "lib/bitops.h"
#include "lib/range.h"

constexpr int Memories::SRAM_DEPTH;

int Memories::Use::rams_required() const {
    int rv = 0;
    for (auto r : row) {
        rv += r.col.size();
    }
    return rv;
}

/**
 * A function to determine whether or not the asm_output for a table needs to have a separate
 * search bus and result bus printed, rather than having just a single bus, as the result bus
 * a search bus have different values.
 */
bool Memories::Use::separate_search_and_result_bus() const {
    if (type != EXACT && type != ATCAM) return false;
    for (auto r : row) {
        if (!(r.result_bus == -1 || r.result_bus == r.bus)) return true;
    }
    return false;
}

std::ostream &operator<<(std::ostream &out, const Memories::Use::Way &w) {
    out << "size : " << w.size << ", select_mask: " << w.select_mask << std::endl;
    out << "\trams - ";
    for (auto r : w.rams) {
        out << "[ " << r.first << ", " << r.second << " ] ";
    }
    return out << std::endl;
}

Memories *Memories::create() { return new Tofino::Memories; }
