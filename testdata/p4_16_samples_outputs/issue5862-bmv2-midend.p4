#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Result {
    bit<8> flags;
}

header H {
    bit<8> x;
}

struct Headers {
    Result result;
    H      u_a;
    H      u_b;
}

struct Meta {
}

parser P(packet_in packet, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        packet.extract<Result>(h.result);
        packet.extract<H>(h.u_a);
        h.u_b.setInvalid();
        transition accept;
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action issue5862bmv2l29() {
        h.result.flags = 8w1;
    }
    @hidden action issue5862bmv2l27() {
        h.u_b.x = 8w2;
        h.result.flags = 8w0;
    }
    @hidden action issue5862bmv2l30() {
        h.result.flags = h.result.flags | 8w2;
    }
    @hidden action issue5862bmv2l31() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_issue5862bmv2l27 {
        actions = {
            issue5862bmv2l27();
        }
        const default_action = issue5862bmv2l27();
    }
    @hidden table tbl_issue5862bmv2l29 {
        actions = {
            issue5862bmv2l29();
        }
        const default_action = issue5862bmv2l29();
    }
    @hidden table tbl_issue5862bmv2l30 {
        actions = {
            issue5862bmv2l30();
        }
        const default_action = issue5862bmv2l30();
    }
    @hidden table tbl_issue5862bmv2l31 {
        actions = {
            issue5862bmv2l31();
        }
        const default_action = issue5862bmv2l31();
    }
    apply {
        tbl_issue5862bmv2l27.apply();
        if (h.u_a.isValid()) {
            tbl_issue5862bmv2l29.apply();
        }
        if (h.u_b.isValid()) {
            tbl_issue5862bmv2l30.apply();
        }
        tbl_issue5862bmv2l31.apply();
    }
}

control E(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control Verify(inout Headers h, inout Meta m) {
    apply {
    }
}

control Update(inout Headers h, inout Meta m) {
    apply {
    }
}

control D(packet_out packet, in Headers h) {
    apply {
        packet.emit<Result>(h.result);
    }
}

V1Switch<Headers, Meta>(P(), Verify(), I(), E(), Update(), D()) main;
