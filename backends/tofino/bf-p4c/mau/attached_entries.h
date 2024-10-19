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

#ifndef BF_P4C_MAU_ATTACHED_ENTRIES_H_
#define BF_P4C_MAU_ATTACHED_ENTRIES_H_

#include <ostream>

#include "lib/ordered_map.h"

namespace P4 {
namespace IR {
namespace MAU {
class AttachedMemory;  // forward declaration
}
}  // namespace IR
}  // namespace P4

using namespace P4;

// Table Placement needs to communicate infomation about attached tables (how many entries
// are being placed in the current stage and whether more will be in a later stage) to Memory
// allocation, IXBar allocation, and table layout.  It does so via an attached_entries_t
// map, which needs to be declared before anything related to any of the above can be
// declared, so there's no good place to do it.  So we declare it here as a stand-alone
// header that can be included before anything else

struct attached_entries_element_t {
    int entries;
    bool need_more = false;   // need more entries in a later stage
    bool first_stage = true;  // no entries are in any earlier stage
    attached_entries_element_t() = delete;
    explicit attached_entries_element_t(int e) : entries(e) {}
};

typedef ordered_map<const IR::MAU::AttachedMemory *, attached_entries_element_t> attached_entries_t;

// not a consistent ordering -- true if first has more of anything than second
bool operator>(const attached_entries_t &, const attached_entries_t &);

std::ostream &operator<<(std::ostream &, const attached_entries_t &);
std::ostream &operator<<(std::ostream &, const attached_entries_element_t &);

#endif /* BF_P4C_MAU_ATTACHED_ENTRIES_H_ */
