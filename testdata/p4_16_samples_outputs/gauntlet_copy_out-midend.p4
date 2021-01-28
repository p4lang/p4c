#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.c") bit<8> c_0;
    @name("ingress.do_thing") action do_thing() {
        h.h.a = c_0;
    }
    @name("ingress.do_thing") action do_thing_2() {
        h.h.a = c_0;
    }
    @hidden action gauntlet_copy_out17() {
        c_0 = 8w12;
    }
    @hidden table tbl_gauntlet_copy_out17 {
        actions = {
            gauntlet_copy_out17();
        }
        const default_action = gauntlet_copy_out17();
    }
    @hidden table tbl_do_thing {
        actions = {
            do_thing();
        }
        const default_action = do_thing();
    }
    @hidden table tbl_do_thing_0 {
        actions = {
            do_thing_2();
        }
        const default_action = do_thing_2();
    }
    apply {
        tbl_gauntlet_copy_out17.apply();
        tbl_do_thing.apply();
        tbl_do_thing_0.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
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

