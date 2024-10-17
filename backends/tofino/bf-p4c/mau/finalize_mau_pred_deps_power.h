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

#ifndef BF_P4C_MAU_FINALIZE_MAU_PRED_DEPS_POWER_H_
#define BF_P4C_MAU_FINALIZE_MAU_PRED_DEPS_POWER_H_

#include <map>
#include <ostream>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/mau/build_power_graph.h"
#include "bf-p4c/mau/determine_power_usage.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/pass_manager.h"

namespace MauPower {

class MauFeatures;

/**
  * Collection of passes to:
  * 1. Set the MAU stage dependencies
  * 2. Compute the match-power reduction configuration (Tofino2 and beyond)
  * 3. Estimate the worst-case MAU power usage
  * 4. Log information to JSON and text files.
  * 5. Output dependencies and MPR configuration to the assembly file.
  */
class FinalizeMauPredDepsPower : public PassManager {
 public:
  FinalizeMauPredDepsPower(const PhvInfo& phv, DependencyGraph& dep_graph,
                           const NextTable* next_table_properties,
                           const BFN_Options& options);
  std::ostream& emit_stage_asm(std::ostream &out, gress_t g, int stage) const;
  bool requires_stage_asm(gress_t g, int stage) const;

 private:
  // inputs
  const PhvInfo& phv_;
  DependencyGraph& dep_graph_;
  const NextTable* next_table_properties_;
  const BFN_Options& options_;

  // derived
  MauFeatures* mau_features_;
  // flag indicating if table placement used more stages than available in the device.
  bool exceeds_stages_ = false;
  // map from match table UniqueId to tuple of memory accesses
  // A match table will also include any attached table's memory accesses used.
  std::map<UniqueId, PowerMemoryAccess> table_memory_access_ = {};

  // Per-gress graphs collected that will be walked to estimate power usage and compute MPR.
  BuildPowerGraph* graphs_ = nullptr;
  // Map from gress to MAU MPR settings.
  std::map<gress_t, MprSettings*> mpr_settings_ = {};
};

}  // end namespace MauPower

#endif /* BF_P4C_MAU_FINALIZE_MAU_PRED_DEPS_POWER_H_ */
