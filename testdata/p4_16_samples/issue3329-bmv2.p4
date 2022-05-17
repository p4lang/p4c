/*
Copyright 2019 Cisco Systems, Inc.

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

struct s {
    bit<32> b;
}

@controller_header("packet_out")
header packet_out_header_t {
    s b;
}

struct headers_t {
    packet_out_header_t packet_out;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    state start {
        transition accept;
    }
}

control ingress(inout headers_t hdr,
                inout metadata_t meta,
                inout standard_metadata_t stdmeta)
{
    apply {
    }
}

control egress(inout headers_t hdr,
               inout metadata_t meta,
               inout standard_metadata_t stdmeta)
{
    apply { }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
