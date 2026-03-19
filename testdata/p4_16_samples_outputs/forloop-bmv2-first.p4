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
    action sum_loop() {
        h.h.sum = 32w0;
        for (bit<8> i = 8w1; i <= h.h.count; i = i + 8w1) {
            h.h.sum = h.h.sum + (bit<32>)i;
        }
    }
    action nested_loop() {
        h.h.product = 32w0;
        for (bit<8> i = 8w0; i < h.h.count; i = i + 8w1) {
            for (bit<8> j = 8w0; j < h.h.count; j = j + 8w1) {
                h.h.product = h.h.product + 32w1;
            }
        }
    }
    action conditional_loop() {
        h.h.sum = 32w0;
        for (bit<8> i = 8w0; i < 8w10; i = i + 8w1) {
            if (i < 8w5) {
                h.h.sum = h.h.sum + 32w1;
            }
        }
    }
    table t {
        key = {
            h.h.count: exact @name("h.h.count");
        }
        actions = {
            sum_loop();
            nested_loop();
            conditional_loop();
        }
        const default_action = sum_loop();
    }
    apply {
        t.apply();
        sm.egress_spec = 9w0;
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
