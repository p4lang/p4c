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

#include "bf-p4c/mau/reduction_or.h"

/**
 * A check to see if an instruction is part of a reduction or.  The operation is part of
 * a binary OR, i.e.:
 *
 *    f = f | salu.execute();
 *
 * where the stateful ALU is part of a reduction or group found in the map.  This will return
 * the name of the key used by the stateful ALU.
 */
bool ReductionOrInfo::is_reduction_or(const IR::MAU::Instruction *instr, const IR::MAU::Table *tbl,
                                      cstring &reduction_or_key) const {
    if (instr == nullptr || tbl == nullptr) return false;

    if (!(instr->name == "or" || instr->operands.size() == 3)) return false;

    std::function<const IR::Node *(const IR::Node *)> ignoreSlice =
        [&ignoreSlice](const IR::Node *n) {
            if (auto s = n->to<IR::Slice>()) return ignoreSlice(s->e0);
            return n;
        };

    int non_attached_index = 0;
    const IR::MAU::AttachedOutput *attached = nullptr;
    if ((attached = ignoreSlice(instr->operands.at(2))->to<IR::MAU::AttachedOutput>()))
        non_attached_index = 1;
    else if ((attached = ignoreSlice(instr->operands.at(1))->to<IR::MAU::AttachedOutput>()))
        non_attached_index = 2;

    if (!(non_attached_index > 0 &&
          instr->operands.at(0)->equiv(*instr->operands.at(non_attached_index))))
        return false;

    auto salu = attached->attached->to<IR::MAU::StatefulAlu>();
    if (salu == nullptr) return false;

    if (salu->reduction_or_group.isNull()) return false;
    auto &tbl_set = tbl_reduction_or_group.at(salu->reduction_or_group);
    if (!tbl_set.count(tbl)) return false;
    reduction_or_key = salu->reduction_or_group;
    return true;
}

Visitor::profile_t GatherReductionOrReqs::init_apply(const IR::Node *node) {
    auto rv = MauInspector::init_apply(node);
    red_or_info.clear();
    return rv;
}

bool GatherReductionOrReqs::preorder(const IR::MAU::StatefulAlu *salu) {
    auto tbl = findContext<IR::MAU::Table>();
    if (salu->reduction_or_group) {
        red_or_info.salu_reduction_or_group[salu->reduction_or_group].insert(salu);
        red_or_info.tbl_reduction_or_group[salu->reduction_or_group].insert(tbl);
    }
    return false;
}
