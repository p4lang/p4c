/*
Copyright 2016-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifdef SFLOW_ENABLE
header_type sflow_meta_t {
    fields {
        sflow_session_id        : 16;
    }
}
metadata sflow_meta_t sflow_metadata;

counter sflow_ingress_session_pkt_counter {
    type : packets;
    direct : sflow_ing_take_sample;
    saturating;
}
#endif

/* ---------------------- sflow ingress processing ------------------------ */
#ifdef SFLOW_ENABLE
action sflow_ing_session_enable(rate_thr, session_id)
{
    /* take_sample(sat) = rate_thr + take_sample(initialized from RNG) */
    /* if take_sample == max_val, sample will be take in the subsequent table-action */
    add(ingress_metadata.sflow_take_sample, rate_thr, 
                                ingress_metadata.sflow_take_sample);
    modify_field(sflow_metadata.sflow_session_id, session_id);
}

table sflow_ingress {
    /* Table to determine ingress port based enablement */
    /* This is separate from ACLs so that this table can be applied */
    /* independent of ACLs */
    reads {
        ingress_metadata.ifindex    : ternary;
        ipv4_metadata.lkp_ipv4_sa   : ternary;
        ipv4_metadata.lkp_ipv4_da   : ternary;
        sflow                       : valid;    /* do not sflow an sflow frame */
    }
    actions {
        nop; /* default action */
        sflow_ing_session_enable;
    }
    size : SFLOW_INGRESS_TABLE_SIZE;
}

field_list sflow_cpu_info {
    cpu_info;
    sflow_metadata.sflow_session_id;
    i2e_metadata.mirror_session_id;
}

action sflow_ing_pkt_to_cpu(sflow_i2e_mirror_id, reason_code) {
    modify_field(fabric_metadata.reason_code, reason_code);
    modify_field(i2e_metadata.mirror_session_id, sflow_i2e_mirror_id);
    clone_ingress_pkt_to_egress(sflow_i2e_mirror_id, sflow_cpu_info);
}

table sflow_ing_take_sample {
    /* take_sample > MAX_VAL_31 and valid sflow_session_id => take the sample */
    reads {
        ingress_metadata.sflow_take_sample : ternary;
        sflow_metadata.sflow_session_id : exact;
    }
    actions {
        nop;
        sflow_ing_pkt_to_cpu;
    }
}
#endif /*SFLOW_ENABLE */

control process_ingress_sflow {
#ifdef SFLOW_ENABLE
    apply(sflow_ingress);
    apply(sflow_ing_take_sample);
#endif
}
/* ----- egress processing ----- */
#ifdef SFLOW_ENABLE
action sflow_pkt_to_cpu() {
    /* This action is called from the mirror table in the egress pipeline */
    /* Add sflow header to the packet */
    /* sflow header sits between cpu header and the rest of the original packet */
    /* The reasonCode in the cpu header is used to identify the */
    /* presence of the cpu header */
    /* pkt_count(sample_pool) on a given sflow session is read directly by CPU */
    /* using counter read mechanism */
    add_header(fabric_header_sflow);
    modify_field(fabric_header_sflow.sflow_session_id,
                 sflow_metadata.sflow_session_id);
}
#endif
