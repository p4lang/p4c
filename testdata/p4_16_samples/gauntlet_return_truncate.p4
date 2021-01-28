#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>  a;
    bit<32> b;
    bit<64> c;
}

struct Headers {
    H    h;
}

struct Meta {
}

bit<16> do_thing() {
    return (bit<16>)16w4;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<64> y = 64w3;
    bit<64> z = 64w2;
    action iuJze(in bit<8> hyhe) {
        h.h.c = 64w2;
        y = (bit<64>)do_thing();
        h.h.c = y;
    }
    apply {
        iuJze(8w2);
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

