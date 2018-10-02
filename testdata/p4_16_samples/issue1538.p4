/*
Copyright 2018 Cisco Systems, Inc.

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


bit<16> incr(in bit<16> x)
{
    return x + 1;
}

bit<16> twoxplus1(in bit<16> x)
{
    return x + incr(x);
}


struct metadata {
    bit<16> tmp_port;
}


header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

action my_drop() {
    mark_to_drop();
}

parser ParserImpl(packet_in packet,
                  out headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata)
{
    bit<16> tmp_port = incr((bit<16>) standard_metadata.ingress_port);

    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        hdr.ethernet.etherType = incr(hdr.ethernet.etherType);
        meta.tmp_port = tmp_port;
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata)
{
    action set_port(bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    table mac_da {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            set_port;
            my_drop;
        }
        default_action = my_drop;
    }
    apply {
        mac_da.apply();
        // Any one of these 3 lines by itself causes errors with
        // p4test or p4c if uncommented.
        hdr.ethernet.srcAddr[15:0] = twoxplus1(hdr.ethernet.srcAddr[15:0]);
        hdr.ethernet.srcAddr[15:0] = incr(hdr.ethernet.srcAddr[15:0]);
        hdr.ethernet.etherType = incr(hdr.ethernet.etherType);
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               inout standard_metadata_t standard_metadata)
{
    apply {
    }
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

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
