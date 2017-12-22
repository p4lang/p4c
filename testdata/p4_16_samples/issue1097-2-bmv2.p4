/*
Copyright 2017 Cisco Systems, Inc.

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

header myhdr_t {
    bit<8> reg_idx_to_update;
    bit<8> value_to_add;
    bit<8> debug_last_reg_value_written;
}

struct Headers {
    myhdr_t myhdr;
}

struct Meta {}

parser p(packet_in b,
    out Headers h,
    inout Meta m,
    inout standard_metadata_t sm)
{
    state start {
        b.extract(h.myhdr);
        transition accept;
    }
}

register<bit<8>>(256) r;

control ingress(inout Headers h,
    inout Meta m,
    inout standard_metadata_t sm)
{
    apply {
        bit<8> x;
        r.read(x, (bit<32>) h.myhdr.reg_idx_to_update);
        r.write((bit<32>) h.myhdr.reg_idx_to_update, 0x2a);
    }
}

control egress(inout Headers h,
    inout Meta m,
    inout standard_metadata_t sm)
{
    apply {
        bit<8> tmp;
        r.read(tmp, (bit<32>) h.myhdr.reg_idx_to_update);
        tmp = tmp + h.myhdr.value_to_add;
        r.write((bit<32>) h.myhdr.reg_idx_to_update, tmp);
        h.myhdr.debug_last_reg_value_written = tmp;
    }
}

control deparser(packet_out b,
    in Headers h)
{
    apply {
        b.emit(h.myhdr);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
