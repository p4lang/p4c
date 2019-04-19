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

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
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

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
    }
}

header_type icmp_t {
    fields {
        typeCode : 16;
        hdrChecksum : 16;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header ethernet_t ethernet;
header vlan_tag_t vlan_tag;
header ipv4_t     ipv4;
header ipv6_t     ipv6;
header tcp_t      tcp;
header udp_t      udp;
header icmp_t     icmp;

parser start {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100, 0x9100 : parse_vlan_tag;
        0x0800         : parse_ipv4;
        0x86DD         : parse_ipv6;
        default        : ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.etherType) {
        0x0800         : parse_ipv4;
        0x86DD         : parse_ipv6;
        default        : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.ihl, latest.protocol) {
        0x0000501 mask 0x0000fff : parse_icmp;
        0x0000506 mask 0x0000fff : parse_tcp;
        0x0000511 mask 0x0000fff : parse_udp;
        default                  : ingress;
    }
}

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        0x01 : parse_icmp;
        0x06 : parse_tcp;
        0x11 : parse_udp;
        default : ingress;
    }
}

parser parse_icmp {
    extract(icmp);
    return ingress;
}

parser parse_tcp {
    extract(tcp);
    return ingress;
}

parser parse_udp {
    extract(udp);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop : 1;
        egress_port : 9;
        packet_type : 4;
    }
}

metadata ingress_metadata_t ing_metadata;

action nop() {
}
action _drop() {
    modify_field(ing_metadata.drop, 1);
}
action discard() {
    drop();
}

action set_egress_port(egress_port) {
    modify_field(ing_metadata.egress_port, egress_port);
}

action l2_packet() {
    modify_field(ing_metadata.packet_type, 0);
}

action ipv4_packet() {
    modify_field(ing_metadata.packet_type, 1);
}

action ipv6_packet() {
    modify_field(ing_metadata.packet_type, 2);
}

action mpls_packet() {
    modify_field(ing_metadata.packet_type, 3);
}

action mim_packet() {
    modify_field(ing_metadata.packet_type, 4);
}

table ethertype_match {
    reads {
        ethernet.etherType : exact;
    }
    actions {
        l2_packet;
        ipv4_packet;
        ipv6_packet;
        mpls_packet;
        mim_packet;
    }
}

table ipv4_match {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

table ipv6_match {
    reads {
        ipv6.dstAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

table l2_match {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

table tcp_check {
    reads {
        tcp.dstPort : exact;
    }
    actions {
        nop;
        _drop;
    }
}
table udp_check {
    reads {
        udp.dstPort : exact;
    }
    actions {
        nop;
        _drop;
    }
}
table icmp_check {
    reads {
        icmp.typeCode : exact;
    }
    actions {
        nop;
        _drop;
    }
}

action send_packet() {
    modify_field(standard_metadata.egress_spec, ing_metadata.egress_port);
}

table set_egress {
    reads {
        ing_metadata.drop : exact;
    }
    actions {
        discard;
        send_packet;
    }
}

control ingress {
    apply(ethertype_match) {
        ipv4_packet {
            apply(ipv4_match);
        }
        ipv6_packet, mpls_packet {
            apply(ipv6_match);
        }
        default {
          apply(l2_match);
        }
    }
    if (valid(tcp)) {
        apply(tcp_check); }
    else if (valid(udp)) {
        apply(udp_check); }
    else if (valid(icmp)) {
        apply(icmp_check); }
    apply(set_egress);
}

control egress {
}
