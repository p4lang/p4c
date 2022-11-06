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
    @name("ingress.retval") bit<16> retval_1;
    @name("ingress.not_initialized") H not_initialized_1;
    @name("ingress.new_val") bit<32> new_val_1;
    @name("ingress.do_thing_action") action do_thing_action() {
        not_initialized_0.setInvalid();
    }
    apply {
        not_initialized_1.setInvalid();
        new_val_1 = 32w1;
        if (not_initialized_1.a < 16w6) {
            ;
        } else {
            new_val_1 = 32w232;
        }
        retval_1 = (bit<16>)new_val_1;
        h.h.a = retval_1;
        do_thing_action();
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
