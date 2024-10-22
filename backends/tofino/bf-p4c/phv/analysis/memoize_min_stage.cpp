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
