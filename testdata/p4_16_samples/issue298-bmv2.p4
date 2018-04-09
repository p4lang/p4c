/*
Copyright 2017 VMware, Inc.

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

#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<4> PortId;

const PortId DROP_PORT = 0xF;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3> flags;
    bit<13> fragOffset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}


#define MSGTYPE_SIZE    16
#define INSTANCE_SIZE   32
#define ROUND_SIZE      16
#define INSTANCE_COUNT  65536


header myhdr_t {
    bit<MSGTYPE_SIZE>   msgtype;
    bit<INSTANCE_SIZE>  inst;
    bit<ROUND_SIZE>     rnd;
}

struct headers {
    @name("ethernet")
    ethernet_t ethernet;
    @name("ipv4")
    ipv4_t ipv4;
    @name("udp")
    udp_t udp;
    @name("myhdr")
    myhdr_t myhdr;
}

struct ingress_metadata_t {
    bit<ROUND_SIZE> round;
    bit<1> set_drop;
}

struct metadata {
    @name("ingress_metadata")
    ingress_metadata_t   local_metadata;
}

#define ETHERTYPE_IPV4 16w0x0800
#define UDP_PROTOCOL 8w0x11
#define myhdr_PROTOCOL 16w0x8888

parser TopParser(packet_in b, out headers p, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
        }
    }

    state parse_ipv4 {
        b.extract(p.ipv4);
        transition select(p.ipv4.protocol) {
            UDP_PROTOCOL : parse_udp;
            default : accept;
        }
    }

    state parse_udp {
        b.extract(p.udp);
        transition select(p.udp.dstPort) {
            myhdr_PROTOCOL : parse_myhdr;
            default : accept;
        }
    }

    state parse_myhdr {
        b.extract(p.myhdr);
        transition accept;
    }
}

control TopDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.myhdr);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum(hdr.ipv4.isValid(), {hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.ipv4.isValid(), {hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {

    action _drop() {
        mark_to_drop();
    }

    table drop_tbl {
        key = { meta.local_metadata.set_drop : exact; }
        actions = {
            _drop;
             NoAction;
        }
        size = 2;
        default_action =  NoAction();
    }

    apply {
      drop_tbl.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    register<bit<ROUND_SIZE>>(INSTANCE_COUNT) registerRound;

    action read_round() {
        registerRound.read(meta.local_metadata.round, hdr.myhdr.inst);
    }

    table round_tbl {
        key = {}
        actions = {
            read_round;
        }
        size = 8;
        default_action = read_round;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            if (hdr.myhdr.isValid()) {
                round_tbl.apply();
            }
        }
    }
}

V1Switch(TopParser(), verifyChecksum(), ingress(), egress(), computeChecksum(), TopDeparser()) main;
