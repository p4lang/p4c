#include <core.p4>
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    H   h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> c = 8w12;
    action do_thing(inout bit<8> inout_c) {
        h.h.a = inout_c;
        c = 8w24;
    }
    apply {
        do_thing(c);
        do_thing(c);
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

