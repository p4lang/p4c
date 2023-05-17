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

#include <ubpf_model.p4>
#include <core.p4>

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

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

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ipv4;
            default : reject;
        }
    }

    state ipv4 {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action Reject() {
        mark_to_drop();
    }

    action set_ipv4_version(bit<4> version) {
        headers.ipv4.version = version;
    }

    action set_ihl(bit<4> ihl) {
        headers.ipv4.ihl = ihl;
    }

    action set_diffserv(bit<8> diffserv) {
        headers.ipv4.diffserv = diffserv;
    }

    action set_identification(bit<16> identification) {
        headers.ipv4.identification = identification;
    }

    action set_flags(bit<3> flags) {
        headers.ipv4.flags = flags;
    }

    action set_fragOffset(bit<13> fragOffset) {
        headers.ipv4.fragOffset = fragOffset;
    }

    action set_ttl(bit<8> ttl) {
        headers.ipv4.ttl = ttl;
    }

    action set_protocol(bit<8> protocol) {
        headers.ipv4.protocol = protocol;
    }

    action set_srcAddr(bit<32> srcAddr) {
        headers.ipv4.srcAddr = srcAddr;
    }

    action set_dstAddr(bit<32> dstAddr) {
        headers.ipv4.dstAddr = dstAddr;
    }

    action set_srcAddr_dstAddr(bit<32> srcAddr, bit<32> dstAddr) {
        headers.ipv4.srcAddr = srcAddr;
        headers.ipv4.dstAddr = dstAddr;
    }

    action set_ihl_diffserv(bit<4> ihl, bit<8> diffserv) {
        headers.ipv4.ihl = ihl;
        headers.ipv4.diffserv = diffserv;
    }

    action set_fragOffset_flags(bit<13> fragOffset, bit<3> flags) {
        headers.ipv4.flags = flags;
        headers.ipv4.fragOffset = fragOffset;
    }

    action set_flags_ttl(bit<3> flags, bit<8> ttl) {
        headers.ipv4.flags = flags;
        headers.ipv4.ttl = ttl;
    }

    action set_fragOffset_srcAddr(bit<13> fragOffset, bit<32> srcAddr) {
        headers.ipv4.fragOffset = fragOffset;
        headers.ipv4.srcAddr = srcAddr;
    }

    table filter_tbl {
        key = {
            headers.ipv4.srcAddr : exact;
        }
        actions = {
            set_ipv4_version;
            set_ihl;
            set_diffserv;
            set_identification;
            set_flags;
            set_fragOffset;
            set_ttl;
            set_protocol;
            set_srcAddr;
            set_dstAddr;
            set_srcAddr_dstAddr;
            set_ihl_diffserv;
            set_fragOffset_flags;
            set_flags_ttl;
            set_fragOffset_srcAddr;
            Reject;
            NoAction;
        }
    }

    apply {
        filter_tbl.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}


ubpf(prs(), pipe(), dprs()) main;
