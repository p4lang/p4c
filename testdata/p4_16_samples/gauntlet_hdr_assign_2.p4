#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>  a;
    bit<8>  b;
}


struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    apply {
        h.h.a = 8w1;
        h.h.setInvalid();
        h.h.b = 8w3;
        if (sm.egress_port == 2) {
            h.h.setValid();
        } else {
            h.h.setValid();
        }
        h.h.b = 8w2;
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

