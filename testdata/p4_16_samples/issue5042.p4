/*
Copyright 2026 Deval Gupta

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

/*
 * Test for issue #5042: Directionless action parameters should accept
 * non-compile-time-known values when provided by the P4 program.
 * Per P4 Spec Section 6.8, such parameters behave as 'in' direction.
 */

#include <core.p4>
#include <v1model.p4>

header h_t {
    bit<8> f;
}

struct metadata_t {
    bit<8> x;
}

struct headers_t {
    h_t h;
}

parser p(packet_in pkt, out headers_t hdr, inout metadata_t meta,
         inout standard_metadata_t std_meta) {
    state start {
        pkt.extract(hdr.h);
        meta.x = hdr.h.f;
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta,
                inout standard_metadata_t std_meta) {
    // Action with directionless parameter 'data'
    action a(bit<8> data) {
        hdr.h.f = data;
    }

    table t {
        key = { hdr.h.f : exact; }
        actions = { a; }
    }

    apply {
        a(meta.x);
        t.apply();
    }
}

control egress(inout headers_t hdr, inout metadata_t meta,
               inout standard_metadata_t std_meta) {
    apply { }
}

control deparser(packet_out pkt, in headers_t hdr) {
    apply { pkt.emit(hdr.h); }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

V1Switch(p(), verifyChecksum(), ingress(), egress(),
         computeChecksum(), deparser()) main;
