#include <core.p4>
#include <v1model.p4>

struct Headers {}

struct Meta {
    bit<1> b;
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        m.b = m.b + 1;
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    Meta x;
    apply {
        clone3<Meta>(CloneType.I2E, 32w64, x);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }
control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }
control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
