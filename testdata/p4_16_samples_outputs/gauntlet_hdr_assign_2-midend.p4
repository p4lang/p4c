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
    @hidden action gauntlet_hdr_assign_2l24() {
        h.h.setValid();
    }
    @hidden action gauntlet_hdr_assign_2l26() {
        h.h.setValid();
    }
    @hidden action gauntlet_hdr_assign_2l21() {
        h.h.setInvalid();
    }
    @hidden action gauntlet_hdr_assign_2l28() {
        h.h.b = 8w2;
    }
    @hidden table tbl_gauntlet_hdr_assign_2l21 {
        actions = {
            gauntlet_hdr_assign_2l21();
        }
        const default_action = gauntlet_hdr_assign_2l21();
    }
    @hidden table tbl_gauntlet_hdr_assign_2l24 {
        actions = {
            gauntlet_hdr_assign_2l24();
        }
        const default_action = gauntlet_hdr_assign_2l24();
    }
    @hidden table tbl_gauntlet_hdr_assign_2l26 {
        actions = {
            gauntlet_hdr_assign_2l26();
        }
        const default_action = gauntlet_hdr_assign_2l26();
    }
    @hidden table tbl_gauntlet_hdr_assign_2l28 {
        actions = {
            gauntlet_hdr_assign_2l28();
        }
        const default_action = gauntlet_hdr_assign_2l28();
    }
    apply {
        tbl_gauntlet_hdr_assign_2l21.apply();
        if (sm.egress_port == 9w2) {
            tbl_gauntlet_hdr_assign_2l24.apply();
        } else {
            tbl_gauntlet_hdr_assign_2l26.apply();
        }
        tbl_gauntlet_hdr_assign_2l28.apply();
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

