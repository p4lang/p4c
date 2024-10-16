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

#ifndef BF_P4C_MAU_WALK_POWER_GRAPH_H_
#define BF_P4C_MAU_WALK_POWER_GRAPH_H_

#include <map>
#include <ostream>
#include <set>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/mau/build_power_graph.h"
#include "bf-p4c/mau/determine_power_usage.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/mau/mau_power.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/gress.h"
#include "power_schema.h"

namespace MauPower {

/**
 * This class uses all the collected information (Dependency graphs, power
 * estimations, control flow graphs) to adjust MAU stage dependencies,
 * compute MPR settings (for Tofino2 and beyond), and estimate the power
 * consumption of the worst-case table control flow for all threads of
 * compute (ingress, egress, and ghost).
 */
class WalkPowerGraph : public MauInspector {
 public:
    using PowerLogging = Logging::Power_Schema_Logger;
    bool preorder(const IR::MAU::Table *t) override;
    void end_apply(const IR::Node *root) override;
    WalkPowerGraph(const NextTable *next_table_properties, BuildPowerGraph *graphs,
                   const bool &exceeds_stages,
                   const std::map<UniqueId, PowerMemoryAccess> &table_memory_access,
                   const BFN_Options &options, MauFeatures *mau_features,
                   std::map<gress_t, MprSettings *> &mpr_settings)
        : next_table_properties_(next_table_properties),
          graphs_(graphs),
          exceeds_stages_(exceeds_stages),
          options_(options),
          table_memory_access_(table_memory_access),
          mau_features_(mau_features),
          mpr_settings_(mpr_settings) {
        disable_mpr_config_ = options.disable_mpr_config;
        if (Device::currentDevice() != Device::TOFINO) {
            // This will not be necessary when turn on MPR by default.
            // disable_mpr_config_ = true;
        }
    }

 private:
    // inputs
    const NextTable *next_table_properties_;
    BuildPowerGraph *graphs_;
    const bool &exceeds_stages_;
    const BFN_Options &options_;
    const std::map<UniqueId, PowerMemoryAccess> &table_memory_access_;
    // modified
    MauFeatures *mau_features_;
    std::map<gress_t, MprSettings *> &mpr_settings_;
    // created
    PowerLogging *logger_ = nullptr;
    // Temporarily use this variable until can turn MPR on by default.
    bool disable_mpr_config_ = false;
    /**
     * Mapping from gress_t to the worst-case estimated power.
     */
    std::map<gress_t, double> gress_powers_real_ = {};
    /**
     * Mapping from gress_t to the worst-case estimated power adjusted to traffic
     * limit if present.
     */
    std::map<gress_t, double> gress_powers_ = {};
    /**
     * Mapping from UniqueId to Boolean indicating if the logical table is
     * considered to be on the worst-case table control flow for power estimation.
     */
    std::map<UniqueId, bool> on_critical_path_ = {};
    /**
     * Mapping from UniqueId to Boolean indicating if the logical is always powered
     * on.  On Tofino, this would be a table that is not on the critical path
     * but is an action- or concurrent-dependent MAU stage.
     */
    std::map<UniqueId, bool> always_powered_on_ = {};
    /*
     * Powered on due to latency difference between ingress & egress
     * JBAY-2889 HW Issue
     */
    std::map<UniqueId, bool> always_powered_on_latency_ = {};
    /**
     * Mapping from UniqueId to external name.
     */
    std::map<UniqueId, cstring> external_names_ = {};
    /**
     * Mapping from Unique ID to what stage it is found in.
     */
    std::map<UniqueId, int> stages_ = {};
    /**
     * Mapping from Unique ID to what gress it is found in.
     */
    std::map<UniqueId, gress_t> gress_map_ = {};
    /**
     * For text formatting purposes, keep track of the longest table name.
     */
    size_t longest_table_name_ = 0;

    /**
     * During power estimation, MPR settings are computed to help
     * determine what tables' lookups are powered on.  Since power estimation
     * is computed in a loop, this functions resets the settings prior to the
     * next iteration.
     */
    void clear_mpr_settings();
    void compute_mpr();
    bool check_mpr_conflict();  // check for ingress/egress match/action conflict
    /**
     * Function to call to estimate the total MAU power.
     */
    double estimate_power();
    /**
     * Tofino-specific function to estimate the worst-case MAU power.
     */
    double estimate_power_tofino();
    /**
     * Tofino2 and beyond-specific function to estimate the worst-case MAU power.
     */
    double estimate_power_non_tofino();

    /**
     * Convenience function to return whether the given MAU table object is
     * considered always powered on in MPR settings.
     * This function is only intended to be called for Tofino2 and beyond and when
     * the stage it is in is not match dependent.
     */
    bool is_mpr_powered_on(gress_t g, int stage, const IR::MAU::Table *t) const;

    /**
     * Scale power based on input traffic limit
     */
    double traffic_limit_scaling(double pwr) const;

    /**
     * JSON logging functions, for producing power.json.
     */
    void create_mau_power_json(const IR::Node *root);
    void produce_json_tables();
    void produce_json_total_power(int pipe_id);
    void produce_json_total_latency(int pipe_id);
    void produce_json_stage_characteristics();

    /**
     * Text-based logging functions.
     */
    void create_mau_power_log(const IR::Node *root) const;
    void print_features(std::ofstream &out) const;
    void print_latency(std::ofstream &out) const;
    void print_mpr_settings(std::ofstream &out) const;
    void print_worst_power(std::ofstream &out) const;
};

}  // end namespace MauPower

#endif /* BF_P4C_MAU_WALK_POWER_GRAPH_H_ */
