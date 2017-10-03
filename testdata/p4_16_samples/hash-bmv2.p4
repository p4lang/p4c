/*
Copyright 2016 VMware, Inc.

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
typedef standard_metadata_t std_meta_t;

header hash_t {
    bit<16> hash;
}

header ipv4_t {
    bit<32> lkp_ipv4_sa;
}

struct M {
    hash_t hash;
    ipv4_t ipv4;
}

struct H { }


parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start {
        transition accept;
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {

    action a() {
        hash(meta.hash.hash,
             HashAlgorithm.crc16,
             (bit<16>)0,
             { meta.ipv4.lkp_ipv4_sa },
             (bit<32>)65536);
    }

    apply {
        a();
    }
}


control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply { }
}

control DeparserI(packet_out b, in H hdr) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
