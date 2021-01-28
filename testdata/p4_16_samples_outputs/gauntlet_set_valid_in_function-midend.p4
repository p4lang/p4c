#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8>  a;
    bit<64> b;
    bit<16> c;
}

struct Headers {
    H h;
}

struct Meta {
    bit<32> t;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp_ret") H tmp_ret_0;
    @hidden action gauntlet_set_valid_in_function19() {
        tmp_ret_0.setValid();
        tmp_ret_0.a = 8w0;
        tmp_ret_0.b = 64w0;
        tmp_ret_0.c = 16w0;
        m.t = 32w0;
    }
    @hidden table tbl_gauntlet_set_valid_in_function19 {
        actions = {
            gauntlet_set_valid_in_function19();
        }
        const default_action = gauntlet_set_valid_in_function19();
    }
    apply {
        tbl_gauntlet_set_valid_in_function19.apply();
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

