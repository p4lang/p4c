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
    bit<8> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
    bit<8> tmp;
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.random_val_0") bit<8> random_val;
    @name("ingress.val_0") bit<8> val;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.val_1") bit<8> val_2;
    @name("ingress.hasReturned") bool hasReturned_1;
    @name("ingress.retval") bit<8> retval_1;
    apply {
        hasReturned = false;
        hasReturned = true;
        retval = 8w1;
        random_val = val;
        hasReturned_1 = false;
        hasReturned_1 = true;
        retval_1 = 8w1;
        m.tmp = val_2;
        h.h.a = retval_1;
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
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

