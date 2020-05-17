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

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

const bit<16> UDP_PORT_GTP  = 2152;
const bit<8> UDP_PROTO      = 17;
const bit<16> IPV4_ETHTYPE  = 0x800;
const bit<16> ETH_HDR_SIZE  = 14;
const bit<16> IPV4_HDR_SIZE = 20;
const bit<16> UDP_HDR_SIZE  = 8;
const bit<16> GTP_HDR_SIZE  = 8;
const bit<4> IP_VERSION_4   = 4;
const bit<4> IPV4_MIN_IHL   = 5;

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

header gtp_t {
    bit<3>  version;
    bit<1>  ptFlag;
    bit<1>  spare;
    bit<1>  extHdrFlag;
    bit<1>  seqNumberFlag;
    bit<1>  npduFlag;
    bit<8>  msgType;
    bit<16> len;
    bit<32> tunnelEndID;
}


struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
    udp_t      udp;
    gtp_t      gtp;
    IPv4_h     inner_ipv4;
}

struct metadata {}

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
            UDP_PORT_GTP: parse_gtp;
            default: accept;
         }
    }
    state parse_gtp {
        packet.extract(hdr.gtp);
        transition parse_inner_ipv4;
    }

    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t hdr, inout metadata meta) {

    action gtp_decap() {
        // as simple as set outer headers as invalid
        hdr.ipv4.setInvalid();
        hdr.udp.setInvalid();
        hdr.gtp.setInvalid();
    }

    table upstream_tbl {
        key = {
            hdr.gtp.tunnelEndID : exact;
        }
        actions = {
            gtp_decap;
            NoAction;
        }
    }

    action gtp_encap(bit<32> tunnelId) {
        hdr.inner_ipv4 = hdr.ipv4;

        hdr.ipv4.setValid();
        hdr.ipv4.version = IP_VERSION_4;
        hdr.ipv4.ihl = IPV4_MIN_IHL;
        hdr.ipv4.diffserv = 0;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen
                            + (IPV4_HDR_SIZE + UDP_HDR_SIZE + GTP_HDR_SIZE);
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
        hdr.udp.dstPort = UDP_PORT_GTP;
        hdr.udp.length = hdr.inner_ipv4.totalLen + (UDP_HDR_SIZE + GTP_HDR_SIZE);
        hdr.udp.checksum = 0;

        hdr.gtp.setValid();
        hdr.gtp.tunnelEndID = tunnelId;
        hdr.gtp.version = 0x01;
        hdr.gtp.ptFlag = 0x01;
        hdr.gtp.spare =0;
        hdr.gtp.extHdrFlag =0;
        hdr.gtp.seqNumberFlag =0;
        hdr.gtp.npduFlag = 0;
        hdr.gtp.msgType = 0xff;
        hdr.gtp.len = hdr.inner_ipv4.totalLen;

    }


    table downstream_tbl {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            gtp_encap;
            NoAction;
        }
    }


    apply {
        if (hdr.gtp.isValid()) {
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
        packet.emit(hdr.gtp);
        packet.emit(hdr.inner_ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;