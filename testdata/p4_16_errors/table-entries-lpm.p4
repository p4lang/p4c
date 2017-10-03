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

#define ENABLE_NEGATIVE_TESTS 1

header hdr {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8> r;
    bit<1>  v;
}

struct Header_t {
    hdr h;
}
struct Meta_t {}

parser p(packet_in b, out Header_t h, inout Meta_t m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Header_t h, inout Meta_t m) { apply {} }
control update(inout Header_t h, inout Meta_t m) { apply {} }
control egress(inout Header_t h, inout Meta_t m, inout standard_metadata_t sm) { apply {} }
control deparser(packet_out b, in Header_t h) { apply { b.emit(h.h); } }

control ingress(inout Header_t h, inout Meta_t m, inout standard_metadata_t standard_meta) {

    action a() { standard_meta.egress_spec = 0; }
    action a_with_control_params(bit<9> x) { standard_meta.egress_spec = x; }

    table t_lpm {

  	key = {
            h.h.l : lpm;
        }

	actions = {
            a;
            a_with_control_params;
        }

	default_action = a;

        const entries = {
            0x11 &&& 0xF0 : a_with_control_params(11);
            0x12          : a_with_control_params(12);
            _             : a_with_control_params(13);
            #ifdef ENABLE_NEGATIVE_TESTS
            // negative tests:
            0x11 &&& 0x0F: a_with_control_params(14); // invalid mask
            (0x1, 0x11 &&& 0xF0): a(); // invalid keyset
            #endif
        }
    }

    apply {
        t_lpm.apply();
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
