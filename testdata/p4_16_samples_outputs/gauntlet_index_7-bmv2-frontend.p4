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
    bit<1> id;
    bit<7> padding;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]       h;
    I          i;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<I>(hdr.i);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<1> tmp;
    @name("ingress.tmp_0") bit<1> tmp_0;
    apply {
        {
            @name("ingress.val_0") bit<1> val_0 = h.i.id;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<1> retval;
            hasReturned = true;
            retval = val_0;
            tmp = retval;
        }
        tmp_0 = tmp;
        {
            @name("ingress.id_0") bit<1> id_0 = h.i.id;
            @name("ingress.hasReturned_0") bool hasReturned_0 = false;
            @name("ingress.retval_0") bit<8> retval_0;
            id_0 = 1w0;
            hasReturned_0 = true;
            retval_0 = 8w1;
            h.i.id = id_0;
            h.h[tmp_0].a = retval_0;
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
        pkt.emit<H>(h.h[0]);
        pkt.emit<H>(h.h[1]);
        pkt.emit<I>(h.i);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

