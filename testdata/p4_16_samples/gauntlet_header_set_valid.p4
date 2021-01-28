#include <core.p4>
#include <v1model.p4>

header H {
    bit<16> a;
    bit<64> b;
    bit<16> c;
}


struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    apply {
        H local_h = { 16w0, 64w0, 16w0 };
        if (local_h == h.h) {
            h.h.c = 16w2;
        }
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

