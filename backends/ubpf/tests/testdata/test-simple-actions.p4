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

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
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

control pipe(inout Headers_t headers, inout metadata meta) {

    action ip_modify_saddr(bit<32> srcAddr) {
        headers.ipv4.srcAddr = srcAddr;
    }

    action mpls_modify_tc(bit<3> tc) {
        headers.mpls.tc = tc;
    }

    action mpls_set_label(bit<20> label) {
        headers.mpls.label = label;
    }

    action mpls_set_label_tc(bit<20> label, bit<3> tc) {
        headers.mpls.label = label;
        headers.mpls.tc = tc;
    }

    action mpls_decrement_ttl() {
        headers.mpls.ttl = headers.mpls.ttl - 1;
    }

    action mpls_set_label_decrement_ttl(bit<20> label) {
            headers.mpls.label = label;
            mpls_decrement_ttl();
    }

    action mpls_modify_stack(bit<1> stack) {
        headers.mpls.stack = stack;
    }

    action change_ip_ver() {
        headers.ipv4.version = 6;
    }

    action ip_swap_addrs() {
        bit<32> tmp = headers.ipv4.dstAddr;
        headers.ipv4.dstAddr = headers.ipv4.srcAddr;
        headers.ipv4.srcAddr = tmp;
    }

    action Reject() {
        mark_to_drop();
    }

    table filter_tbl {
        key = {
            headers.ipv4.dstAddr : exact;
        }
        actions = {
            mpls_decrement_ttl;
            mpls_set_label;
            mpls_set_label_decrement_ttl;
            mpls_modify_tc;
            mpls_set_label_tc;
            mpls_modify_stack;
            change_ip_ver;
            ip_swap_addrs;
            ip_modify_saddr;
            Reject;
            NoAction;
        }

        const default_action = NoAction;
    }

    apply {
        filter_tbl.apply();
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
