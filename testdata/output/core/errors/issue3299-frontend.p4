#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct H {
}

struct M {
    bit<8> m0;
}

parser P(packet_in p, out H h, inout M m, inout standard_metadata_t s) {
    state start {
        m.m0 = 8w0;
        transition accept;
    }
}

control I(inout H h, inout M m, inout standard_metadata_t s) {
    @name("I.a0") action a0() {
        switch (m.m0) {
            8w0x0: {
            }
        }
    }
    apply {
        a0();
    }
}

control E(inout H h, inout M m, inout standard_metadata_t s) {
    apply {
    }
}

control D(packet_out p, in H h) {
    apply {
    }
}

control V(inout H h, inout M m) {
    apply {
    }
}

control C(inout H h, inout M m) {
    apply {
    }
}

V1Switch<H, M>(P(), V(), I(), E(), C(), D()) main;
