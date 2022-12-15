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

header IDX {
    bit<8> idx;
}

struct Headers {
    ethernet_t eth_hdr;
    IDX        idx;
    H[2]       h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<IDX>(hdr.idx);
        pkt.extract<H>(hdr.h[32w0]);
        pkt.extract<H>(hdr.h[32w1]);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<8> tmp;
    bit<8> hsiVar;
    @name("ingress.simple_action") action simple_action() {
        tmp = (h.idx.idx < 8w1 ? h.idx.idx : tmp);
        tmp = (h.idx.idx < 8w1 ? h.idx.idx : 8w1);
        hsiVar = (h.idx.idx < 8w1 ? h.idx.idx : 8w1);
        if (hsiVar == 8w0) {
            h.h[8w0].a = 8w1;
        } else if (hsiVar == 8w1) {
            h.h[8w1].a = 8w1;
        }
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action();
        }
        const default_action = simple_action();
    }
    apply {
        tbl_simple_action.apply();
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
        b.emit<IDX>(h.idx);
        b.emit<H>(h.h[0]);
        b.emit<H>(h.h[1]);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
