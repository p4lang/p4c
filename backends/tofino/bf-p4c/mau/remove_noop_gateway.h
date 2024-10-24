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

#ifndef BF_P4C_MAU_REMOVE_NOOP_GATEWAY_H_
#define BF_P4C_MAU_REMOVE_NOOP_GATEWAY_H_

#include "mau_visitor.h"

using namespace P4;

/* "Noop" gateways that don't actually test anything and just trigger another TableSeq
 * may be introduced by MulitpleApply::MergeTails; they don't actually do anything, but
 * are needed to allow table placement to place the tables properly.  We may be able to
 * eliminate them after table placement, which this pass attempts to do */
class RemoveNoopGateway : public MauTransform {
    const IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *seq) override {
        if (seq->size() < 1) return seq;
        auto *last = seq->back();
        if (!last->conditional_gateway_only() || last->gateway_rows.size() != 1 ||
            last->next.size() != 1) {
            // not a noop gateway
            return seq;
        }
        if (seq->size() == 1) {
            // drop the noop gateway, going directly to the target TableSeq
            return last->next.begin()->second;
        }
        auto *prev = seq->tables.at(seq->size() - 2);
        if (!prev->next.empty()) {
            // can't move the dependent tables to the previous table
            return seq;
        }
        seq->tables.pop_back();       // toss the gateway;
        auto *clone = prev->clone();  // clone the last table
        clone->next["$default"_cs] = last->next.begin()->second;
        // move the dependent sequence to the last table as default next
        seq->tables.back() = clone;
        return seq;
    }
};

#endif /* BF_P4C_MAU_REMOVE_NOOP_GATEWAY_H_ */
