/*
Copyright 2015 Barefoot Networks, Inc.

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
action int_set_header_0() { //switch_id
    add_header(int_switch_id_header);
    modify_field(int_switch_id_header.switch_id, int_metadata.switch_id);
}
/* Instr Bit 1 */
action int_set_header_1() { //ingress_port_id
    add_header(int_ingress_port_id_header);
    modify_field(int_ingress_port_id_header.ingress_port_id_1, 0);
    modify_field(int_ingress_port_id_header.ingress_port_id_0,
                    ingress_metadata.ifindex);
}
/* Instr Bit 2 */
action int_set_header_2() { //hop_latency
    add_header(int_hop_latency_header);
    modify_field(int_hop_latency_header.hop_latency,
                    intrinsic_metadata.deq_timedelta);
}
/* Instr Bit 3 */
action int_set_header_3() { //q_occupancy
    add_header(int_q_occupancy_header);
    modify_field(int_q_occupancy_header.q_occupancy1, 0);
    modify_field(int_q_occupancy_header.q_occupancy0,
                    intrinsic_metadata.enq_qdepth);
}
/* Instr Bit 4 */
action int_set_header_4() { //ingress_tstamp
    add_header(int_ingress_tstamp_header);
    modify_field(int_ingress_tstamp_header.ingress_tstamp,
                                            i2e_metadata.ingress_tstamp);
}
/* Instr Bit 5 */
action int_set_header_5() { //egress_port_id
    add_header(int_egress_port_id_header);
    modify_field(int_egress_port_id_header.egress_port_id,
                    standard_metadata.egress_port);
}

/* Instr Bit 6 */
action int_set_header_6() { //q_congestion
    add_header(int_q_congestion_header);
    modify_field(int_q_congestion_header.q_congestion, 0x7FFFFFFF);
}
/* Instr Bit 7 */
action int_set_header_7() { //egress_port_tx_utilization
    add_header(int_egress_port_tx_utilization_header);
    modify_field(int_egress_port_tx_utilization_header.egress_port_tx_utilization, 0x7FFFFFFF);
}

/* action function for bits 0-3 combinations, 0 is msb, 3 is lsb */
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
    size : 17;
}

/* action function for bits 4-7 combinations, 4 is msb, 7 is lsb */
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
        nop;
    }
    size : 17;
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
    size : 17;
}

/* BOS bit - set for the bottom most header added by INT src device */
action int_set_header_0_bos() { //switch_id
    modify_field(int_switch_id_header.bos, 1);
}
action int_set_header_1_bos() { //ingress_port_id
    modify_field(int_ingress_port_id_header.bos, 1);
}
action int_set_header_2_bos() { //hop_latency
    modify_field(int_hop_latency_header.bos, 1);
}
action int_set_header_3_bos() { //q_occupancy
    modify_field(int_q_occupancy_header.bos, 1);
}
action int_set_header_4_bos() { //ingress_tstamp
    modify_field(int_ingress_tstamp_header.bos, 1);
}
action int_set_header_5_bos() { //egress_port_id
    modify_field(int_egress_port_id_header.bos, 1);
}
action int_set_header_6_bos() { //q_congestion
    modify_field(int_q_congestion_header.bos, 1);
}
action int_set_header_7_bos() { //egress_port_tx_utilization
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
    size : 17;       // number of instruction bits
}

// update the INT metadata header
action int_set_e_bit() {
    modify_field(int_header.e, 1);
}

action int_update_total_hop_cnt() {
    add_to_field(int_header.total_hop_cnt, 1);
}

table int_meta_header_update {
    // This table is applied only if int_insert table is a hit, which
    // computes insert_cnt
    // E bit is set if insert_cnt == 0
    // Else total_hop_cnt is incremented by one
    reads {
        int_metadata.insert_cnt : ternary;
    }
    actions {
        int_set_e_bit;
        int_update_total_hop_cnt;
    }
    size : 2;
}
#endif

#ifdef INT_EP_ENABLE
action int_set_src () {
    modify_field(int_metadata_i2e.source, 1);
}

action int_set_no_src () {
    modify_field(int_metadata_i2e.source, 0);
}

table int_source {
    // Decide to initiate INT based on client IP address pair
    // lkp_src, lkp_dst addresses are either outer or inner based
    // on if this switch is VTEP src or not respectively.
    //
    // {int_header, lkp_src, lkp_dst}
    //      0, src, dst => int_src=1
    //      1, x, x => mis-config, transient error, int_src=0
    //      miss => int_src=0
    reads {
        int_header                  : valid;
        // use outer ipv4/6 header when VTEP src
        ipv4                        : valid;
        ipv4_metadata.lkp_ipv4_da   : ternary;
        ipv4_metadata.lkp_ipv4_sa   : ternary;

        // use inner_ipv4 header when not VTEP src
        inner_ipv4                  : valid;
        inner_ipv4.dstAddr          : ternary;
        inner_ipv4.srcAddr          : ternary;
    }
    actions {
        int_set_src;
        int_set_no_src;
    }
    size : INT_SOURCE_TABLE_SIZE;
}

field_list int_i2e_mirror_info {
    int_metadata_i2e.sink;
    i2e_metadata.mirror_session_id;
}

action int_sink (mirror_id) {
    modify_field(int_metadata_i2e.sink, 1);

    // If this is sink, need to send the INT information to the
    // pre-processor/monitor. This is done via mirroring
    modify_field(i2e_metadata.mirror_session_id, mirror_id);
    clone_ingress_pkt_to_egress(mirror_id, int_i2e_mirror_info);

    // remove all the INT information from the packet
    // max 24 headers are supported
    remove_header(int_header);
    remove_header(int_val[0]);
    remove_header(int_val[1]);
    remove_header(int_val[2]);
    remove_header(int_val[3]);
    remove_header(int_val[4]);
    remove_header(int_val[5]);
    remove_header(int_val[6]);
    remove_header(int_val[7]);
    remove_header(int_val[8]);
    remove_header(int_val[9]);
    remove_header(int_val[10]);
    remove_header(int_val[11]);
    remove_header(int_val[12]);
    remove_header(int_val[13]);
    remove_header(int_val[14]);
    remove_header(int_val[15]);
    remove_header(int_val[16]);
    remove_header(int_val[17]);
    remove_header(int_val[18]);
    remove_header(int_val[19]);
    remove_header(int_val[20]);
    remove_header(int_val[21]);
    remove_header(int_val[22]);
    remove_header(int_val[23]);
}

action int_sink_gpe (mirror_id) {
    // convert the word len from gpe-shim header to byte_cnt
    shift_left(int_metadata.insert_byte_cnt, int_metadata.gpe_int_hdr_len, 2);
    int_sink(mirror_id);

}

action int_no_sink() {
    modify_field(int_metadata_i2e.sink, 0);
}

table int_terminate {
    /* REMOVE after discussion
     * It would be nice to keep this encap un-aware. But this is used
     * to compute byte count of INT info from shim headers from outer
     * protocols (vxlan_gpe_shim, geneve_tlv etc)
     * That make vxlan_gpe_int_header.valid as part of the key
     */

    // This table is used to decide if this node is INT sink
    // lkp_dst addr can be outer or inner ip addr, depending on how
    // user wants to configure.
    // {int_header, gpe, lkp_dst}
    //  1, 1, dst => int_gpe_sink(remove/update headers), int_sink=1
    //  (one entry per dst_addr)
    //  miss => no_sink
    reads {
        int_header                  : valid;
        vxlan_gpe_int_header        : valid;
        // when configured based on tunnel IPs
        ipv4                        : valid;
        ipv4_metadata.lkp_ipv4_da   : ternary;
        // when configured based on client IPs
        inner_ipv4                  : valid;
        inner_ipv4.dstAddr          : ternary;
    }
    actions {
        int_sink_gpe;
        int_no_sink;
    }
    size : INT_TERMINATE_TABLE_SIZE;
}

action int_sink_update_vxlan_gpe_v4() {
    modify_field(vxlan_gpe.next_proto, vxlan_gpe_int_header.next_proto);
    remove_header(vxlan_gpe_int_header);
    subtract(ipv4.totalLen, ipv4.totalLen, int_metadata.insert_byte_cnt);
    subtract(udp.length_, udp.length_, int_metadata.insert_byte_cnt);
}

table int_sink_update_outer {
    // This table is used to update the outer(underlay) headers on int_sink
    // to reflect removal of INT headers
    // Add more entries as other underlay protocols are added
    // {sink, gpe}
    // 1, 1 => update ipv4 and udp headers
    // miss => nop
    reads {
        vxlan_gpe_int_header        : valid;
        ipv4                        : valid;
        int_metadata_i2e.sink       : exact;
    }
    actions {
        int_sink_update_vxlan_gpe_v4;
        nop;
    }
    size : 2;
}
#endif

/*
 * Check if this switch needs to act as INT source or sink
 */
control process_int_endpoint {
#ifdef INT_EP_ENABLE
    if (not valid(int_header)) {
        apply(int_source);
    } else {
        apply(int_terminate);
        apply(int_sink_update_outer);
    }
#endif
}

#ifdef INT_ENABLE
#ifdef INT_TRANSIT_ENABLE
action int_transit(switch_id) {
    subtract(int_metadata.insert_cnt, int_header.max_hop_cnt,
                                            int_header.total_hop_cnt);
    modify_field(int_metadata.switch_id, switch_id);
    shift_left(int_metadata.insert_byte_cnt, int_metadata.instruction_cnt, 2);
    modify_field(int_metadata.gpe_int_hdr_len8, int_header.ins_cnt);
}
#endif

action int_reset() {
    modify_field(int_metadata.switch_id, 0);
    modify_field(int_metadata.insert_byte_cnt, 0);
    modify_field(int_metadata.insert_cnt, 0);
    modify_field(int_metadata.gpe_int_hdr_len8, 0);
    modify_field(int_metadata.gpe_int_hdr_len, 0);
    modify_field(int_metadata.instruction_cnt, 0);
}

#ifdef INT_EP_ENABLE
action int_src(switch_id, hop_cnt, ins_cnt, ins_mask0003, ins_mask0407, ins_byte_cnt, total_words) {
    modify_field(int_metadata.insert_cnt, hop_cnt);
    modify_field(int_metadata.switch_id, switch_id);
    modify_field(int_metadata.insert_byte_cnt, ins_byte_cnt);
    modify_field(int_metadata.gpe_int_hdr_len8, total_words);
    add_header(int_header);
    modify_field(int_header.ver, 0);
    modify_field(int_header.rep, 0);
    modify_field(int_header.c, 0);
    modify_field(int_header.e, 0);
    modify_field(int_header.rsvd1, 0);
    modify_field(int_header.ins_cnt, ins_cnt);
    modify_field(int_header.max_hop_cnt, hop_cnt);
    modify_field(int_header.total_hop_cnt, 0);
    modify_field(int_header.instruction_mask_0003, ins_mask0003);
    modify_field(int_header.instruction_mask_0407, ins_mask0407);
    modify_field(int_header.instruction_mask_0811, 0); // not supported
    modify_field(int_header.instruction_mask_1215, 0); // not supported
    modify_field(int_header.rsvd2, 0);
}
#endif

table int_insert {
    /* REMOVE - changed src/sink bits to ternary to use TCAM
     * keep int_header.valid in the key to force reset on error condition
     */

    // int_sink takes precedence over int_src
    // {int_src, int_sink, int_header} :
    //      0, 0, 1 => transit  => insert_cnt = max-total
    //      1, 0, 0 => insert (src) => insert_cnt = max
    //      x, 1, x => nop (reset) => insert_cnt = 0
    //      1, 0, 1 => nop (error) (reset) => insert_cnt = 0
    //      miss (0,0,0) => nop (reset)
    reads {
        int_metadata_i2e.source       : ternary;
        int_metadata_i2e.sink         : ternary;
        int_header                    : valid;
    }
    actions {
#ifdef INT_TRANSIT_ENABLE
        int_transit;
#endif
#ifdef INT_EP_ENABLE
        int_src;
#endif
        int_reset;
    }
    size : 3;
}
#endif

control process_int_insertion {
#ifdef INT_ENABLE
    apply(int_insert) {
        int_transit {
            // int_transit | int_src
            // insert_cnt = max_hop_cnt - total_hop_cnt
            // (cannot be -ve, not checked)
            if (int_metadata.insert_cnt != 0) {
                apply(int_inst_0003);
                apply(int_inst_0407);
                apply(int_inst_0811);
                apply(int_inst_1215);
                apply(int_bos);
            }
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

action int_add_update_vxlan_gpe_ipv4() {
    // INT source - vxlan gpe header is already added (or present)
    // Add the INT shim header for vxlan GPE
    add_header(vxlan_gpe_int_header);
    modify_field(vxlan_gpe_int_header.int_type, 0x01);
    modify_field(vxlan_gpe_int_header.next_proto, 3); // Ethernet
    modify_field(vxlan_gpe.next_proto, 5); // Set proto = INT
    modify_field(vxlan_gpe_int_header.len, int_metadata.gpe_int_hdr_len8);
    add_to_field(ipv4.totalLen, int_metadata.insert_byte_cnt);
    add_to_field(udp.length_, int_metadata.insert_byte_cnt);
}

table int_outer_encap {
    /* REMOVE from open-srouce version -
     * ipv4 and gpe valid bits are used as key so that other outer protocols
     * can be added in future. Table size
     */
    // This table is applied only if it is decided to add INT info
    // as part of transit or source functionality
    // based on outer(underlay) encap, vxlan-GPE, Geneve, .. update
    // outer headers, options, IP total len etc.
    // {int_src, vxlan_gpe, egr_tunnel_type} :
    //      0, 0, X : nop (error)
    //      0, 1, X : update_vxlan_gpe_int (transit case)
    //      1, 0, tunnel_gpe : add_update_vxlan_gpe_int
    //      1, 1, X : add_update_vxlan_gpe_int
    //      miss => nop
    reads {
        ipv4                                : valid;
        vxlan_gpe                           : valid;
        int_metadata_i2e.source             : exact;
        tunnel_metadata.egress_tunnel_type  : ternary;
    }
    actions {
#ifdef INT_TRANSIT_ENABLE
        int_update_vxlan_gpe_ipv4;
#endif
#ifdef INT_EP_ENABLE
        int_add_update_vxlan_gpe_ipv4;
#endif
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
