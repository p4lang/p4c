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
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name("ingress.hPSe_0") bit<8> hPSe;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<16> retval;
    @name("ingress.hPSe_1") bit<8> hPSe_2;
    @name("ingress.hasReturned") bool hasReturned_1;
    @name("ingress.retval") bit<16> retval_1;
    apply {
        if (h.eth_hdr.src_addr == 48w5) {
            hasReturned = false;
            hasReturned = true;
            retval = 16w2;
            h.h.a = hPSe;
            tmp_0 = retval;
            tmp = tmp_0;
        } else {
            hasReturned_1 = false;
            hasReturned_1 = true;
            retval_1 = 16w2;
            h.h.b = hPSe_2;
            tmp_1 = retval_1;
            tmp = tmp_1;
        }
        h.eth_hdr.eth_type = tmp;
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
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

