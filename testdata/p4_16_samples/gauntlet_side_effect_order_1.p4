#include <core.p4>
#include <v1model.p4>

header H {
    bit<16>  a;
}

struct Headers {
    H    h;
}

struct Meta {
}


parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action do_action_1(out bit<16> val_1, out bit<16> val_2) {
        val_1 = val_2;
    }
    action do_action_2(out bit<16> val) {
        do_action_1(val, val);
    }
    apply {
        do_action_2(h.h.a);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

