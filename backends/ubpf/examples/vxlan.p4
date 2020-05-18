/*
Copyright 2019 Orange

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

#define UBPF_MODEL_VERSION 20200304
#include <ubpf_model.p4>
#include <core.p4>

const bit<16> UDP_PORT_VXLAN  = 4789;
const bit<8>  UDP_PROTO = 17;
const bit<16> IPV4_ETHTYPE = 0x800;
const bit<16> ETH_HDR_SIZE = 14;
const bit<16> IPV4_HDR_SIZE = 20;
const bit<16> UDP_HDR_SIZE = 8;
const bit<16> VXLAN_HDR_SIZE = 8;
const bit<4>  IP_VERSION_4 = 4;
const bit<4>  IPV4_MIN_IHL = 5;

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

// IPv4 header without options
header IPv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length;
    bit<16> checksum;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
    udp_t      udp;
    vxlan_t    vxlan;

    Ethernet_h inner_ethernet;
    IPv4_h     inner_ipv4;
}

struct metadata {
    bit<24> vxlan_vni;
    bit<32> nexthop;
    bit<32> vtepIP;
}

parser prs(packet_in packet, out Headers_t hdr, inout metadata meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            IPV4_ETHTYPE: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            UDP_PROTO: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition select(hdr.udp.dstPort) {
            UDP_PORT_VXLAN: parse_vxlan;
            default: accept;
         }
    }
    state parse_vxlan {
        packet.extract(hdr.vxlan);
        transition parse_inner_ethernet;
    }
    state parse_inner_ethernet {
        packet.extract(hdr.inner_ethernet);
        transition select(hdr.ethernet.etherType) {
            IPV4_ETHTYPE: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        transition accept;
    }
}


control pipe(inout Headers_t hdr, inout metadata meta) {

    action vxlan_decap() {
        // as simple as set outer headers as invalid
        hdr.ethernet.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.udp.setInvalid();
        hdr.vxlan.setInvalid();
    }

    table upstream_tbl {
        key = {
            hdr.vxlan.vni : exact;
        }
        actions = {
            vxlan_decap;
            NoAction;
        }

    }

    action vxlan_encap() {
        hdr.inner_ethernet = hdr.ethernet;
        hdr.inner_ipv4 = hdr.ipv4;

        hdr.ethernet.setValid();

        hdr.ipv4.setValid();
        hdr.ipv4.version = IP_VERSION_4;
        hdr.ipv4.ihl = IPV4_MIN_IHL;
        hdr.ipv4.diffserv = 0;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen
                            + (ETH_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + VXLAN_HDR_SIZE);
        hdr.ipv4.identification = 0x1513; /* From NGIC */
        hdr.ipv4.flags = 0;
        hdr.ipv4.fragOffset = 0;
        hdr.ipv4.ttl = 64;
        hdr.ipv4.protocol = UDP_PROTO;
        hdr.ipv4.dstAddr = hdr.inner_ipv4.dstAddr;
        hdr.ipv4.srcAddr = hdr.inner_ipv4.srcAddr;
        hdr.ipv4.hdrChecksum = 0;

        hdr.udp.setValid();

        hdr.udp.srcPort = 15221; // random port
        hdr.udp.dstPort = UDP_PORT_VXLAN;
        hdr.udp.length = hdr.ipv4.totalLen + (UDP_HDR_SIZE + VXLAN_HDR_SIZE);
        hdr.udp.checksum = 0;

        hdr.vxlan.setValid();
        hdr.vxlan.reserved = 0;
        hdr.vxlan.reserved_2 = 0;
        hdr.vxlan.flags = 0;
        hdr.vxlan.vni = 25;

    }

    table downstream_tbl {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            vxlan_encap;
            NoAction;
        }

    }

    apply {
        if (hdr.vxlan.isValid()) {
            upstream_tbl.apply();
        } else {
            if (hdr.ipv4.isValid()) {
                downstream_tbl.apply();
            }
        }

    }
}

control dprs(packet_out packet, in Headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.vxlan);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;