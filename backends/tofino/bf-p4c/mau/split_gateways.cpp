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

#include "split_gateways.h"

#include "ir/ir.h"
#include "lib/log.h"

Visitor::profile_t SpreadGatewayAcrossSeq::init_apply(const IR::Node *root) {
    auto rv = MauTransform::init_apply(root);
    root->apply(uses);
    LOG2(uses);
    return rv;
}

const IR::Node *SpreadGatewayAcrossSeq::postorder(IR::MAU::Table *t) {
    char suffix[14];
    int counter = 0;
#if 1
    if (!do_splitting) {
        prune();
        return t;
    }
#endif
    if (!t->uses_gateway()) return t;
    if (!t->conditional_gateway_only()) return t;
    assert(t->actions.empty());
    assert(t->attached.empty());
    auto rv = new IR::Vector<IR::MAU::Table>();
    for (auto it = t->next.rbegin(); it != t->next.rend(); it++) {
        auto seq = dynamic_cast<const IR::MAU::TableSeq *>(it->second);
        if (!seq) return t;
        bool splitting = true;
        IR::MAU::Table *newtable = nullptr;
        for (auto &table : seq->tables) {
            if (splitting) {
                snprintf(suffix, sizeof(suffix), ".%d", ++counter);
                newtable = t->clone_rename(cstring(suffix));
                newtable->next.clear();
                rv->push_back(newtable);
            }
            newtable->next[it->first] = new IR::MAU::TableSeq(newtable->next[it->first], table);
            if (uses.tables_modify(table) & uses.table_reads(t)) splitting = false;
        }
    }
    if (rv->size() <= 1) return t;
    for (int i = 1; i <= counter; i++) {
        snprintf(suffix, sizeof(suffix), ".%d", i);
        uses.cloning_table(t->name, t->name + suffix);
    }
    return rv;
}

static void erase_unused_next(IR::MAU::Table *tbl) {
    BUG_CHECK(tbl->match_table == nullptr, "Can only run erase_unused_next on pure gateways");
    std::set<cstring> results;
    for (auto &row : tbl->gateway_rows) results.insert(row.second);
    for (auto it = tbl->next.begin(); it != tbl->next.end();) {
        if (results.count(it->first) == 0)
            it = tbl->next.erase(it);
        else
            ++it;
    }
}

namespace P4 {
std::ostream &operator<<(std::ostream &out,
                         const std::pair<const P4::IR::Expression *, cstring> &p) {
    if (p.first) out << *p.first << " => ";
    out << (p.second ? p.second : "_");
    return out;
}
}  // namespace P4

const IR::MAU::Table *SplitComplexGateways::preorder(IR::MAU::Table *tbl) {
    if (tbl->gateway_rows.empty()) return tbl;
    BUG_CHECK(tbl->gateway_rows.back().first == nullptr, "Gateway not canonicalized?");
    if (tbl->match_table)
        BUG("Must run SplitComplexGateways before attaching gateways to match tables");
    CollectGatewayFields collect(phv);
    tbl->apply(collect);
    if (collect.compute_offsets() &&
        tbl->gateway_rows.size() <= static_cast<std::size_t>(Device::gatewaySpec().MaxRows + 1))
        return tbl;
    LOG1("Trying to split gateway " << tbl->name << tbl->gateway_rows << collect);
    for (unsigned i = tbl->gateway_rows.size() - 2; i > 0; --i) {
        if (i > 4) i = 4;
        CollectGatewayFields collect(phv, i);
        tbl->apply(collect);
        if (collect.compute_offsets()) {
            LOG1("Splitting " << i << " rows into " << tbl->name);
            auto rest = tbl->clone_rename("$split"_cs);
            rest->gateway_rows.erase(rest->gateway_rows.begin(), rest->gateway_rows.begin() + i);
            tbl->gateway_rows.erase(tbl->gateway_rows.begin() + i, tbl->gateway_rows.end());
            tbl->gateway_rows.emplace_back(nullptr, "$gwcont"_cs);
            tbl->next.emplace("$gwcont"_cs, new IR::MAU::TableSeq(rest));
            erase_unused_next(rest);
            erase_unused_next(tbl);
            return tbl;
        }
    }
    return tbl;
}
