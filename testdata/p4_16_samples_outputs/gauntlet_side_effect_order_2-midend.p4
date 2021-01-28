#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    @name("ingress.tmp_2") bit<9> tmp_2;
    bit<9> tmp;
    @name("ingress.do_thing") action do_thing() {
        sm.ingress_port = tmp_2;
    }
    @hidden action act() {
        tmp_2 = 9w3;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_do_thing {
        actions = {
            do_thing();
        }
        const default_action = do_thing();
    }
    apply {
        tbl_act.apply();
        tbl_do_thing.apply();
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

