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

struct H { bit<32> x; };
struct M { };

parser ParserI(packet_in b, out H parsedHdr, inout M meta,
              inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}


control VerifyChecksumI(in H hdr,
                        inout M meta) {
    apply { }
}


control IngressI(inout H hdr,
                 inout M meta,
                 inout standard_metadata_t standard_metadata) {
    apply { }
}


control EgressI(inout H hdr,
                inout M meta,
                inout standard_metadata_t standard_metadata) {
    apply { }
}


control ComputeChecksumI(inout H hdr,
                         inout M meta) {
    apply { }
}


control DeparserI(packet_out b, in H hdr) {
    apply {
        b.emit(hdr);  // illegal data type
    }
}


V1Switch(ParserI(),
         VerifyChecksumI(),
         IngressI(),
         EgressI(),
         ComputeChecksumI(),
         DeparserI()) main;
