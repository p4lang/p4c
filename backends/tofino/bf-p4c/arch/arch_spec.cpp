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

#include "arch/arch_spec.h"

ArchSpec::ArchSpec() {
    // ingress_intrinsic_metadata_for_tm
    add_tm_md(INGRESS, IntrinsicField("ucast_egress_port", "egress_unicast_port"));
    add_tm_md(INGRESS, IntrinsicField("deflect_on_drop"));
    add_tm_md(INGRESS, IntrinsicField("ingress_cos", "icos"));
    add_tm_md(INGRESS, IntrinsicField("qid"));
    add_tm_md(INGRESS, IntrinsicField("icos_for_copy_to_cpu", "copy_to_cpu_cos"));
    add_tm_md(INGRESS, IntrinsicField("copy_to_cpu"));
    add_tm_md(INGRESS, IntrinsicField("packet_color", "meter_color"));
    add_tm_md(INGRESS, IntrinsicField("disable_ucast_cutthru", "ct_disable"));
    add_tm_md(INGRESS, IntrinsicField("enable_mcast_cutthru", "ct_mcast"));
    add_tm_md(INGRESS, IntrinsicField("mcast_grp_a"));
    add_tm_md(INGRESS, IntrinsicField("mcast_grp_b"));
    add_tm_md(INGRESS, IntrinsicField("level1_mcast_hash"));
    add_tm_md(INGRESS, IntrinsicField("level2_mcast_hash"));
    add_tm_md(INGRESS, IntrinsicField("level1_exclusion_id", "xid"));
    add_tm_md(INGRESS, IntrinsicField("level2_exclusion_id", "yid"));
    add_tm_md(INGRESS, IntrinsicField("rid"));

    // egress_intrinsic_metadata
    add_md(EGRESS, IntrinsicField("egress_port", "egress_unicast_port"));

    // egress_intrinsic_metadata_for_deparser
    add_dprsr_md(EGRESS, IntrinsicField("drop_ctl"));
}

void ArchSpec::setTofinoIntrinsicTypes() {
    parser_intrinsic_types[INGRESS] = {
        {"ingress_intrinsic_metadata_t", "ingress_intrinsic_metadata"},
        {"ingress_intrinsic_metadata_for_tm_t", "ingress_intrinsic_metadata_for_tm"},
        {"ingress_intrinsic_metadata_from_parser_t", "ingress_intrinsic_metadata_from_parser"}};

    parser_intrinsic_types[EGRESS] = {
        {"egress_intrinsic_metadata_t", "egress_intrinsic_metadata"},
        {"egress_intrinsic_metadata_from_parser_t", "egress_intrinsic_metadata_from_parser"},
        {"egress_intrinsic_metadata_for_deparser_t", "egress_intrinsic_metadata_for_deparser"}};

    mauppu_intrinsic_types[INGRESS] = {
        {"ingress_intrinsic_metadata_t", "ingress_intrinsic_metadata"},
        {"ingress_intrinsic_metadata_for_tm_t", "ingress_intrinsic_metadata_for_tm"},
        {"ingress_intrinsic_metadata_from_parser_t", "ingress_intrinsic_metadata_from_parser"},
        {"ingress_intrinsic_metadata_for_deparser_t", "ingress_intrinsic_metadata_for_deparser"}};

    mauppu_intrinsic_types[EGRESS] = {
        {"egress_intrinsic_metadata_t", "egress_intrinsic_metadata"},
        {"egress_intrinsic_metadata_from_parser_t", "egress_intrinsic_metadata_from_parser"},
        {"egress_intrinsic_metadata_for_deparser_t", "egress_intrinsic_metadata_for_deparser"},
        {"egress_intrinsic_metadata_for_output_port_t",
         "egress_intrinsic_metadata_for_output_port"}};

    deparser_intrinsic_types[INGRESS] = {
        {"ingress_intrinsic_metadata_t", "ingress_intrinsic_metadata"},
        {"ingress_intrinsic_metadata_for_deparser_t", "ingress_intrinsic_metadata_for_deparser"}};

    deparser_intrinsic_types[EGRESS] = {
        {"egress_intrinsic_metadata_t", "egress_intrinsic_metadata"},
        {"egress_intrinsic_metadata_from_parser_t", "egress_intrinsic_metadata_from_parser"},
        {"egress_intrinsic_metadata_for_deparser_t", "egress_intrinsic_metadata_for_deparser"}};
}

TofinoArchSpec::TofinoArchSpec() : ArchSpec() {
    // Index of the intrinsic metadata for deparser param
    deparser_intrinsic_metadata_for_deparser_param_index = 3;

    // ingress_intrinsic_metadata_from_parser
    add_prsr_md(INGRESS, IntrinsicField("global_tstamp"));
    add_prsr_md(INGRESS, IntrinsicField("global_ver"));
    add_prsr_md(INGRESS, IntrinsicField("parser_err"));

    // ingress_intrinsic_metadata_for_tm
    add_tm_md(INGRESS, IntrinsicField("bypass_egress", "bypss_egr"));

    // ingress_intrinsic_metadata_for_deparser
    add_dprsr_md(INGRESS, IntrinsicField("drop_ctl"));

    // egress_intrinsic_metadata_for_output_port
    add_outport_md(EGRESS, IntrinsicField("capture_tstamp_on_tx", "capture_tx_ts"));
    add_outport_md(EGRESS, IntrinsicField("update_delay_on_tx", "tx_pkt_has_offsets"));

    setTofinoIntrinsicTypes();
}

JBayArchSpec::JBayArchSpec() : ArchSpec() {
    // Index of the intrinsic metadata for deparser param
    deparser_intrinsic_metadata_for_deparser_param_index = 3;

    // ingress_intrinsic_metadata_from_parser
    add_prsr_md(INGRESS, IntrinsicField("global_tstamp"));
    add_prsr_md(INGRESS, IntrinsicField("global_ver"));
    add_prsr_md(INGRESS, IntrinsicField("parser_err"));

    // ingress_intrinsic_metadata_for_tm
    add_tm_md(INGRESS, IntrinsicField("bypass_egress", "bypss_egr"));

    // ingress_intrinsic_metadata_for_deparser
    add_dprsr_md(INGRESS, IntrinsicField("drop_ctl"));

    // ingress_intrinsic_metadata_for_deparser
    add_dprsr_md(INGRESS, IntrinsicField("mirror_hash", "mirr_hash"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_io_select", "mirr_io_sel"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_egress_port", "mirr_epipe_port"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_qid", "mirr_qid"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_deflect_on_drop", "mirr_dond_ctrl"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_ingress_cos", "mirr_icos"));
    add_dprsr_md(INGRESS, IntrinsicField("mirror_multicast_ctrl", "mirr_mc_ctrl"));
    // XXX(TF2LAB-105): disabled due to JBAY-A0 TM BUG
    // add_dprsr_md(INGRESS, IntrinsicField("mirror_copy_to_cpu_ctrl", "mirr_c2c_ctrl"));
    add_dprsr_md(INGRESS, IntrinsicField("adv_flow_ctl", "afc"));
    add_dprsr_md(INGRESS, IntrinsicField("mtu_trunc_len"));
    add_dprsr_md(INGRESS, IntrinsicField("mtu_trunc_err_f"));
    add_dprsr_md(INGRESS, IntrinsicField("pktgen", "pgen"));
    add_dprsr_md(INGRESS, IntrinsicField("pktgen_length", "pgen_len"));
    add_dprsr_md(INGRESS, IntrinsicField("pktgen_address", "pgen_addr"));

    // egress_intrinsic_metadata_for_deparser
    add_dprsr_md(EGRESS, IntrinsicField("mirror_hash", "mirr_hash"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_io_select", "mirr_io_sel"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_egress_port", "mirr_epipe_port"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_qid", "mirr_qid"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_deflect_on_drop", "mirr_dond_ctrl"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_ingress_cos", "mirr_icos"));
    add_dprsr_md(EGRESS, IntrinsicField("mirror_multicast_ctrl", "mirr_mc_ctrl"));
    // XXX(TF2LAB-105): disabled due to JBAY-A0 TM BUG
    // add_dprsr_md(EGRESS, IntrinsicField("mirror_copy_to_cpu_ctrl", "mirr_c2c_ctrl"));
    add_dprsr_md(EGRESS, IntrinsicField("adv_flow_ctl", "afc"));
    add_dprsr_md(EGRESS, IntrinsicField("mtu_trunc_len"));
    add_dprsr_md(EGRESS, IntrinsicField("mtu_trunc_err_f"));

    // egress_intrinsic_metadata_for_output_port
    add_outport_md(EGRESS, IntrinsicField("capture_tstamp_on_tx", "capture_tx_ts"));
    add_outport_md(EGRESS, IntrinsicField("update_delay_on_tx", "tx_pkt_has_offsets"));

    setTofinoIntrinsicTypes();
}
