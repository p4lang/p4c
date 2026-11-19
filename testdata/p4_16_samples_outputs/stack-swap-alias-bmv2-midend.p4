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
        p.extract<H>(h.hs[0]);
        p.extract<H>(h.hs[1]);
        transition accept;
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    H[2] tmp;
    @hidden action stackswapaliasbmv2l22() {
        h.hs[0].setInvalid();
    }
    @hidden action stackswapaliasbmv2l23() {
        h.hs[1].setInvalid();
    }
    @hidden action stackswapaliasbmv2l25() {
        tmp[0] = h.hs[1];
        tmp[1] = h.hs[0];
        h.hs[0] = h.hs[1];
        h.hs[1] = tmp[1];
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_stackswapaliasbmv2l22 {
        actions = {
            stackswapaliasbmv2l22();
        }
        const default_action = stackswapaliasbmv2l22();
    }
    @hidden table tbl_stackswapaliasbmv2l23 {
        actions = {
            stackswapaliasbmv2l23();
        }
        const default_action = stackswapaliasbmv2l23();
    }
    @hidden table tbl_stackswapaliasbmv2l25 {
        actions = {
            stackswapaliasbmv2l25();
        }
        const default_action = stackswapaliasbmv2l25();
    }
    apply {
        if (h.hs[0].x == 8w0xfe) {
            tbl_stackswapaliasbmv2l22.apply();
        }
        if (h.hs[1].x == 8w0xfe) {
            tbl_stackswapaliasbmv2l23.apply();
        }
        tbl_stackswapaliasbmv2l25.apply();
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
        p.emit<H>(h.hs[0]);
        p.emit<H>(h.hs[1]);
    }
}

V1Switch<Headers, Meta>(P(), V(), I(), E(), U(), D()) main;
