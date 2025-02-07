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

#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>

/* Sample P4 program */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pri     : 3;
        cfi     : 1;
        vlan_id : 12;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags_resv : 1;
        flags_df : 1;
        flags_mf : 1;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type ingress_metadata_t {
    fields {
        ifid : 32; /* Logical Interface ID */
        brid : 16; /* Bridging Domain ID */
        vrf  : 16;
        l3   :  1; /* Set if routed */
    }
}

metadata ingress_metadata_t ing_md;
header ethernet_t ethernet;
header ipv4_t ipv4;
header vlan_tag_t vlan_tag;


parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100 : parse_vlan_tag;
        0x800 : parse_ipv4;
        default: ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.etherType) {
        0x800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

field_list ipv4_field_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags_resv;
    ipv4.flags_df;
    ipv4.flags_mf;
    ipv4.fragOffset;
    ipv4.ttl;
    ipv4.protocol;
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation ipv4_chksum_calc {
    input {
        ipv4_field_list;
    }
    algorithm : csum16;
    output_width: 16;
}

calculated_field ipv4.hdrChecksum {
    update ipv4_chksum_calc;
}







/******************************************************************************
 *
 * Table: ing_port
 * Key: Incoming physical port number
 *      Top VLAN tag
 *
 * Maps the physical port to a logical port and also invalidates the egress
 * unicast port and MGID intrinsic metadatas.
 *
 *****************************************************************************/
action set_ifid(ifid) {
    modify_field(ing_md.ifid, ifid);
    /* Set the destination port to an invalid value. */
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 0x1FF);
}

table ing_port {
    reads {
        ig_intr_md.ingress_port : exact;
        vlan_tag                : valid;
        vlan_tag.vlan_id        : exact;
    }
    actions {
        set_ifid;
    }
}


/******************************************************************************
 *
 * Table: ing_src_ifid
 * Key: Logical interface id
 *
 * Assigns ingress interface specific metadata: iRID, XID, YID, and brid.
 * Also sets the hash values used for multicast replication.
 *
 *****************************************************************************/
action set_src_ifid_md(rid, yid, brid, h1, h2) {
    modify_field(ig_intr_md_for_tm.rid, rid);
    modify_field(ig_intr_md_for_tm.level2_exclusion_id, yid);
    modify_field(ing_md.brid, brid);
    modify_field(ig_intr_md_for_tm.level1_mcast_hash, h1);
    modify_field(ig_intr_md_for_tm.level2_mcast_hash, h2);
}

table ing_src_ifid {
    reads {
        ing_md.ifid : exact;
    }
    actions {
        set_src_ifid_md;
    }
}


/******************************************************************************
 *
 * Table: ing_dmac
 * Key: Bridging domain and DMAC
 *
 * Will either setup the packet to L2 flood, L2 switch or route.
 *
 *****************************************************************************/
action flood() {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, ing_md.brid);
}
action switch(egr_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egr_port);
}
action route(vrf) {
    modify_field(ing_md.l3, 1);
    modify_field(ing_md.vrf, vrf);
}
table ing_dmac {
    reads {
        ing_md.brid      : exact;
        ethernet.dstAddr : exact;
    }
    actions {
        flood;
        switch;
        route;
    }
}


/******************************************************************************
 *
 * Table: ing_ipv4_mcast
 * Key: IPv4 addresses and VRF
 *
 * Will assign MGID and XID
 *
 *****************************************************************************/
action mcast_route(xid, mgid1, mgid2) {
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, xid);
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mgid1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid2);
    add_to_field(ipv4.ttl, -1);
}
table ing_ipv4_mcast {
    reads {
        ing_md.vrf   : exact;
        ipv4.srcAddr : ternary;
        ipv4.dstAddr : ternary;
    }
    actions {
        mcast_route;
    }
}


control ingress {
    apply(ing_port);
    apply(ing_src_ifid);
    apply(ing_dmac);
    if (ing_md.l3 == 1) {
        apply(ing_ipv4_mcast);
    }
}


/******************************************************************************
 *
 * Table: egr_encode
 * Key: None
 *
 * Encodes metadata generated by the multicast replication into the packet.
 *
 *****************************************************************************/
action do_egr_encode() {
    modify_field(ipv4.identification, eg_intr_md.egress_rid);
    modify_field(ipv4.diffserv,       eg_intr_md.egress_rid_first);
}
table egr_encode {
    actions {
        do_egr_encode;
    }
}


control egress {
    if (valid(ipv4)) {
        apply(egr_encode);
    }
}


