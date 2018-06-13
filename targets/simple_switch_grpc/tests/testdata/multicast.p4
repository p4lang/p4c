/* Copyright 2018-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <core.p4>
#include <v1model.p4>

header Hdr {
    bit<16> f1;
}

struct Headers {
    Hdr hdr;
}

struct Meta { }

parser p(packet_in b, out Headers h,
         inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.hdr);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply { sm.mcast_grp = h.hdr.f1; }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply { h.hdr.f1 = sm.egress_rid; }
}

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.hdr); }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
