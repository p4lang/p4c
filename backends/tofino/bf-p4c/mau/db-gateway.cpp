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

#include "bf-p4c/phv/phv_fields.h"
#include "gateway.h"
#include "ir/dbprint.h"
#include "lib/indent.h"
#include "lib/log.h"

using namespace DBPrint;
using namespace IndentCtl;

std::ostream &operator<<(std::ostream &out, const CollectGatewayFields::info_t &info) {
    if (info.const_eq) out << " const_eq";
    if (info.need_range) out << " range";
    if (info.valid_bit) out << " valid";
    for (auto &off : info.offsets) out << " " << off.first << ":" << off.second;
    for (auto &off : info.xor_offsets) out << " xor:" << off.first << ":" << off.second;
    return out;
}

std::ostream &operator<<(std::ostream &out, const CollectGatewayFields &gwf) {
    auto flags = dbgetflags(out);
    out << Brief << indent;
    if (gwf.tbl) {
        out << "table: " << gwf.tbl->name;
        if (gwf.ixbar) out << "  ";
    }
    if (gwf.ixbar) out << "ixbar: " << indent << *gwf.ixbar << unindent;
    for (auto &i : gwf.info) {
        out << Log::endl << i.first << i.second;
        for (auto x : i.second.xor_with) out << " xor " << x;
    }
    if (gwf.bytes || gwf.bits) out << Log::endl << "bytes=" << gwf.bytes << " bits=" << gwf.bits;
    out << unindent;
    dbsetflags(out, flags);
    return out;
}

std::ostream &operator<<(std::ostream &out, const BuildGatewayMatch &m) {
    if (m.range_match.empty()) return out << m.match;
    // only used as a gateway key, so format it as a YAML complex key
    out << "? [ ";
    for (int i = m.range_match.size() - 1; i >= 0; --i)
        out << "0x" << hex(m.range_match[i]) << ", ";
    return out << m.match << " ] ";
}

void dump(const CollectGatewayFields &gwf) { std::cout << gwf << std::endl; }
void dump(const CollectGatewayFields *gwf) { std::cout << *gwf << std::endl; }
void dump(const BuildGatewayMatch &gwm) { std::cout << gwm << std::endl; }
void dump(const BuildGatewayMatch *gwm) { std::cout << *gwm << std::endl; }
