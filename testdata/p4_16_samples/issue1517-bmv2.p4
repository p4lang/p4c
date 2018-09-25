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

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet,
                  out headers_t hdr,
                  inout meta_t meta,
                  inout standard_metadata_t standard_metadata)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr,
                inout meta_t meta,
                inout standard_metadata_t standard_metadata)
{
    apply {
        bit<16> rand_int;

        // Compiler Bug: typechecker mutated program with this line
        // and latest version of compiler as of 2018-Sep-24:
        random<bit<16>>(rand_int, 0, 48*1024-1);

        // No error with this line:
        //random<bit<16>>(rand_int, 0, (bit<16>) 48*1024-1);

        if (rand_int < 32*1024) {
            mark_to_drop();
        }
    }
}

control egress(inout headers_t hdr,
               inout meta_t meta,
               inout standard_metadata_t standard_metadata)
{
    apply { }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply { }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply { }
}

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
