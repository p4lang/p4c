#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(h.eth_hdr);
        pkt.extract<H>(h.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    @hidden action gauntlet_set_invalidbmv2l35() {
        h.h.a = 8w1;
        h.h.setInvalid();
        h.h.setValid();
        h.h.b = 8w2;
    }
    @hidden table tbl_gauntlet_set_invalidbmv2l35 {
        actions = {
            gauntlet_set_invalidbmv2l35();
        }
        const default_action = gauntlet_set_invalidbmv2l35();
    }
    apply {
        tbl_gauntlet_set_invalidbmv2l35.apply();
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

