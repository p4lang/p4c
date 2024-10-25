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

#include "gateway_control_flow.h"

const IR::MAU::Table *BFN::GatewayControlFlow::gateway_context(int &idx) const {
    const Context *ctxt = nullptr;
    if (auto *rv = findContext<IR::MAU::Table>(ctxt)) {
        if (size_t(ctxt->child_index) >= rv->gateway_rows.size()) return nullptr;
        idx = ctxt->child_index;
        return rv;
    }
    return nullptr;
}

const IR::MAU::Table *BFN::GatewayControlFlow::gateway_context(cstring &tag) const {
    int idx;
    auto *rv = gateway_context(idx);
    if (rv) tag = rv->gateway_rows[idx].second;
    return rv;
}

std::set<cstring> BFN::GatewayControlFlow::gateway_earlier_tags() const {
    const Context *ctxt = nullptr;
    std::set<cstring> rv;
    if (auto *tbl = findContext<IR::MAU::Table>(ctxt)) {
        for (int i = 0; i < ctxt->child_index && i < static_cast<int>(tbl->gateway_rows.size());
             ++i)
            rv.insert(tbl->gateway_rows[i].second);
    }
    return rv;
}

std::set<cstring> BFN::GatewayControlFlow::gateway_later_tags() const {
    const Context *ctxt = nullptr;
    std::set<cstring> rv;
    if (auto *tbl = findContext<IR::MAU::Table>(ctxt)) {
        for (size_t i = ctxt->child_index + 1; i < tbl->gateway_rows.size(); ++i)
            rv.insert(tbl->gateway_rows[i].second);
    }
    return rv;
}
