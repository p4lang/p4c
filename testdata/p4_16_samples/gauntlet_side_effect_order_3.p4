#include <core.p4>
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct Headers {
    H h;
}

struct Meta {
    bit<8> tmp;
}

bit<8> do_thing(out bit<8> val) {
    return 8w1;
}
parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> random_val_0;
    apply {
        random_val_0 = 8w3;
        m.tmp = do_thing(random_val_0);
        h.h.a = do_thing(m.tmp);
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

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

