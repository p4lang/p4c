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

#include "bf-p4c/mau/finalize_mau_pred_deps_power.h"

#include <map>
#include <ostream>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/mau/build_power_graph.h"
#include "bf-p4c/mau/determine_power_usage.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/walk_power_graph.h"
#include "bf-p4c/phv/phv_fields.h"

namespace MauPower {

FinalizeMauPredDepsPower::FinalizeMauPredDepsPower(const PhvInfo &phv, DependencyGraph &dep_graph,
                                                   const NextTable *next_table_properties,
                                                   const BFN_Options &options)
    : phv_(phv),
      dep_graph_(dep_graph),
      next_table_properties_(next_table_properties),
      options_(options),
      mau_features_(new MauFeatures()) {
    graphs_ = new BuildPowerGraph(next_table_properties_, options_);

    addPasses({// Get final dependency graph
               // Populates dep_graph_.
               new FindDependencyGraph(phv_, dep_graph_, &options_, "power_graph"_cs,
                                       "Power Calculation"_cs),

               // Construct final control flow graphs that will be utilized
               // to estimate power and compute MPR settings.
               graphs_,

               // Estimate power usage for each logical table.
               // Populates mau_features_, exceeds_stages_, and table_memory_access_.
               new DeterminePowerUsage(phv_, dep_graph_, graphs_, options_, mau_features_,
                                       exceeds_stages_, table_memory_access_),

               // Walk power graph to compute MPR, estimate power, update latencies.
               // Populates mpr_settings_.  Updates stage dependencies in mau_features_.
               new WalkPowerGraph(next_table_properties_, graphs_, exceeds_stages_,
                                  table_memory_access_, options_, mau_features_, mpr_settings_)});
}

std::ostream &FinalizeMauPredDepsPower::emit_stage_asm(std::ostream &out, gress_t g,
                                                       int stage) const {
    mau_features_->emit_dep_asm(out, g, stage);
    if (mpr_settings_.find(g) != mpr_settings_.end())
        mpr_settings_.at(g)->emit_stage_asm(out, stage);
    return out;
}

bool FinalizeMauPredDepsPower::requires_stage_asm(gress_t g, int stage) const {
    return mau_features_->requires_dep_asm(g, stage);
}

}  // end namespace MauPower
