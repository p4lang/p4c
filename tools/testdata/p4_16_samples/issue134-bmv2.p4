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

header ipv4_t {
    bit<4> ihl;
    bit<4> version;
    bit<16> hdrChecksum;
}

struct H {
    ipv4_t ipv4;
}

struct M { };

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start { transition accept; }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {  }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply { }
}

control DeparserI(packet_out pk, in H hdr) {
    apply { }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
        // c16 invoked conditionally
        update_checksum(hdr.ipv4.ihl == 5, { 1w0 }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}


V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(),
         ComputeChecksumI(), DeparserI()) main;
