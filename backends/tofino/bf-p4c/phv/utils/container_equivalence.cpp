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

#include "bf-p4c/phv/utils/container_equivalence.h"

namespace PHV {

ContainerEquivalenceTracker::ContainerEquivalenceTracker(const PHV::Allocation &alloc)
    : alloc(alloc), restrictW0(Device::currentDevice() == Device::JBAY) {}

std::optional<PHV::Container> ContainerEquivalenceTracker::find_equivalent_tried_container(
    PHV::Container c) {
    if (!restrictW0) {
        return find_single(c);
    } else {
        // W0 is special on Tofino 2
        if (c == PHV::Container({PHV::Kind::normal, PHV::Size::b32}, 0)) return std::nullopt;
        if (!c.is(PHV::Size::b8)) return find_single(c);

        // Parser write mode is shared for the continuous tuple of B containers on Tofino 2
        // that share all but last bit of identifier, therefore we can only safely consider
        // the current container c to be equivalent to some other empty container e if the
        // paired container of e is also empty (the paired container of c needs not be empty
        // as that only means that there are more constrains on c, not less).
        auto equiv = find_single(c);
        if (!equiv) return std::nullopt;
        unsigned idx = equiv->index() & 1u ? equiv->index() & ~1u : equiv->index() | 1u;
        PHV::Container pair_equiv(equiv->type(), idx);
        auto pair_stat = alloc.getStatus(pair_equiv);
        if (pair_stat && pair_stat->alloc_status == ContainerAllocStatus::EMPTY) return equiv;
        return std::nullopt;
    }
}

void ContainerEquivalenceTracker::invalidate(PHV::Container c) {
    equivalenceClasses.erase(get_class(c));
}

std::optional<PHV::Container> ContainerEquivalenceTracker::find_single(PHV::Container c) {
    if (wideArithLow || excluded.count(c)) return std::nullopt;

    auto cls = get_class(c);
    if (!cls.is_empty) return std::nullopt;

    auto ins = equivalenceClasses.emplace(cls, c);
    if (!ins.second) {  // not inserted, found
        LOG9("\tField equivalence class for "
             << c << " ~ " << ins.first->second << ": empty = " << cls.is_empty
             << " PHV::Kind = " << PHV::STR_OF_KIND.at(cls.kind) << " gress = " << cls.gress
             << " parser_group_gress = " << cls.parser_group_gress
             << " deparser_group_gress = " << cls.deparser_group_gress
             << " parser_extract_group_source = " << cls.parser_extract_group_source
             << " parity = " << int(cls.parity));
        return ins.first->second;
    }
    return std::nullopt;
}

}  // namespace PHV
