/*
Copyright 2021-present Intel Corporation

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

header hdr {
    bit<32> f1;
    bit<32> f2;
}

struct Headers {
    hdr[3] hs;
}

struct Meta {
    bit<32> v;
}

parser p(packet_in b, out Headers h,
         inout Meta m, inout standard_metadata_t sm)
{
    state start {
        b.extract(h.hs.next);
        m.v = h.hs.last.f2;
        m.v = m.v + h.hs.last.f2;
        transition select(h.hs.last.f1) {
            0: start;
            _: accept;
        }
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {}
}

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.hs); }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        // Compiler should give error for out of bounds index,
        // if due to compile-time known value index.
        h.hs[-1].setValid();
        h.hs[-1].f1 = 5;
        h.hs[-1].f2 = 8;

        // No error or warning for these -- they are good.
        h.hs[0].setValid();
        h.hs[0].f1 = 5;
        h.hs[0].f2 = 8;
        h.hs[2].setValid();
        h.hs[2].f1 = 5;
        h.hs[2].f2 = 8;

        // Compiler should give error for out of bounds index,
        // if due to compile-time known value index.
        h.hs[3].setValid();
        h.hs[3].f1 = 5;
        h.hs[3].f2 = 8;
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
