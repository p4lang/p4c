#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.b") bool b_0;
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.dummy") action dummy_2() {
        if (b_0) {
            tmp = 16w1;
        } else {
            tmp = 16w1;
        }
        h.eth_hdr.eth_type = tmp;
    }
    @hidden action gauntlet_mux_validitybmv2l37() {
        b_0 = false;
    }
    @hidden table tbl_gauntlet_mux_validitybmv2l37 {
        actions = {
            gauntlet_mux_validitybmv2l37();
        }
        const default_action = gauntlet_mux_validitybmv2l37();
    }
    @hidden table tbl_dummy {
        actions = {
            dummy_2();
        }
        const default_action = dummy_2();
    }
    apply {
        tbl_gauntlet_mux_validitybmv2l37.apply();
        tbl_dummy.apply();
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
