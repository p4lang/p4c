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
