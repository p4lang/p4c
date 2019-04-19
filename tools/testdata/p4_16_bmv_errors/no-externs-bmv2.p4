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

#include <v1model.p4>

/** Test extern function. Set d <- s. Will cause compilation failure since
 *  default compilation is without --emit-extern. */
extern void extern_func(out bit<32> d, bit<32> s);

header h {
  bit<8> n;
}

struct s_h {
  h h;
}

struct m {
  bit<32> mf;
}

parser MyParser(packet_in b,
                out s_h parsedHdr,
                inout m meta,
                inout standard_metadata_t standard_metadata) {
  state start {
    b.extract(parsedHdr.h);
    transition accept;
  }
}

control MyVerifyChecksum(inout s_h hdr,
                       inout m meta) {
  apply {}

}
control MyIngress(inout s_h hdr,
                  inout m meta,
                  inout standard_metadata_t standard_metadata) {
  apply {
    extern_func(meta.mf, 32);
  }
}
control MyEgress(inout s_h hdr,
               inout m meta,
               inout standard_metadata_t standard_metadata) {
  apply {}
}

control MyComputeChecksum(inout s_h hdr,
                          inout m meta) {
  apply {}
}

control MyDeparser(packet_out b, in s_h hdr) {
  apply {}
}

V1Switch(MyParser(),
         MyVerifyChecksum(),
         MyIngress(),
         MyEgress(),
         MyComputeChecksum(),
         MyDeparser()) main;
