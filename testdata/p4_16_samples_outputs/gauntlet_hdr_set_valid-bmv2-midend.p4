#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<16> a;
    bit<64> b;
    bit<16> c;
}

struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.local_h") H local_h_0;
    @hidden action gauntlet_hdr_set_validbmv2l23() {
        h.h.c = 16w2;
    }
    @hidden action gauntlet_hdr_set_validbmv2l21() {
        local_h_0.setValid();
        local_h_0.a = 16w0;
        local_h_0.b = 64w0;
        local_h_0.c = 16w0;
    }
    @hidden table tbl_gauntlet_hdr_set_validbmv2l21 {
        actions = {
            gauntlet_hdr_set_validbmv2l21();
        }
        const default_action = gauntlet_hdr_set_validbmv2l21();
    }
    @hidden table tbl_gauntlet_hdr_set_validbmv2l23 {
        actions = {
            gauntlet_hdr_set_validbmv2l23();
        }
        const default_action = gauntlet_hdr_set_validbmv2l23();
    }
    apply {
        tbl_gauntlet_hdr_set_validbmv2l21.apply();
        if (!local_h_0.isValid() && !h.h.isValid() || local_h_0.isValid() && h.h.isValid() && 16w0 == h.h.a && 64w0 == h.h.b && 16w0 == h.h.c) {
            tbl_gauntlet_hdr_set_validbmv2l23.apply();
        }
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

