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

header mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  stack;
    bit<8>  ttl;
}

struct Headers_t
{
    Ethernet_h ethernet;
    mpls_h     mpls;
    IPv4_h     ipv4;
}

struct metadata {}


parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ipv4;
            0x8847   : mpls;
            default : reject;
        }
    }

    state mpls {
            p.extract(headers.mpls);
            transition ipv4;
    }

    state ipv4 {
        p.extract(headers.ipv4);
        transition accept;
    }


}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action mpls_encap() {
        headers.mpls.setValid();
        headers.ethernet.etherType = 0x8847;
        headers.mpls.label = 20;
        headers.mpls.tc = 5;
        headers.mpls.stack = 1;
        headers.mpls.ttl = 64;
    }

    action mpls_decap() {
        headers.mpls.setInvalid();
        headers.ethernet.etherType = 0x0800;
    }

    table upstream_tbl {
        key = {
            headers.mpls.label : exact;
        }
        actions = {
            mpls_decap();
            NoAction;
        }
    }

    table downstream_tbl {
        key = {
            headers.ipv4.dstAddr : exact;
        }
        actions = {
            mpls_encap;
            NoAction;
        }
    }

    apply {
        if (headers.mpls.isValid()) {
            upstream_tbl.apply();
        } else {
            downstream_tbl.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.mpls);
        packet.emit(headers.ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;