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
