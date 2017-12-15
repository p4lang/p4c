#include <core.p4>
#include <v1model.p4>

struct Headers {
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

register<bit<8>>(2) r;

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        bit<8> x;
        r.read(x, 0);
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

