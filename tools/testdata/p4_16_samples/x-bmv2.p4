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

struct S { bit<32> x; }
header T { bit<32> y; }
struct H { T s; }
struct M { S s; }

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

parser ParserI(packet_in b, out H parsedHdr, inout M meta, inout std_meta_t std_meta) {
    state start { transition accept; }
}

control ctrl(inout M meta) {
    apply { }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    ctrl() do_ctrl;

    apply {
        do_ctrl.apply(meta);
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}
control DeparserI(packet_out b, in H hdr) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(),
         DeparserI()) main;
