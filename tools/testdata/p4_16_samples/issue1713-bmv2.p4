/*
Copyright 2019-present Barefoot Networks, Inc.
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
    bit<16> a;
    bit<16> b;
    bit<8> c;
}

struct Headers {
    hdr h;
}

struct Meta {
    bit<8> x;
    bit<8> y;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action case0() { h.h.c = ((bit<12>)h.h.a)[11:4]; }
    action case1() { h.h.c = (bit<8>)(h.h.a[14:3][8:4]); }
    action case2() { h.h.c = (bit<8>)((16w0 ++ h.h.a)[31:8]); }
    action case3() { h.h.c = (bit<8>)((int<32>)(int<16>)h.h.a)[14:2][10:3]; }
    table t {
        actions = {
	  case0;
	  case1;
	  case2;
	  case3;
	}
        const default_action = case0;
    }
    apply { t.apply(); }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.h); }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
