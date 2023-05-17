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
    @name("ingress.tmp") bit<1> tmp;
    @name("ingress.tmp_0") bit<1> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<1> tmp_2;
    @name("ingress.tmp_3") bit<1> tmp_3;
    @name("ingress.val_0") bit<1> val;
    @name("ingress.retval") bit<1> retval;
    @name("ingress.val_1") bit<8> val_2;
    @name("ingress.retval_0") bit<1> retval_0;
    apply {
        val = h.i.idx;
        retval = val;
        tmp = retval;
        tmp_0 = tmp;
        tmp_1 = h.h[tmp_0].a;
        val_2 = tmp_1;
        retval_0 = 1w0;
        tmp_1 = val_2;
        tmp_2 = retval_0;
        h.h[tmp_0].a = tmp_1;
        tmp_3 = tmp_2;
        h.h[tmp_3].a = 8w1;
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
