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

#include "gateway_control_flow.h"

const IR::MAU::Table *BFN::GatewayControlFlow::gateway_context(int &idx) const {
    const Context *ctxt = nullptr;
    if (auto *rv = findContext<IR::MAU::Table>(ctxt)) {
        if (size_t(ctxt->child_index) >= rv->gateway_rows.size())
            return nullptr;
        idx = ctxt->child_index;
        return rv; }
    return nullptr;
}

const IR::MAU::Table *BFN::GatewayControlFlow::gateway_context(cstring &tag) const {
    int idx;
    auto *rv = gateway_context(idx);
    if (rv)
        tag = rv->gateway_rows[idx].second;
    return rv;
}

std::set<cstring> BFN::GatewayControlFlow::gateway_earlier_tags() const {
    const Context *ctxt = nullptr;
    std::set<cstring> rv;
    if (auto *tbl = findContext<IR::MAU::Table>(ctxt)) {
        for (int i = 0; i < ctxt->child_index &&
                        i < static_cast<int>(tbl->gateway_rows.size()); ++i)
            rv.insert(tbl->gateway_rows[i].second); }
    return rv;
}

std::set<cstring> BFN::GatewayControlFlow::gateway_later_tags() const {
    const Context *ctxt = nullptr;
    std::set<cstring> rv;
    if (auto *tbl = findContext<IR::MAU::Table>(ctxt)) {
        for (size_t i = ctxt->child_index + 1; i < tbl->gateway_rows.size(); ++i)
            rv.insert(tbl->gateway_rows[i].second); }
    return rv;
}
