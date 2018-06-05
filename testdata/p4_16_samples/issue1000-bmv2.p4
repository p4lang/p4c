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

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    bit<16> transition_taken;
}

struct headers {
    ethernet_t    ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.dstAddr[47:40],
                          hdr.ethernet.dstAddr[39:32],
                          hdr.ethernet.dstAddr[31:24]) {
            (0xca, 0xfe, 0xad): a7;
            (0xca, 0xfe,    _): a6;
            (0xca,    _, 0xad): a5;
            (   _, 0xfe, 0xad): a3;
            (0xca,    _,    _): a4;
            (   _, 0xfe,    _): a2;
            (   _,    _, 0xad): a1;
            (   _,    _,    _): a0;
        }
    }
    state a0 {
        meta.transition_taken = 0xa0;
        transition accept;
    }
    state a1 {
        meta.transition_taken = 0xa1;
        transition accept;
    }
    state a2 {
        meta.transition_taken = 0xa2;
        transition accept;
    }
    state a3 {
        meta.transition_taken = 0xa3;
        transition accept;
    }
    state a4 {
        meta.transition_taken = 0xa4;
        transition accept;
    }
    state a5 {
        meta.transition_taken = 0xa5;
        transition accept;
    }
    state a6 {
        meta.transition_taken = 0xa6;
        transition accept;
    }
    state a7 {
        meta.transition_taken = 0xa7;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        hdr.ethernet.etherType = meta.transition_taken;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
