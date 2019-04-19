/*
Copyright 2013-present Barefoot Networks, Inc.

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

// Test free-form annotations.
@scrabble(
    - What do you get if you multiply six by nine?
    - Six by nine. Forty two.
    - That's it. That's all there is.
    - I always thought there was something fundamentally wrong with the
      universe.
    0xdeadbeef
)
header hdr {
    bit<112> field;
}

struct Header_t {
    hdr h;
}

struct Meta_t {}

parser p(packet_in b,
         out Header_t h,
         inout Meta_t m,
         inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control c(inout Header_t h, inout Meta_t m) { apply {} }

control ingress(inout Header_t h,
                inout Meta_t m,
                inout standard_metadata_t standard_meta) {
    apply {}
}

control egress (inout Header_t h,
                inout Meta_t m,
                inout standard_metadata_t sm) {
    apply {}
}

control deparser(packet_out b, in Header_t h) { apply {} }

V1Switch(p(), c(), ingress(), egress(), c(), deparser()) main;
