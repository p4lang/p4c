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
    bit<8> b;
    bit<8> c;
    bit<8> d;
}

header B {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
    B          b;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        pkt.extract<B>(hdr.b);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp_2") bit<8> tmp_2;
    @name("ingress.val_0") bit<8> val;
    @hidden action gauntlet_short_circuitbmv2l55() {
        tmp_2 = 8w1;
    }
    @hidden action gauntlet_short_circuitbmv2l55_0() {
        tmp_2 = 8w2;
    }
    @hidden action act() {
        h.b.b = val;
    }
    @hidden action gauntlet_short_circuitbmv2l55_1() {
        h.b.a = tmp_2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_short_circuitbmv2l55 {
        actions = {
            gauntlet_short_circuitbmv2l55();
        }
        const default_action = gauntlet_short_circuitbmv2l55();
    }
    @hidden table tbl_gauntlet_short_circuitbmv2l55_0 {
        actions = {
            gauntlet_short_circuitbmv2l55_0();
        }
        const default_action = gauntlet_short_circuitbmv2l55_0();
    }
    @hidden table tbl_gauntlet_short_circuitbmv2l55_1 {
        actions = {
            gauntlet_short_circuitbmv2l55_1();
        }
        const default_action = gauntlet_short_circuitbmv2l55_1();
    }
    apply {
        tbl_act.apply();
        if (8w1 != val) {
            tbl_gauntlet_short_circuitbmv2l55.apply();
        } else {
            tbl_gauntlet_short_circuitbmv2l55_0.apply();
        }
        tbl_gauntlet_short_circuitbmv2l55_1.apply();
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
        b.emit<ethernet_t>(h.eth_hdr);
        b.emit<H>(h.h);
        b.emit<B>(h.b);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
