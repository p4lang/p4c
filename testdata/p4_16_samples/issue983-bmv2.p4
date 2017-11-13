/*
Copyright 2017 Cisco Systems, Inc.

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


typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct fwd_meta_t {
    bit<16> tmp;
    bit<32> x1;
    bit<16> x2;
    bit<32> x3;
    bit<32> x4;
    bit<16> exp_etherType;
    bit<32> exp_x1;
    bit<16> exp_x2;
    bit<32> exp_x3;
    bit<32> exp_x4;
}

struct metadata {
    fwd_meta_t fwd_meta;
}

struct headers {
    ethernet_t       ethernet;
}


parser IngressParserImpl(packet_in buffer,
                         out headers hdr,
                         inout metadata user_meta,
                         inout standard_metadata_t standard_metadata)
{
    state start {
        buffer.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata user_meta,
                inout standard_metadata_t standard_metadata) {
    table debug_table_cksum1 {
        key = {
            hdr.ethernet.srcAddr : exact;
            hdr.ethernet.dstAddr : exact;
            hdr.ethernet.etherType : exact;
            user_meta.fwd_meta.exp_etherType : exact;
            user_meta.fwd_meta.tmp : exact;
            user_meta.fwd_meta.exp_x1 : exact;
            user_meta.fwd_meta.x1 : exact;
            user_meta.fwd_meta.exp_x2 : exact;
            user_meta.fwd_meta.x2 : exact;
            user_meta.fwd_meta.exp_x3 : exact;
            user_meta.fwd_meta.x3 : exact;
            user_meta.fwd_meta.exp_x4 : exact;
            user_meta.fwd_meta.x4 : exact;
        }
        actions = { NoAction; }
    }
    bit<16> tmp;
    bit<32> x1;
    bit<16> x2;
    apply {
        tmp = ~hdr.ethernet.etherType;

        x1 = (bit<32>) tmp;
        x2 = x1[31:16] + x1[15:0];

        user_meta.fwd_meta.tmp = tmp;
        user_meta.fwd_meta.x1 = x1;
        user_meta.fwd_meta.x2 = x2;
        user_meta.fwd_meta.x3 = (bit<32>) ~hdr.ethernet.etherType;
        user_meta.fwd_meta.x4 = ~((bit<32>) hdr.ethernet.etherType);

        user_meta.fwd_meta.exp_etherType = 0x0800;
        user_meta.fwd_meta.exp_x1 = (bit<32>) (~16w0x0800);
        user_meta.fwd_meta.exp_x2 = (~16w0x0800);
        user_meta.fwd_meta.exp_x3 = (bit<32>) (~16w0x0800);
        user_meta.fwd_meta.exp_x4 = (~32w0x0800);

        // Use dstAddr field of Ethernet header to store 'error
        // codes', so they are easily checkable in output packet.
        hdr.ethernet.dstAddr = 0;
        if (hdr.ethernet.etherType != user_meta.fwd_meta.exp_etherType) {
            hdr.ethernet.dstAddr[47:40] = 1;
        }
        if (user_meta.fwd_meta.x1 != user_meta.fwd_meta.exp_x1) {
            hdr.ethernet.dstAddr[39:32] = 1;
        }
        if (user_meta.fwd_meta.x2 != user_meta.fwd_meta.exp_x2) {
            hdr.ethernet.dstAddr[31:24] = 1;
        }
        if (user_meta.fwd_meta.x3 != user_meta.fwd_meta.exp_x3) {
            hdr.ethernet.dstAddr[23:16] = 1;
        }
        if (user_meta.fwd_meta.x4 != user_meta.fwd_meta.exp_x4) {
            hdr.ethernet.dstAddr[15:8] = 1;
        }
        debug_table_cksum1.apply();
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               inout standard_metadata_t standard_metadata)
{
    apply { }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

V1Switch(IngressParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
