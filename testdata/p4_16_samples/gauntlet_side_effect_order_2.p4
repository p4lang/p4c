#include <core.p4>
#include <v1model.p4>

header H {
}

struct Headers {
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {

    state start {
        transition accept;
    }

}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action do_thing(out bit<9> tmp, in bit<9> val_1) {
        sm.ingress_port = val_1;
    }
    apply {
        bit<9> tmp = 9w3;
        do_thing(tmp, tmp);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

