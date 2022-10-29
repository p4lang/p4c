#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header I {
    bit<3> a;
    bit<5> padding;
}

header H {
    bit<32> a;
}

struct Headers {
    ethernet_t eth_hdr;
    I          i;
    H[2]       h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<I>(hdr.i);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<3> tmp_1;
    @hidden action gauntlet_index_1bmv2l4() {
        tmp_1 = 3w1;
    }
    @hidden action gauntlet_index_1bmv2l4_0() {
        tmp_1 = h.i.a;
    }
    @hidden action gauntlet_index_1bmv2l45() {
        h.h[3w0].a = 32w1;
    }
    @hidden action gauntlet_index_1bmv2l45_0() {
        h.h[3w1].a = 32w1;
    }
    @hidden table tbl_gauntlet_index_1bmv2l4 {
        actions = {
            gauntlet_index_1bmv2l4();
        }
        const default_action = gauntlet_index_1bmv2l4();
    }
    @hidden table tbl_gauntlet_index_1bmv2l4_0 {
        actions = {
            gauntlet_index_1bmv2l4_0();
        }
        const default_action = gauntlet_index_1bmv2l4_0();
    }
    @hidden table tbl_gauntlet_index_1bmv2l45 {
        actions = {
            gauntlet_index_1bmv2l45();
        }
        const default_action = gauntlet_index_1bmv2l45();
    }
    @hidden table tbl_gauntlet_index_1bmv2l45_0 {
        actions = {
            gauntlet_index_1bmv2l45_0();
        }
        const default_action = gauntlet_index_1bmv2l45_0();
    }
    apply {
        if (h.i.a > 3w1) {
            tbl_gauntlet_index_1bmv2l4.apply();
        } else {
            tbl_gauntlet_index_1bmv2l4_0.apply();
        }
        if (tmp_1 == 3w0) {
            tbl_gauntlet_index_1bmv2l45.apply();
        } else if (tmp_1 == 3w1) {
            tbl_gauntlet_index_1bmv2l45_0.apply();
        }
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
        pkt.emit<I>(h.i);
        pkt.emit<H>(h.h[0]);
        pkt.emit<H>(h.h[1]);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
