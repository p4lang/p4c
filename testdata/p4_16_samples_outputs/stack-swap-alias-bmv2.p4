#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> x;
}

struct Headers {
    H[2] hs;
}

struct Meta {
}

parser P(packet_in p, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        p.extract(h.hs[0]);
        p.extract(h.hs[1]);
        transition accept;
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        if (h.hs[0].x == 0xfe) {
            h.hs[0].setInvalid();
        }
        if (h.hs[1].x == 0xfe) {
            h.hs[1].setInvalid();
        }
        h.hs = { h.hs[1], h.hs[0] };
        sm.egress_spec = 0;
    }
}

control E(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control V(inout Headers h, inout Meta m) {
    apply {
    }
}

control U(inout Headers h, inout Meta m) {
    apply {
    }
}

control D(packet_out p, in Headers h) {
    apply {
        p.emit(h.hs);
    }
}

V1Switch(P(), V(), I(), E(), U(), D()) main;
