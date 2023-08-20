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

header I {
    bit<1> idx;
    bit<7> padding;
}

struct Headers {
    ethernet_t eth_hdr;
    H[3]       h;
    I          i;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<I>(hdr.i);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action gauntlet_index_8bmv2l51() {
        h.h[1w0].a = 8w1;
    }
    @hidden table tbl_gauntlet_index_8bmv2l51 {
        actions = {
            gauntlet_index_8bmv2l51();
        }
        const default_action = gauntlet_index_8bmv2l51();
    }
    apply {
        tbl_gauntlet_index_8bmv2l51.apply();
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
        pkt.emit<H>(h.h[0]);
        pkt.emit<H>(h.h[1]);
        pkt.emit<H>(h.h[2]);
        pkt.emit<I>(h.i);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
