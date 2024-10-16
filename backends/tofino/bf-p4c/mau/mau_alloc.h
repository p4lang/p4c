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

#ifndef EXTENSIONS_BF_P4C_MAU_MAU_ALLOC_H_
#define EXTENSIONS_BF_P4C_MAU_MAU_ALLOC_H_

#include "ir/ir.h"
#include "lib/json.h"

#include "bf-p4c-options.h"
#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/phv/mau_backtracker.h"

class TableSummary;

class TableAllocPass : public Logging::PassManager {
 private:
    IgnoreTableDeps                     ignore;
    SplitAttachedInfo                   att_info;
    TablesMutuallyExclusive             mutex;
    ActionMutuallyExclusive             action_mutex;
    const BFN_Options&                  options;
    LayoutChoices                       *lc = nullptr;
    SharedIndirectAttachedAnalysis      *siaa = nullptr;

 public:
    static int table_placement_round;
    TableAllocPass(const BFN_Options& options, PhvInfo& phv,
                    DependencyGraph &deps, TableSummary &, Util::JsonObject*,
                    MauBacktracker &mau_backtracker);
};


#endif /* EXTENSIONS_BF_P4C_MAU_MAU_ALLOC_H_ */
