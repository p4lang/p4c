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
    @name("ingress.tmp_0") bit<3> tmp;
    @name("ingress.tmp_1") bit<3> tmp_0;
    @name("ingress.val_0") bit<3> val;
    @name("ingress.bound_val_0") bit<3> bound_val;
    @name("ingress.retval") bit<3> retval;
    @name("ingress.tmp") bit<3> tmp_1;
    apply {
        val = h.i.a;
        bound_val = 3w1;
        if (val > bound_val) {
            tmp_1 = bound_val;
        } else {
            tmp_1 = val;
        }
        retval = tmp_1;
        tmp = retval;
        tmp_0 = tmp;
        h.h[tmp_0].a = 32w1;
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
