#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr_t {
    bit<8>  count;
    bit<32> sum;
    bit<32> product;
}

struct Headers {
    hdr_t h;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<hdr_t>(h.h);
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
        b.emit<hdr_t>(h.h);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.i") bit<8> i_0;
    @name("ingress.i") bit<8> i_1;
    @name("ingress.j") bit<8> j_0;
    @name("ingress.i") bit<8> i_3;
    @name("ingress.sum_loop") action sum_loop() {
        h.h.sum = 32w0;
        for (i_0 = 8w1; i_0 <= h.h.count; i_0 = i_0 + 8w1) {
            h.h.sum = h.h.sum + (bit<32>)i_0;
        }
    }
    @name("ingress.nested_loop") action nested_loop() {
        h.h.product = 32w0;
        for (i_1 = 8w0; i_1 < h.h.count; i_1 = i_1 + 8w1) {
            for (j_0 = 8w0; j_0 < h.h.count; j_0 = j_0 + 8w1) {
                h.h.product = h.h.product + 32w1;
            }
        }
    }
    @name("ingress.conditional_loop") action conditional_loop() {
        h.h.sum = 32w0;
        h.h.sum = 32w1;
        h.h.sum = 32w2;
        h.h.sum = 32w3;
        h.h.sum = 32w4;
        h.h.sum = 32w5;
    }
    @name("ingress.break_loop") action break_loop() {
        h.h.sum = 32w0;
        for (i_3 = 8w0; i_3 < h.h.count; i_3 = i_3 + 8w1) {
            if (i_3 == 8w5) {
                break;
            } else {
                h.h.sum = h.h.sum + 32w1;
            }
        }
    }
    @name("ingress.continue_loop") action continue_loop() {
        h.h.sum = 32w0;
        h.h.sum = 32w1;
        h.h.sum = 32w2;
        h.h.sum = 32w3;
        h.h.sum = 32w4;
        h.h.sum = 32w5;
    }
    @name("ingress.t") table t_0 {
        key = {
            h.h.count: exact @name("h.h.count");
        }
        actions = {
            sum_loop();
            nested_loop();
            conditional_loop();
            break_loop();
            continue_loop();
        }
        const default_action = sum_loop();
    }
    @hidden action forloopbmv2l99() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_forloopbmv2l99 {
        actions = {
            forloopbmv2l99();
        }
        const default_action = forloopbmv2l99();
    }
    apply {
        t_0.apply();
        tbl_forloopbmv2l99.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
