#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>  a;
    bit<8>  b;
    bit<8>  c;
}

struct Headers {
    H h;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    H tmp_1 = { 3, 8w1, 1};
    H tmp_2 = { 8w4, 8w0, 2};
    bit<8> val = 8w3;
    action do_action() {
        sm.ingress_port = 9w0;

    }
    table BfyXpa {
        key = {
            val     : exact @name("eJPEfW") ;
        }
        actions = {
            do_action();
        }
    }
    apply {
        val = tmp_1.a;
        BfyXpa.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

