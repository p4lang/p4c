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

#include "gateway.h"
#include "bf-p4c/phv/phv_fields.h"
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
        if (gwf.ixbar) out << "  "; }
    if (gwf.ixbar) out << "ixbar: " << indent << *gwf.ixbar << unindent;
    for (auto &i : gwf.info) {
        out << Log::endl << i.first << i.second;
        for (auto x : i.second.xor_with)
            out << " xor " << x; }
    if (gwf.bytes || gwf.bits)
        out << Log::endl << "bytes=" << gwf.bytes << " bits=" << gwf.bits;
    out << unindent;
    dbsetflags(out, flags);
    return out;
}

std::ostream &operator<<(std::ostream &out, const BuildGatewayMatch &m) {
    if (m.range_match.empty())
        return out << m.match;
    // only used as a gateway key, so format it as a YAML complex key
    out << "? [ ";
    for (int i = m.range_match.size()-1; i >= 0; --i)
        out << "0x" << hex(m.range_match[i]) << ", ";
    return out << m.match << " ] ";
}

void dump(const CollectGatewayFields &gwf) { std::cout << gwf << std::endl; }
void dump(const CollectGatewayFields *gwf) { std::cout << *gwf << std::endl; }
void dump(const BuildGatewayMatch &gwm) { std::cout << gwm << std::endl; }
void dump(const BuildGatewayMatch *gwm) { std::cout << *gwm << std::endl; }
