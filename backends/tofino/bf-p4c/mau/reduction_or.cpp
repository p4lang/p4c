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
