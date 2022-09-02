#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<16> a;
}

struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.not_initialized") H not_initialized_0;
    @name("ingress.not_initialized") H not_initialized_1;
    @name("ingress.new_val") bit<32> new_val_1;
    @name("ingress.do_thing_action") action do_thing_action() {
        not_initialized_0.setInvalid();
    }
    @hidden action issue2148l21() {
        new_val_1 = 32w232;
    }
    @hidden action issue2148l17() {
        not_initialized_1.setInvalid();
        new_val_1 = 32w1;
    }
    @hidden action issue2148l30() {
        h.h.a = (bit<16>)new_val_1;
    }
    @hidden table tbl_issue2148l17 {
        actions = {
            issue2148l17();
        }
        const default_action = issue2148l17();
    }
    @hidden table tbl_issue2148l21 {
        actions = {
            issue2148l21();
        }
        const default_action = issue2148l21();
    }
    @hidden table tbl_issue2148l30 {
        actions = {
            issue2148l30();
        }
        const default_action = issue2148l30();
    }
    @hidden table tbl_do_thing_action {
        actions = {
            do_thing_action();
        }
        const default_action = do_thing_action();
    }
    apply {
        tbl_issue2148l17.apply();
        if (not_initialized_1.a < 16w6) {
            ;
        } else {
            tbl_issue2148l21.apply();
        }
        tbl_issue2148l30.apply();
        tbl_do_thing_action.apply();
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

