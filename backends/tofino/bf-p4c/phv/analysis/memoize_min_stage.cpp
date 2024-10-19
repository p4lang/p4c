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

#include "bf-p4c/phv/analysis/memoize_min_stage.h"

Visitor::profile_t MemoizeStage::init_apply(const IR::Node *root) {
    PhvInfo::clearMinStageInfo();
    LOG4("Printing dependency graph");
    LOG4(dg);
    PhvInfo::setDeparserStage(dg.max_min_stage + 1);

    PhvInfo::clearPhysicalStageInfo();
    return Inspector::init_apply(root);
}

bool MemoizeStage::preorder(const IR::MAU::Table *tbl) {
    cstring tblName = TableSummary::getTableName(tbl);
    LOG2("\t" << dg.min_stage(tbl) << " : " << tblName << ", backend name: " << tbl->name);
    PhvInfo::addMinStageEntry(tbl, dg.min_stage(tbl));

    if (backtracker.hasTablePlacement()) {
        const auto &stages = backtracker.stage(tbl, true);
        PhvInfo::setPhysicalStages(tbl, {stages.begin(), stages.end()});
    }
    return true;
}
