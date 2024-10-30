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

#include "resource.h"

TableResourceAlloc *TableResourceAlloc::rename(const IR::MAU::Table *tbl, int stage_table,
                                               int logical_table) {
    UniqueId u_id = tbl->pp_unique_id();
    u_id.stage_table = stage_table;
    u_id.logical_table = logical_table;

    // Only keep resource nodes that are part of the same stage table and logical table
    for (auto it = memuse.begin(); it != memuse.end();) {
        if (u_id.equal_table(it->first))
            ++it;
        else
            it = memuse.erase(it);
    }

    // Gateway ixbar has to be cleared from ATCAM in the non-first logical table
    bool has_gateway = false;
    for (auto &kv : memuse) {
        if (kv.second.type == Memories::Use::GATEWAY) {
            has_gateway = true;
            break;
        }
    }

    if (!has_gateway) gateway_ixbar.reset();

    return this;
}

void TableResourceAlloc::merge_instr(const TableResourceAlloc *resources) {
    instr_mem.merge(resources->instr_mem);
}

/**
 * A ternary indirect table might have been created, when the original layout did not have
 * one, as a gateway is overriding the original TCAM table.
 */
bool TableResourceAlloc::has_tind() const {
    bool rv = false;
    for (auto &kv : memuse) {
        if (kv.second.type != Memories::Use::TIND) continue;
        rv = true;
        break;
    }

    if (rv)
        BUG_CHECK(table_format.has_overhead(),
                  "A ternary indirect table is currently "
                  "required with no overhead");
    return rv;
}

/**
 * Return a 2 entry vector indicating which fields are headed to HashDist Immed Lo and
 * HashDist Immediate Hi.  If the hash dist is not used, the a -1 will be returned.
 */
safe_vector<int> TableResourceAlloc::hash_dist_immed_units() const {
    safe_vector<int> rv;
    for (int i = IXBar::HD_IMMED_LO; i <= IXBar::HD_IMMED_HI; i++) {
        int unit = -1;
        for (auto &hd : hash_dists) {
            if (hd.destinations().getbit(i)) {
                BUG_CHECK(unit == -1,
                          "Multiple HashDistUse objects cannot head to the same "
                          "output destination");
                unit = hd.unit;
            }
        }
        rv.push_back(unit);
    }
    return rv;
}

/**
 * Returns which rng unit has been assigned to this table
 */
int TableResourceAlloc::rng_unit() const {
    if (action_data_xbar) return action_data_xbar->rng_unit();
    return -1;
}

int TableResourceAlloc::findBytesOnIxbar(const PHV::FieldSlice &slice) const {
    int bytesOnIxbar = 0;
    if (match_ixbar) bytesOnIxbar = match_ixbar->findBytesOnIxbar(slice);
    if (bytesOnIxbar > 0) return bytesOnIxbar;
    if (gateway_ixbar) bytesOnIxbar = gateway_ixbar->findBytesOnIxbar(slice);
    if (bytesOnIxbar > 0) return bytesOnIxbar;
    if (proxy_hash_ixbar) bytesOnIxbar = proxy_hash_ixbar->findBytesOnIxbar(slice);
    if (bytesOnIxbar > 0) return bytesOnIxbar;
    if (selector_ixbar) bytesOnIxbar = selector_ixbar->findBytesOnIxbar(slice);
    if (bytesOnIxbar > 0) return bytesOnIxbar;
    if (salu_ixbar) bytesOnIxbar = salu_ixbar->findBytesOnIxbar(slice);
    if (bytesOnIxbar > 0) return bytesOnIxbar;
    if (meter_ixbar) bytesOnIxbar = meter_ixbar->findBytesOnIxbar(slice);
    return bytesOnIxbar;
}

::IXBar::Use *TableResourceAlloc::find_ixbar(IXBar::Use::type_t type) const {
    switch (type) {
        case IXBar::Use::EXACT_MATCH:
            return match_ixbar.get();
        case IXBar::Use::TERNARY_MATCH:
            return match_ixbar.get();
        case IXBar::Use::GATEWAY:
            return gateway_ixbar.get();
        case IXBar::Use::PROXY_HASH:
            return proxy_hash_ixbar.get();
        case IXBar::Use::SELECTOR:
            return selector_ixbar.get();
        case IXBar::Use::STATEFUL_ALU:
            return salu_ixbar.get();
        case IXBar::Use::METER:
            return meter_ixbar.get();
        case IXBar::Use::ACTION:
            return action_ixbar.get();
        case IXBar::Use::HASH_DIST:
            return nullptr;  // intentionally not supported, hash_dist IR is different
        default:
            BUG("Unknown ixbar type %d", type);
    }
}

std::ostream &operator<<(std::ostream &out, const TableResourceAlloc &alloc) {
    // FIXME -- there's a huge amount of data in alloc -- what should we log?
    // this is a prime candidate for structured logging.
    if (alloc.match_ixbar) out << "match_ixbar: " << *alloc.match_ixbar << Log::endl;
    if (alloc.gateway_ixbar) out << "gateway_ixbar: " << *alloc.gateway_ixbar << Log::endl;
    if (alloc.proxy_hash_ixbar) out << "proxy_hash_ixbar: " << *alloc.proxy_hash_ixbar << Log::endl;
    if (alloc.action_ixbar) out << "action_ixbar: " << *alloc.action_ixbar << Log::endl;
    if (alloc.selector_ixbar) out << "selector_ixbar: " << *alloc.selector_ixbar << Log::endl;
    if (alloc.salu_ixbar) out << "salu_ixbar: " << *alloc.salu_ixbar << Log::endl;
    if (alloc.meter_ixbar) out << "meter_ixbar: " << *alloc.meter_ixbar << Log::endl;
    std::unique_ptr<Memories> mem(Memories::create());
    for (auto &mu : alloc.memuse) mem->update(mu.first.build_name(), mu.second);
    out << *mem;
    return out;
}

void dump(const TableResourceAlloc &ra) { std::cout << ra << std::endl; }
void dump(const TableResourceAlloc *ra) { std::cout << *ra << std::endl; }
