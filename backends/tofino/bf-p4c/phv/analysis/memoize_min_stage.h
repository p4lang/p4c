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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MEMOIZE_MIN_STAGE_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MEMOIZE_MIN_STAGE_H_

#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/phv_fields.h"

// MemoizeStage save table stage mapping to static vairables of PhvInfo object,
// both physical stage and min stage.
class MemoizeStage : public Inspector {
 private:
    const DependencyGraph &dg;          // for min stages
    const MauBacktracker &backtracker;  // for physical stages

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::Table *tbl) override;

 public:
    explicit MemoizeStage(const DependencyGraph &d, const MauBacktracker &backtracker)
        : dg(d), backtracker(backtracker) {}
};

#endif /*  BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MEMOIZE_MIN_STAGE_H_  */
