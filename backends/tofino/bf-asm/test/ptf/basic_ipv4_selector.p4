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
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        hdr_length : 16;
        checksum : 16;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100 : parse_vlan_tag;
        0x800 : parse_ipv4;
        default: ingress;
    }
}

#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        default: ingress;
    }
}

header vlan_tag_t vlan_tag;

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.etherType) {
        0x800 : parse_ipv4;
        default : ingress;
    }
}

header_type md_t {
    fields {
        sport:16;
        dport:16;
    }
}

metadata md_t md;

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    set_metadata(md.sport, latest.srcPort);
    set_metadata(md.dport, latest.dstPort);
    return ingress;
}

header udp_t udp;

parser parse_udp {
    extract(udp);
    set_metadata(md.sport, latest.srcPort);
    set_metadata(md.dport, latest.dstPort);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        drop: 1;
        learn_meta_1:20;
        learn_meta_2:24;
        learn_meta_3:25;
        learn_meta_4:10;
    }
}

metadata routing_metadata_t /*metadata*/ routing_metadata;

header_type range_metadata_t {
    fields {
        src_range_index : 16;
        dest_range_index : 16;
    }
}

metadata range_metadata_t range_mdata;

field_list ipv4_field_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags;
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

action udp_set_src(port) {
    modify_field(udp.srcPort, port);
}

action nop() {
}

@pragma command_line --no-dead-code-elimination
table ipv4_routing_select {
    reads {
            ipv4.dstAddr: lpm;
    }
    action_profile : ecmp_action_profile;
    size : 512;
}

field_list ecmp_hash_fields {
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.identification;
    ipv4.protocol;
}

field_list_calculation ecmp_hash {
    input {
        ecmp_hash_fields;
    }
#if defined(BMV2TOFINO)
    algorithm : xxh64;
#else
    algorithm : random;
#endif
    output_width : 64;
}

action_profile ecmp_action_profile {
    actions {
        nhop_set;
        nop;
    }
    size : 1024;
    // optional
    dynamic_action_selection : ecmp_selector;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash; // take a field_list_calculation only
    // optional
    selection_mode : resilient; // “resilient” or “non-resilient”
}

action nhop_set(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

table eg_udp {
    reads {
        ethernet    : valid;
        ipv4        : valid;
        udp         : valid;
        udp.srcPort : exact;
    }
    actions {
      nop;
      udp_set_src;
    }
}


control ingress {
    apply(ipv4_routing_select);
}

control egress {
    apply(eg_udp);
}

