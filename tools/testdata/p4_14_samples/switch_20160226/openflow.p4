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
 * Openflow Processing
 */

// Openflow features
#define OPENFLOW_ENABLE_MPLS
#define OPENFLOW_ENABLE_VLAN
#define OPENFLOW_ENABLE_L3

/* enables fabric header for non-switch.p4 targets */
//#define OPENFLOW_PACKET_IN_OUT 

header_type openflow_metadata_t {
    fields {
        index : 32;
        bmap : 32;
        group_id : 32;
        ofvalid : 1;
    }
}

metadata openflow_metadata_t openflow_metadata;

#ifndef CPU_PORT_ID
    #define CPU_PORT_ID 64
#endif

#ifdef OPENFLOW_PACKET_IN_OUT
#define ETHERTYPE_BF_FABRIC 0x9000

#define TRUE 1

header_type fabric_header_t {
    fields {
        packetType : 3;
        headerVersion : 2;
        packetVersion : 2;
        pad1 : 1;

        fabricColor : 3;
        fabricQos : 5;

        dstDevice : 8;
        dstPortOrGroup : 16;
    }
}

header_type fabric_header_cpu_t {
    fields {
        egressQueue : 5;
        txBypass : 1;
        reserved : 2;

        ingressPort: 16;
        ingressIfindex : 16;
        ingressBd : 16;

        reasonCode : 16;
    }
}

header_type fabric_payload_header_t {
    fields {
        etherType : 16;
    }
}

header fabric_header_t fabric_header;
header fabric_header_cpu_t fabric_header_cpu;
header fabric_payload_header_t fabric_payload_header;

parser fabric_header {
    extract(fabric_header);
    extract(fabric_header_cpu);
    extract(fabric_payload_header);
    return ingress;
}

action nop () {
}

action terminate_cpu_packet() {
    modify_field(standard_metadata.egress_spec,fabric_header.dstPortOrGroup);
    modify_field(ethernet.etherType, fabric_payload_header.etherType);

    remove_header(fabric_header);
    remove_header(fabric_header_cpu);
    remove_header(fabric_payload_header);
}
#endif /* OPENFLOW_PACKET_IN_OUT */

/****************************************************************
 * Actions common to all openflow tables, sets a bitmap indicating
 * which OFPAT to be applied to packets in flow flow_id.
 ****************************************************************/

action openflow_apply(bmap, index, group_id) {
    modify_field(openflow_metadata.bmap, bmap);
    modify_field(openflow_metadata.index, index);
    modify_field(openflow_metadata.group_id, group_id);
    modify_field(openflow_metadata.ofvalid, TRUE);
//    modify_field(egress_metadata.bypass, TRUE);
}

action openflow_miss(reason, table_id) {
    modify_field(fabric_metadata.reason_code, reason);

    shift_left(fabric_metadata.reason_code, fabric_metadata.reason_code, 8);
    bit_or(fabric_metadata.reason_code, fabric_metadata.reason_code, table_id);

    modify_field(standard_metadata.egress_spec, CPU_PORT_ID);
}

/***************************************************************
 * Packet Out
 ***************************************************************/

action packet_out_eth_flood() {
    modify_field(intrinsic_metadata.mcast_grp, fabric_header.dstPortOrGroup);
    terminate_cpu_packet();
    modify_field(openflow_metadata.ofvalid, TRUE);
}

action packet_out_unicast() {
    modify_field(standard_metadata.egress_spec, fabric_header.dstPortOrGroup);
    terminate_cpu_packet();
    modify_field(openflow_metadata.ofvalid, TRUE);
}

table packet_out {
    reads {
        fabric_header.packetType : exact;
        fabric_header_cpu.reasonCode : exact;
    }

    actions {
        packet_out_eth_flood;
        packet_out_unicast;
        nop;
    }
}

/****************************************************************
 * Egress openflow bitmap translation
 ****************************************************************/

action ofpat_group_egress_update(bmap) {
    bit_or (openflow_metadata.bmap, openflow_metadata.bmap, bmap);
}

table ofpat_group_egress {
    reads {
        openflow_metadata.group_id : exact;
        standard_metadata.egress_port : exact;
    }

    actions {
        ofpat_group_egress_update;
        nop;
    }
}

/****************************************************************
 * GROUPS 
 ****************************************************************/

action ofpat_group_ingress_uc(ifindex) {
    modify_field(standard_metadata.egress_spec, ifindex);
}

action ofpat_group_ingress_mc(mcindex) {
    modify_field(intrinsic_metadata.mcast_grp, mcindex);
}

table ofpat_group_ingress {
    reads {
        openflow_metadata.group_id : exact;
    }

    actions {
        ofpat_group_ingress_uc;
        ofpat_group_ingress_mc;
        nop;
    }
}

/****************************************************************
 * OFPAT_OUTPUT
 ****************************************************************/

action ofpat_output(ifindex) {
    modify_field(standard_metadata.egress_spec, ifindex);
}

table ofpat_output {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_spec : ternary;
    }

    actions {
        ofpat_output;
        nop;
    }
}

#ifdef OPENFLOW_ENABLE_MPLS
/***************************************************************
 * OFPAT_SET_MPLS_TTL
 ***************************************************************/

action ofpat_set_mpls_ttl(ttl) {
    modify_field(mpls[0].ttl, ttl);
}

table ofpat_set_mpls_ttl {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_set_mpls_ttl;
        nop;
    }
}

/***************************************************************
 * OFPAT_DEC_MPLS_TTL
 ***************************************************************/

action ofpat_dec_mpls_ttl() {
    add_to_field(mpls[0].ttl, -1);
}

table ofpat_dec_mpls_ttl {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_dec_mpls_ttl;
        nop;
    }
}

/****************************************************************
 * OFPAT_PUSH_MPLS
 ****************************************************************/

action ofpat_push_mpls() {
    modify_field(ethernet.etherType, 0x8847);
    add_header(mpls[0]);
}

table ofpat_push_mpls {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_push_mpls;
        nop;
    }
}

/***************************************************************
 * OFPAT_POP_MPLS
 ***************************************************************/

action ofpat_pop_mpls() {
    remove_header(mpls[0]);
}

table ofpat_pop_mpls {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_pop_mpls;
        nop;
    }
}
#endif /* OPENFLOW_ENABLE_MPLS */
#ifdef OPENFLOW_ENABLE_VLAN
/***************************************************************
 * OFPAT_PUSH_VLAN
 ***************************************************************/

action ofpat_push_vlan() {
    modify_field(ethernet.etherType, 0x8100);
    add_header(vlan_tag_[0]);
    modify_field(vlan_tag_[0].etherType, 0x0800);
}

table ofpat_push_vlan {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_push_vlan;
        nop;
    }
}

/***************************************************************
 * OFPAT_POP_VLAN
 ***************************************************************/

action ofpat_pop_vlan() {
    modify_field(ethernet.etherType, vlan_tag_[0].etherType);
    remove_header(vlan_tag_[0]);
}

table ofpat_pop_vlan {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }
    
    actions {
        ofpat_pop_vlan;
        nop;
    }
}

/***************************************************************
 * OFPAT_SET_FIELD
 ***************************************************************/

action ofpat_set_vlan_vid(vid) {
    modify_field(vlan_tag_[0].vid, vid);
}

table ofpat_set_field {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_set_vlan_vid;
        nop;
    }
}

#endif /* OPENFLOW_ENABLE_VLAN */

/****************************************************************
 * OFPAT_SET_QUEUE
 ****************************************************************/


 //TODO

#ifdef OPENFLOW_ENABLE_L3
/***************************************************************
 * OFPAT_SET_NW_TTL IPV4
 ***************************************************************/

action ofpat_set_nw_ttl_ipv4(ttl) {
    modify_field(ipv4.ttl, ttl);
}

table ofpat_set_nw_ttl_ipv4 {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_set_nw_ttl_ipv4;
        nop;
    }
}

/***************************************************************
 * OFPAT_SET_NW_TTL IPV6
 ***************************************************************/

action ofpat_set_nw_ttl_ipv6(ttl) {
    modify_field(ipv6.hopLimit, ttl);
}

table ofpat_set_nw_ttl_ipv6 {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_set_nw_ttl_ipv6;
        nop;
    }
}

/***************************************************************
 * OFPAT_DEC_NW_TTL IPV4
 ***************************************************************/

action ofpat_dec_nw_ttl_ipv4() {
    add_to_field(ipv4.ttl, -1);
}

table ofpat_dec_nw_ttl_ipv4 {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_dec_nw_ttl_ipv4;
        nop;
    }
}

/***************************************************************
 * OFPAT_DEC_NW_TTL IPV6
 ***************************************************************/

action ofpat_dec_nw_ttl_ipv6(ttl) {
    add_to_field(ipv6.hopLimit, -1);
}

table ofpat_dec_nw_ttl_ipv6 {
    reads {
        openflow_metadata.index : ternary;
        openflow_metadata.group_id : ternary;
        standard_metadata.egress_port : ternary;
    }

    actions {
        ofpat_dec_nw_ttl_ipv6;
        nop;
    }
}
#endif /* OPENFLOW_ENABLE_L3 */

/***************************************************************
 * Main control block
 ***************************************************************/

control process_ofpat_ingress {
    if (openflow_metadata.bmap & 0x400000 == 0x400000) {
        apply(ofpat_group_ingress);
    }

    if (openflow_metadata.bmap & 0x1 == 1) {
        apply(ofpat_output);
    }
}

control process_ofpat_egress {
    apply(ofpat_group_egress);

#ifdef OPENFLOW_ENABLE_MPLS
    if (openflow_metadata.bmap & 0x100000 == 0x100000) {
        apply(ofpat_pop_mpls);
    }

    if (openflow_metadata.bmap & 0x80000 == 0x80000) {
        apply(ofpat_push_mpls);
    }

    if (openflow_metadata.bmap & 0x10000 == 0x10000) {
        apply(ofpat_dec_mpls_ttl);
    }

    if (openflow_metadata.bmap & 0x8000 == 0x8000) {
        apply(ofpat_set_mpls_ttl);
    }
#endif /* OPENFLOW_ENABLE_MPLS */
#ifdef OPENFLOW_ENABLE_VLAN
    if (openflow_metadata.bmap & 0x40000 == 0x40000) {
        apply(ofpat_pop_vlan);
    }

    if (openflow_metadata.bmap & 0x20000 == 0x20000) {
        apply(ofpat_push_vlan);
    }

    if (openflow_metadata.bmap & 0x2000000 == 0x2000000) {
        apply(ofpat_set_field);
    }
#endif /* OPENFLOW_ENABLE_VLAN */
#ifdef OPENFLOW_ENABLE_L3
    if (openflow_metadata.bmap & 0x1000000 == 0x1000000) {
        if ((valid(ipv4))) {
            apply(ofpat_dec_nw_ttl_ipv4);
        } else {
            if ((valid(ipv6))) {
                apply(ofpat_dec_nw_ttl_ipv6);
            }
        }
    }

    if (openflow_metadata.bmap & 0x800000 == 0x800000) {
        if (valid(ipv4)) {
            apply(ofpat_set_nw_ttl_ipv4);
        } else {
            if (valid(ipv6)) {
                apply(ofpat_set_nw_ttl_ipv6);
            }
        }
    }
#endif /* OPENFLOW_ENABLE_L3 */

    // oq (set queue)
}

