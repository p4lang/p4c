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

// Test case for Issue #216

#include <core.p4>
#include <v1model.p4>

#include <core.p4>
#include <v1model.p4>

// List of all recognized headers
struct Headers {}

struct Metadata {}

parser P(packet_in b,
         out Headers p,
         inout Metadata meta,
         inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

// match-action pipeline section

control Ing(inout Headers headers,
            inout Metadata meta,
            inout standard_metadata_t standard_meta) {
    apply {}
}

control Eg(inout Headers hdrs,
           inout Metadata meta,
           inout standard_metadata_t standard_meta) {

    action update(in bool p, inout bit<64> val) {
        bit<32> _sub = val[31:0];
        _sub = p ? _sub : 32w1;
        val[31:0] = _sub;
    }

    apply {
        bit<64> res = 0;
        update(true, res);
    }
}

// deparser section
control DP(packet_out b, in Headers p) {
    apply {}
}

// Fillers
control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {}
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {}
}

// Instantiate the top-level V1 Model package.
V1Switch(P(),
         Verify(),
         Ing(),
         Eg(),
         Compute(),
         DP()) main;
