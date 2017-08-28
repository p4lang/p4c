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

header H {
    varbit<32> var;
}

struct Parsed_packet {
    H h;
}

struct Metadata {
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit(hdr);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        // only parse 16 bits
        pkt.extract(hdr.h, 16);
        transition accept;
    }
}

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {}
}

control egress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {}
}

control vc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {}
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {}
}

V1Switch(parserI(), vc(), ingress(), egress(), uc(), DeparserI()) main;
