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

typedef bit<8> USat_t;
typedef int<16> Sat_t;

/// Test saturating arithmetic
/// The header specifies the operation:
/// op = 1 - usat_plus, 2 - usat_minus, 3 - sat_plus, 4 - sat_minus
/// oprX_8 - unsigned 8-bit operands, res_8 contains the result
/// oprX_16 - signed 16-bit operands, res_16 contains the result
header hdr {
    bit<8>  op;
    USat_t  opr1_8;
    USat_t  opr2_8;
    USat_t  res_8;
    Sat_t opr1_16;
    Sat_t opr2_16;
    Sat_t res_16;
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

    action usat_plus() {
        standard_meta.egress_spec = 0; // send to port 0
        h.h.res_8 = h.h.opr1_8 |+| h.h.opr2_8;
    }
    action usat_minus() {
        standard_meta.egress_spec = 0; // send to port 0
        h.h.res_8 = h.h.opr1_8 |-| h.h.opr2_8;
    }
    action sat_plus() {
        standard_meta.egress_spec = 0; // send to port 0
        h.h.res_16 = h.h.opr1_16 |+| h.h.opr2_16;
    }
    action sat_minus() {
        standard_meta.egress_spec = 0; // send to port 0
        h.h.res_16 = h.h.opr1_16 |-| h.h.opr2_16;
    }

    action drop() { mark_to_drop(standard_meta); }

    USat_t ru;
    Sat_t r;

    table t {

  	key = {
            h.h.op : exact;
        }

	actions = {
            usat_plus;
            usat_minus;
            sat_plus;
            sat_minus;
            drop;
        }

	default_action = drop;

        const entries = {
            0x01 : usat_plus();
            0x02 : usat_minus();
            0x03 : sat_plus();
            0x04 : sat_minus();
        }
    }

    apply {
        t.apply();
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
