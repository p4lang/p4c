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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_MAU_ALLOC_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_MAU_ALLOC_H_

#include "bf-p4c-options.h"
#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "ir/ir.h"
#include "lib/json.h"

class TableSummary;

class TableAllocPass : public Logging::PassManager {
 private:
    IgnoreTableDeps ignore;
    SplitAttachedInfo att_info;
    TablesMutuallyExclusive mutex;
    ActionMutuallyExclusive action_mutex;
    const BFN_Options &options;
    LayoutChoices *lc = nullptr;
    SharedIndirectAttachedAnalysis *siaa = nullptr;

 public:
    static int table_placement_round;
    TableAllocPass(const BFN_Options &options, PhvInfo &phv, DependencyGraph &deps, TableSummary &,
                   Util::JsonObject *, MauBacktracker &mau_backtracker);
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_MAU_ALLOC_H_ */
