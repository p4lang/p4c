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

typedef standard_metadata_t std_meta_t;

struct some_meta_t { bool flag; }

struct H { }

struct M { some_meta_t some_meta; }

control DeparserI(packet_out packet, in H hdr) {
    apply { }
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start { transition accept; }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
        meta.some_meta.flag = true;
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
