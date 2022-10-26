#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<16> a;
    bit<64> b;
    bit<16> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.local_h") H local_h_0;
    @hidden action gauntlet_hdr_set_validbmv2l31() {
        h.h.c = 16w2;
    }
    @hidden action gauntlet_hdr_set_validbmv2l29() {
        local_h_0.setValid();
        local_h_0.a = 16w0;
        local_h_0.b = 64w0;
        local_h_0.c = 16w0;
    }
    @hidden table tbl_gauntlet_hdr_set_validbmv2l29 {
        actions = {
            gauntlet_hdr_set_validbmv2l29();
        }
        const default_action = gauntlet_hdr_set_validbmv2l29();
    }
    @hidden table tbl_gauntlet_hdr_set_validbmv2l31 {
        actions = {
            gauntlet_hdr_set_validbmv2l31();
        }
        const default_action = gauntlet_hdr_set_validbmv2l31();
    }
    apply {
        tbl_gauntlet_hdr_set_validbmv2l29.apply();
        if (!local_h_0.isValid() && !h.h.isValid() || local_h_0.isValid() && h.h.isValid() && 16w0 == h.h.a && 64w0 == h.h.b && 16w0 == h.h.c) {
            tbl_gauntlet_hdr_set_validbmv2l31.apply();
        }
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
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
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
