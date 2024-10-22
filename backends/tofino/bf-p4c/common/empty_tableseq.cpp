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

#include "empty_tableseq.h"

void AddEmptyTableSeqs::postorder(IR::BFN::Pipe *pipe) {
    // Ensure pipes do not have null table sequences.
    for (auto &thread : pipe->thread) {
        if (!thread.mau) thread.mau = new IR::MAU::TableSeq();
    }
}

void AddEmptyTableSeqs::postorder(IR::MAU::Table *tbl) {
    if (tbl->next.empty()) {
        // No conditional flow based on the result of this table, so nothing to do here.
        return;
    }

    if (tbl->hit_miss_p4()) {
        for (auto key : {"$hit"_cs, "$miss"_cs}) {
            if (tbl->next.count(key) == 0) tbl->next[key] = new IR::MAU::TableSeq();
        }
    } else if (tbl->has_default_path()) {
        // Nothing to do.
    } else if (tbl->action_chain() && tbl->next.size() < tbl->actions.size()) {
        tbl->next["$default"_cs] = new IR::MAU::TableSeq();
    }

    for (auto &row : tbl->gateway_rows) {
        auto key = row.second;
        if (key && tbl->next.count(key) == 0) {
            tbl->next[key] = new IR::MAU::TableSeq();
        }
    }
}
