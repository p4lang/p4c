/*
Copyright 2013-present Barefoot Networks, Inc. 

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

/*
 * INT transit device implementation
 * This example implements INT transit functionality using VxLAN-GPE as
 * underlay protocol. The udp port# for vxlan-gpe is assume to be 4790
 * Only IPv4 is supported as underlay protocol in this example. 
 */

#ifdef INT_ENABLE
header_type int_metadata_t {
    fields {
        switch_id           : 32;
        insert_cnt          : 8;
        insert_byte_cnt     : 16; 
        gpe_int_hdr_len     : 16;
        gpe_int_hdr_len8    : 8;
        instruction_cnt     : 16;
    }
}
metadata int_metadata_t int_metadata;

header_type int_metadata_i2e_t {
    fields {
        sink    : 1;
        source  : 1;
    }
}
metadata int_metadata_i2e_t int_metadata_i2e;

/* Instr Bit 0 */
action int_set_header_0() { /* switch_id */
    add_header(int_switch_id_header);
    modify_field(int_switch_id_header.switch_id, int_metadata.switch_id);
}
/* Instr Bit 1 */
action int_set_header_1() { /* ingress_port_id */
    add_header(int_ingress_port_id_header);
    modify_field(int_ingress_port_id_header.ingress_port_id, 
                    ingress_metadata.ifindex);
}
/* Instr Bit 2 */
action int_set_header_2() { /* hop_latency */
    add_header(int_hop_latency_header);
    modify_field(int_hop_latency_header.hop_latency,
                                       intrinsic_metadata.deq_timedelta);
}
/* Instr Bit 3 */
action int_set_header_3() { /* q_occupancy */
    add_header(int_q_occupancy_header);
    modify_field(int_q_occupancy_header.q_occupancy,
                    intrinsic_metadata.enq_qdepth); 
}
/* Instr Bit 4 */
action int_set_header_4() { /* ingress_tstamp */
    add_header(int_ingress_tstamp_header);
    modify_field(int_ingress_tstamp_header.ingress_tstamp, 
                                            i2e_metadata.ingress_tstamp);
}
/* Instr Bit 5 */
action int_set_header_5() { /* egress_port_id */
    add_header(int_egress_port_id_header);
    modify_field(int_egress_port_id_header.egress_port_id,
                    standard_metadata.egress_port);
}

/* Instr Bit 6 */
action int_set_header_6() { /* q_congestion */
    add_header(int_q_congestion_header);
    /* un-supported */
    modify_field(int_q_congestion_header.q_congestion, 0x7FFFFFFF);
}
/* Instr Bit 7 */
action int_set_header_7() { /* egress_port_tx_utilization */
    add_header(int_egress_port_tx_utilization_header);
    /* un-supported */
    modify_field(int_egress_port_tx_utilization_header.egress_port_tx_utilization, 0x7FFFFFFF);
}

/* action functions for bits 0-3 combinations, 0 is msb, 3 is lsb */
/* Each bit set indicates that corresponding INT header should be added */
action int_set_header_0003_i0() {
}
action int_set_header_0003_i1() {
    int_set_header_3();
}
action int_set_header_0003_i2() {
    int_set_header_2();
}
action int_set_header_0003_i3() {
    int_set_header_3();
    int_set_header_2();
}
action int_set_header_0003_i4() {
    int_set_header_1();
}
action int_set_header_0003_i5() {
    int_set_header_3();
    int_set_header_1();
}
action int_set_header_0003_i6() {
    int_set_header_2();
    int_set_header_1();
}
action int_set_header_0003_i7() {
    int_set_header_3();
    int_set_header_2();
    int_set_header_1();
}
action int_set_header_0003_i8() {
    int_set_header_0();
}
action int_set_header_0003_i9() {
    int_set_header_3();
    int_set_header_0();
}
action int_set_header_0003_i10() {
    int_set_header_2();
    int_set_header_0();
}
action int_set_header_0003_i11() {
    int_set_header_3();
    int_set_header_2();
    int_set_header_0();
}
action int_set_header_0003_i12() {
    int_set_header_1();
    int_set_header_0();
}
action int_set_header_0003_i13() {
    int_set_header_3();
    int_set_header_1();
    int_set_header_0();
}
action int_set_header_0003_i14() {
    int_set_header_2();
    int_set_header_1();
    int_set_header_0();
}
action int_set_header_0003_i15() {
    int_set_header_3();
    int_set_header_2();
    int_set_header_1();
    int_set_header_0();
}

/* Table to process instruction bits 0-3 */
table int_inst_0003 {
    reads {
        int_header.instruction_mask_0003 : exact;
    }
    actions {
        int_set_header_0003_i0;
        int_set_header_0003_i1;
        int_set_header_0003_i2;
        int_set_header_0003_i3;
        int_set_header_0003_i4;
        int_set_header_0003_i5;
        int_set_header_0003_i6;
        int_set_header_0003_i7;
        int_set_header_0003_i8;
        int_set_header_0003_i9;
        int_set_header_0003_i10;
        int_set_header_0003_i11;
        int_set_header_0003_i12;
        int_set_header_0003_i13;
        int_set_header_0003_i14;
        int_set_header_0003_i15;
    }
    size : 16;
}

/* action function for bits 4-7, 4 is msb, 7 is lsb */
action int_set_header_0407_i0() {
}
action int_set_header_0407_i1() {
    int_set_header_7();
}
action int_set_header_0407_i2() {
    int_set_header_6();
}
action int_set_header_0407_i3() {
    int_set_header_7();
    int_set_header_6();
}
action int_set_header_0407_i4() {
    int_set_header_5();
}
action int_set_header_0407_i5() {
    int_set_header_7();
    int_set_header_5();
}
action int_set_header_0407_i6() {
    int_set_header_6();
    int_set_header_5();
}
action int_set_header_0407_i7() {
    int_set_header_7();
    int_set_header_6();
    int_set_header_5();
}
action int_set_header_0407_i8() {
    int_set_header_4();
}
action int_set_header_0407_i9() {
    int_set_header_7();
    int_set_header_4();
}
action int_set_header_0407_i10() {
    int_set_header_6();
    int_set_header_4();
}
action int_set_header_0407_i11() {
    int_set_header_7();
    int_set_header_6();
    int_set_header_4();
}
action int_set_header_0407_i12() {
    int_set_header_5();
    int_set_header_4();
}
action int_set_header_0407_i13() {
    int_set_header_7();
    int_set_header_5();
    int_set_header_4();
}
action int_set_header_0407_i14() {
    int_set_header_6();
    int_set_header_5();
    int_set_header_4();
}
action int_set_header_0407_i15() {
    int_set_header_7();
    int_set_header_6();
    int_set_header_5();
    int_set_header_4();
}

/* Table to process instruction bits 4-7 */
table int_inst_0407 {
    reads {
        int_header.instruction_mask_0407 : exact;
    }
    actions {
        int_set_header_0407_i0;
        int_set_header_0407_i1;
        int_set_header_0407_i2;
        int_set_header_0407_i3;
        int_set_header_0407_i4;
        int_set_header_0407_i5;
        int_set_header_0407_i6;
        int_set_header_0407_i7;
        int_set_header_0407_i8;
        int_set_header_0407_i9;
        int_set_header_0407_i10;
        int_set_header_0407_i11;
        int_set_header_0407_i12;
        int_set_header_0407_i13;
        int_set_header_0407_i14;
        int_set_header_0407_i15;
    }
    size : 16;
}

/* instruction mask bits 8-15 are not defined in the current spec */
table int_inst_0811 {
    reads {
        int_header.instruction_mask_0811 : exact;
    }
    actions {
        nop;
    }
    size : 16;
}

table int_inst_1215 {
    reads {
        int_header.instruction_mask_1215 : exact;
    }
    actions {
        nop;
    }
    size : 16;
}

/* 
 * BOS bit - set bottom of stack bit for the bottom most header added by 
 * INT src device 
 */
action int_set_header_0_bos() { /* switch_id */
    modify_field(int_switch_id_header.bos, 1);
}
action int_set_header_1_bos() { /* ingress_port_id */
    modify_field(int_ingress_port_id_header.bos, 1);
}
action int_set_header_2_bos() { /* hop_latency */
    modify_field(int_hop_latency_header.bos, 1);
}
action int_set_header_3_bos() { /* q_occupancy */
    modify_field(int_q_occupancy_header.bos, 1);
}
action int_set_header_4_bos() { /* ingress_tstamp */
    modify_field(int_ingress_tstamp_header.bos, 1);
}
action int_set_header_5_bos() { /* egress_port_id */
    modify_field(int_egress_port_id_header.bos, 1);
}
action int_set_header_6_bos() { /* q_congestion */
    modify_field(int_q_congestion_header.bos, 1);
}
action int_set_header_7_bos() { /* egress_port_tx_utilization */
    modify_field(int_egress_port_tx_utilization_header.bos, 1);
}

table int_bos {
    reads {
        int_header.total_hop_cnt            : ternary;
        int_header.instruction_mask_0003    : ternary;
        int_header.instruction_mask_0407    : ternary;
        int_header.instruction_mask_0811    : ternary;
        int_header.instruction_mask_1215    : ternary;
    }
    actions {
        int_set_header_0_bos;
        int_set_header_1_bos;
        int_set_header_2_bos;
        int_set_header_3_bos;
        int_set_header_4_bos;
        int_set_header_5_bos;
        int_set_header_6_bos;
        int_set_header_7_bos;
        nop;
    }
    size : 16;      /* number of instruction bits */
}

/* update the INT metadata header */
action int_set_e_bit() {
    modify_field(int_header.e, 1);
}

action int_update_total_hop_cnt() {
    add_to_field(int_header.total_hop_cnt, 1);
}

table int_meta_header_update {
    /* 
     * This table is applied only if int_insert table is a hit, which 
     * computes insert_cnt
     * E bit is set if insert_cnt == 0 => cannot add INT information
     * Else total_hop_cnt is incremented by one
     */
    reads {
        int_metadata.insert_cnt : ternary;
    }
    actions {
        int_set_e_bit;
        int_update_total_hop_cnt;
    }
    size : 1;
}
#endif

/*
 * Check if this switch needs to act as INT source or sink
 */
control process_int_endpoint {
    /* 
     * INT source/sink (endpoint) functionality is not implemented in
     * this example
     */
}

#ifdef INT_ENABLE
action int_transit(switch_id) {
    subtract(int_metadata.insert_cnt, int_header.max_hop_cnt,
                                            int_header.total_hop_cnt);
    modify_field(int_metadata.switch_id, switch_id);
    shift_left(int_metadata.insert_byte_cnt, int_metadata.instruction_cnt, 2);
    modify_field(int_metadata.gpe_int_hdr_len8, int_header.ins_cnt);
}

action int_reset() {
    modify_field(int_metadata.switch_id, 0);
    modify_field(int_metadata.insert_byte_cnt, 0);
    modify_field(int_metadata.insert_cnt, 0);
    modify_field(int_metadata.gpe_int_hdr_len8, 0);
    modify_field(int_metadata.gpe_int_hdr_len, 0);
    modify_field(int_metadata.instruction_cnt, 0);
}

table int_insert {
    /*
     * This table is used to decide if a given device should act as 
     * INT transit, INT source(not implemented) or not add INT information
     * to the packet.
     *  int_sink takes precedence over int_src
     *  {int_src, int_sink, int_header} :
     *       0, 0, 1 => transit  => insert_cnt = max-total
     *       1, 0, 0 => insert (src) => Not implemented here
     *       x, 1, x => nop (reset) => insert_cnt = 0
     *       1, 0, 1 => nop (error) (reset) => insert_cnt = 0
     *       miss (0,0,0) => nop (reset)
     */
    reads {
        int_metadata_i2e.source       : ternary;
        int_metadata_i2e.sink         : ternary;
        int_header                    : valid;
    }
    actions {
        int_transit;
        int_reset;
    }
    size : 2;
}
#endif

control process_int_insertion {
#ifdef INT_ENABLE
    apply(int_insert) {
        /* Bmv2 does not support 'hit' */
        int_transit {
            /*
             * int_transit computes, insert_cnt = max_hop_cnt - total_hop_cnt
             * (cannot be -ve, not checked)
             */
            if (int_metadata.insert_cnt != 0) {
                apply(int_inst_0003);
                apply(int_inst_0407);
                apply(int_inst_0811);
                apply(int_inst_1215);
                apply(int_bos);
            }

            /* update E-bit or total_hop_cnt in the INT header */
            apply(int_meta_header_update);
        }
    }
#endif
}

#ifdef INT_ENABLE
action int_update_vxlan_gpe_ipv4() {
    add_to_field(ipv4.totalLen, int_metadata.insert_byte_cnt);
    add_to_field(udp.length_, int_metadata.insert_byte_cnt);
    add_to_field(vxlan_gpe_int_header.len, int_metadata.gpe_int_hdr_len8);
}

table int_outer_encap {
    /*
     * This table is applied only if it is decided to add INT info
     * as part of transit or source functionality
     * based on outer(underlay) encap, vxlan-GPE in this example,
     * update outer headers, IP/UDP total len etc.
     * {int_src, vxlan_gpe, egr_tunnel_type} : 
     *      0, 0, X : nop (error)
     *      0, 1, X : update_vxlan_gpe_int (transit case)
     *      1, 0, tunnel_gpe : add_update_vxlan_gpe_int
     *      1, 1, X : add_update_vxlan_gpe_int
     *      miss => nop
     */
    reads {
        ipv4                                : valid;
        vxlan_gpe                           : valid;
        int_metadata_i2e.source             : exact;
        tunnel_metadata.egress_tunnel_type  : ternary;
    }
    actions {
        int_update_vxlan_gpe_ipv4;
        nop;
    }
    size : INT_UNDERLAY_ENCAP_TABLE_SIZE;
}
#endif

control process_int_outer_encap {
#ifdef INT_ENABLE
    if (int_metadata.insert_cnt != 0) {
        apply(int_outer_encap);
    }
#endif
}
