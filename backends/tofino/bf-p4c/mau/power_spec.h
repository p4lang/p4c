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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_POWER_SPEC_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_POWER_SPEC_H_

/**
 * This class encapsulates MAU power-related elements of the target device model,
 * such as the measured or modeled power consumption of a read and/or write
 * of each memory type in the MAU.  In general, raw power estimates
 * should *not*
 * be exposed to the end user, other than the final total.  Instead, a unitless
 * normalized value should be used.
 * Since power needs MAU stage latency information (for Tofino at least),
 * MAU stage cycle latencies are also represented here.
 */
class MauPowerSpec {
 public:
    MauPowerSpec() {}

    virtual double get_max_power() const = 0;  // Value for all pipes.
    /**
     * The additional power given when --disable-power-check is provided
     * at the command line.
     */
    virtual double get_excess_power_threshold() const = 0;
    virtual double get_absolute_max_power_threshold() const = 0;

    /**
     * Memory access power consumption (in Watts)
     * Note that these numbers are not to be made available in any
     * user-facing form.
     */
    virtual double get_ram_scaling_factor() const = 0;
    virtual double get_tcam_scaling_factor() const = 0;
    virtual double get_voltage_scaling_factor() const = 0;
    virtual double get_ram_read_power() const = 0;
    virtual double get_ram_write_power() const = 0;
    virtual double get_tcam_search_power() const = 0;
    virtual double get_map_ram_read_power() const = 0;
    virtual double get_map_ram_write_power() const = 0;
    virtual double get_deferred_ram_read_power() const = 0;
    virtual double get_deferred_ram_write_power() const = 0;

    /**
     * For computing MAU stage latencies (number of clock cycles).
     */
    virtual int get_base_delay() const { return 20; }
    virtual int get_base_predication_delay() const { return 11; }
    virtual int get_tcam_delay() const { return 2; }
    virtual int get_selector_delay() const { return 8; }
    virtual int get_meter_lpf_delay() const { return 4; }
    virtual int get_stateful_delay() const { return 4; }
    virtual int get_mau_corner_turn_delay() const = 0;

    /**
     * For computing latency contribution of a MAU stage.
     */
    virtual int get_concurrent_latency_contribution() const = 0;
    virtual int get_action_latency_contribution() const { return 2; }

    /**
     * For enforcing minimum latency (where appropriate).
     */
    virtual int get_min_required_egress_mau_latency() const = 0;

    /**
     * For scaling MAU power based on deparser buffer depth.
     * The deparser limits how many PHVs may be traversing the MAU
     * pipeline simultaneously.
     */
    virtual int get_deparser_throughput_scaling_starts() const = 0;
    virtual int get_deparser_max_phv_valid() const = 0;
    virtual int get_cycles_to_issue_credit_to_pmarb() const = 0;
    virtual int get_pmarb_cycles_from_receive_credit_to_issue_phv_to_mau() const = 0;
};

class TofinoMauPowerSpec : public MauPowerSpec {
 public:
    TofinoMauPowerSpec() {}

    double get_max_power() const override { return 40.0; }
    double get_excess_power_threshold() const override { return 10.0; }
    double get_absolute_max_power_threshold() const override { return 22.0; }

    /**
     * Memory access power consumption (in Watts)
     * Note that these numbers are not to be made available in any
     * user-facing form.
     */
    double get_ram_scaling_factor() const override {
        return 1.88574 * get_voltage_scaling_factor();
    }
    double get_tcam_scaling_factor() const override {
        return 0.62736 * get_voltage_scaling_factor();
    }
    /**
     * No voltage scaling factor currently needed for Tofino.
     */
    double get_voltage_scaling_factor() const override { return 1.0; }

    // Power nos are for 1.27 GHz PPS (Clock freq of MAU)
    double get_ram_read_power() const override { return 0.012771 * get_ram_scaling_factor(); }
    double get_ram_write_power() const override { return 0.023419 * get_ram_scaling_factor(); }
    double get_tcam_search_power() const override { return 0.0398 * get_tcam_scaling_factor(); }
    double get_map_ram_read_power() const override { return 0.0025861 * get_ram_scaling_factor(); }
    double get_map_ram_write_power() const override { return 0.0030107 * get_ram_scaling_factor(); }
    double get_deferred_ram_read_power() const override {
        return 0.0029238 * get_ram_scaling_factor();
    }
    double get_deferred_ram_write_power() const override {
        return 0.0020185 * get_ram_scaling_factor();
    }

    int get_mau_corner_turn_delay() const override { return 4; }
    int get_concurrent_latency_contribution() const override { return 1; }
    int get_min_required_egress_mau_latency() const override { return 160; }

    // For scaling MAU power based on deparser throttling.
    int get_deparser_throughput_scaling_starts() const override {
        return get_deparser_max_phv_valid() - get_cycles_to_issue_credit_to_pmarb() -
               get_pmarb_cycles_from_receive_credit_to_issue_phv_to_mau();
    }
    int get_deparser_max_phv_valid() const override { return 288; }
    int get_cycles_to_issue_credit_to_pmarb() const override { return 28; }
    int get_pmarb_cycles_from_receive_credit_to_issue_phv_to_mau() const override { return 11; }
};

/**
 * Most of these numbers are currently based on library models, not measurements.
 * This will be re-visited once more measurements have been procured.
 * When a number was not available, we defaulted to Tofino's unscaled value.
 */
class JBayMauPowerSpec : public MauPowerSpec {
 public:
    JBayMauPowerSpec() {}

    double get_max_power() const override { return 52.0; }
    double get_excess_power_threshold() const override { return 10.0; }
    double get_absolute_max_power_threshold() const override { return 10.0; }

    /**
     * Memory access power consumption (in Watts)
     * Note that these numbers are not to be made available in any
     * user-facing form.
     */
    double get_ram_scaling_factor() const override { return 1.0 * get_voltage_scaling_factor(); }
    double get_tcam_scaling_factor() const override { return 1.0 * get_voltage_scaling_factor(); }
    double get_voltage_scaling_factor() const override {
        // The voltage the unit memory numbers are derived from.
        double measured_voltage = 0.750;  // in Volts
        // TODO: Pass in a voltage as command line argument, and sanity check.
        double user_voltage = 0.750;  // in Volts
        // Power consumption scales with difference in voltage from measured as:
        // (V_user / V_meas)^2
        return (user_voltage / measured_voltage) * (user_voltage / measured_voltage);
    }

    // Power nos are for 1.5 GHz PPS (Clock freq of MAU)
    double get_ram_read_power() const override { return 0.0166 * get_ram_scaling_factor(); }
    double get_ram_write_power() const override { return 0.0176 * get_ram_scaling_factor(); }
    double get_tcam_search_power() const override { return 0.0185 * get_tcam_scaling_factor(); }
    double get_map_ram_read_power() const override { return 0.0027 * get_ram_scaling_factor(); }
    double get_map_ram_write_power() const override { return 0.0030 * get_ram_scaling_factor(); }
    double get_deferred_ram_read_power() const override {
        return 0.0025 * get_ram_scaling_factor();
    }
    double get_deferred_ram_write_power() const override {
        return 0.0024 * get_ram_scaling_factor();
    }

    /**
     * There is no imposed corner-turn latency for Tofino2, as there is no
     * corner to turn.
     */
    int get_mau_corner_turn_delay() const override { return 0; }

    /**
     * For computing latency contribution of a MAU stage.
     * There is no concurrent dependency type for for Tofino2.
     * In such cases, use an action dependency.
     */
    int get_concurrent_latency_contribution() const override {
        return get_action_latency_contribution();
    }

    /**
     * There is currently no minimum egress pipeline latency for Tofino2.
     */
    int get_min_required_egress_mau_latency() const override { return 0; }

    /**
     * For scaling MAU power based on deparser throttling.
     * Currently, deparser scaling does not apply to Tofino2.
     * If it does eventually, update this value.
     * For now, it is given a value that is greater than the maximum possible
     * latency (30 cycles * 20 stages)
     */
    int get_deparser_throughput_scaling_starts() const override { return 600; }

    /**
     * FIXME: Do not currently know these numbers, or whether they will be relevant.
     * Since they are not needed currently for Tofino2, leave them at 0.
     */
    int get_deparser_max_phv_valid() const override { return 0; }
    int get_cycles_to_issue_credit_to_pmarb() const override { return 0; }
    int get_pmarb_cycles_from_receive_credit_to_issue_phv_to_mau() const override { return 0; }
};

#if BAREFOOT_INTERNAL
class JBayHMauPowerSpec : public JBayMauPowerSpec {
 public:
    JBayHMauPowerSpec() {}
    double get_max_power() const override { return 15.6; }  // scaled by stages
};
#endif /* BAREFOOT_INTERNAL */

class JBayMMauPowerSpec : public JBayMauPowerSpec {
 public:
    JBayMMauPowerSpec() {}
    double get_max_power() const override { return 31.2; }  // scaled by stages
};

class JBayUMauPowerSpec : public JBayMauPowerSpec {
 public:
    JBayUMauPowerSpec() {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_POWER_SPEC_H_ */
