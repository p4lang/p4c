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
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    ethernet_t tmp_eth_hdr;
    ethernet_t tmp_0;
    bit<48> tmp_1;
    @hidden action issue2488bmv2l37() {
        tmp_1 = h.eth_hdr.dst_addr;
        h.eth_hdr.dst_addr = 48w1;
        tmp_0.setValid();
        tmp_0.dst_addr = tmp_1;
        tmp_0.src_addr = 48w2;
        tmp_0.eth_type = 16w1;
        tmp_eth_hdr.setValid();
        tmp_eth_hdr = tmp_0;
        h.eth_hdr.dst_addr = 48w1;
        h.eth_hdr = tmp_0;
    }
    @hidden table tbl_issue2488bmv2l37 {
        actions = {
            issue2488bmv2l37();
        }
        const default_action = issue2488bmv2l37();
    }
    apply {
        tbl_issue2488bmv2l37.apply();
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
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

