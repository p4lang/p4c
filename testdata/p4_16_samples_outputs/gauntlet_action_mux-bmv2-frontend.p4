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
    H h;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.val_0") bit<8> val;
    @name("ingress.val_0") bit<8> val_2;
    @name("ingress.do_thing") action do_thing() {
        val = h.h.b;
        if (h.h.b >= 8w4) {
            tmp = h.h.b;
        } else {
            tmp = h.h.b + 8w1;
        }
        h.h.a = tmp;
        h.h.b = val;
    }
    @name("ingress.do_thing") action do_thing_1() {
        val_2 = h.h.a;
        h.h.a = val_2;
    }
    apply {
        do_thing();
        do_thing_1();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
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
