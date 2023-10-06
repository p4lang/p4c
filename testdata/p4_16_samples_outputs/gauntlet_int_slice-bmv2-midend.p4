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
    @name("ingress.tmp") ethernet_t tmp_0;
    @hidden action gauntlet_int_slicebmv2l33() {
        tmp_0.setValid();
        tmp_0.dst_addr = 48w1024;
        tmp_0.src_addr = 48w1;
        tmp_0.eth_type = 16w1;
        h.h.a = 8w2;
    }
    @hidden table tbl_gauntlet_int_slicebmv2l33 {
        actions = {
            gauntlet_int_slicebmv2l33();
        }
        const default_action = gauntlet_int_slicebmv2l33();
    }
    apply {
        tbl_gauntlet_int_slicebmv2l33.apply();
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
