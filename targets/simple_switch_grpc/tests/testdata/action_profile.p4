/* Copyright 2019-present Barefoot Networks, Inc.
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

header hdr_t {
    bit<8> in_;
    bit<32> entropy;
}

struct Headers {
    hdr_t hdr;
}

struct Meta {}

parser p(packet_in b, out Headers h,
         inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.hdr);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {}
}

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h); }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name(".ActProfWS")
    action_selector(HashAlgorithm.crc16, 32w128, 32w16) ActProfWS;

    @name(".send")
    action send(bit<9> eg_port) {
        sm.egress_spec = eg_port;
    }

    @name(".IndirectWS")
    table IndirectWS {
        key = {
            h.hdr.in_ : exact;
            h.hdr.entropy : selector;
        }
        actions = { send; }
        implementation = ActProfWS;
        size = 512;
    }

    apply {
        IndirectWS.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
