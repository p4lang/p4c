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
