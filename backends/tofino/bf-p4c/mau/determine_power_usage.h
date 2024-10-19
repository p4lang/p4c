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

#ifndef BF_P4C_MAU_DETERMINE_POWER_USAGE_H_
#define BF_P4C_MAU_DETERMINE_POWER_USAGE_H_

#include <map>

#include "bf-p4c/device.h"
#include "bf-p4c/logging/manifest.h"
#include "bf-p4c/mau/build_power_graph.h"
#include "bf-p4c/mau/finalize_mau_pred_deps_power.h"
#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "lib/error.h"
#include "lib/json.h"
#include "power_schema.h"

namespace MauPower {

using namespace P4;

class MauFeatures;

/**
 * This class is used to estimate the power usage for each logical table.
 * Power estimation is based on the number of memories (RAMs, TCAMs, MapRAMs,
 * Deferred RAMs) that will be read and written.  Each type of access
 * has a device-measured or modeled value.
 */
class DeterminePowerUsage : public MauInspector {
    // map from match table UniqueId to tuple of memory accesses
    // A match table will also include any attached table's memory accesses used.
    std::map<UniqueId, PowerMemoryAccess> &table_memory_access_;

    // map from attached table's UniqueId to memory access struct.
    // Only populated with stats, meters, stateful, selector, action data.
    // This is needed because a shared attached table is only attached to one
    // of its match tables, but each match table that accesses shared table
    // needs to factor in power used by its attached.
    std::map<UniqueId, PowerMemoryAccess> attached_memory_usage_ = {};

    // Need to update these to include accesses from 'attached_memory_usage'
    // once that information is found.
    IR::Vector<IR::MAU::Table> match_tables_with_unattached_;

    // map from table to Boolean indicating if the table accesses mocha containers
    // in the match input crossbar.
    std::map<UniqueId, bool> table_uses_mocha_container_ = {};

    /* --------------------------------------------------------
     *  Function definitions.
     * --------------------------------------------------------*/
    void end_apply(const IR::Node *root) override;
    void postorder(const IR::BFN::Pipe *p) override;
    void postorder(const IR::MAU::Table *t) override;
    bool preorder(const IR::MAU::Meter *m) override;
    bool preorder(const IR::MAU::Counter *c) override;
    bool preorder(const IR::MAU::Selector *sel) override;

 private:
    /**
     * Since table visit order does not guarantee that the match table
     * an unattached table is attached to has been visited, need a function
     * for after the traversal that fills in the memory accesses used
     * by these side effect tables.
     */
    void add_unattached_memory_accesses();

    /**
     * Discovers the MAU stage dependencies to the previous stage
     * by using a DependencyGraph and the final table placement.
     */
    void find_stage_dependencies();

    /**
     * Tofino requires a minimum egress pipeline latency.
     * This function adds match dependencies to egress MAU stages to
     * increase the latency.
     */
    void update_stage_dependencies_for_min_latency();

    /**
     * @return Boolean indicating if a mocha PHV container is used by this table in
     *         an match input crossbar usage.
     */
    bool uses_mocha_containers_in_ixbar(const IR::MAU::Table *t) const;

    const PhvInfo &phv_;
    DependencyGraph &dep_graph_;
    BuildPowerGraph *graphs_;
    const BFN_Options &options_;
    /**
     * Keeps track of what features are used in a given MAU stage per gress.
     * This information is needed to determine stage latency (in cycles).
     */
    MauFeatures *mau_features_;
    bool &exceeds_stages_;

 public:
    explicit DeterminePowerUsage(const PhvInfo &phv, DependencyGraph &dep_graph,
                                 BuildPowerGraph *control_graphs, const BFN_Options &options,
                                 MauFeatures *mau_features, bool &exceeds_stages,
                                 std::map<UniqueId, PowerMemoryAccess> &table_memory_access)
        : table_memory_access_(table_memory_access),
          phv_(phv),
          dep_graph_(dep_graph),
          graphs_(control_graphs),
          options_(options),
          mau_features_(mau_features),
          exceeds_stages_(exceeds_stages) {
        force_match_dependency_ = options_.force_match_dependency;
        if (Device::currentDevice() != Device::TOFINO) {
            // This will not be necessary when turn on MPR by default.
            // force_match_dependency_ = true;
        }
    }

 private:
    // Temporarily use this variable until can turn MPR on by default.
    bool force_match_dependency_ = false;
};

}  // end namespace MauPower

#endif /* BF_P4C_MAU_DETERMINE_POWER_USAGE_H_ */
