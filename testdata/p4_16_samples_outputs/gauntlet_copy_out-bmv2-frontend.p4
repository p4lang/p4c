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

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.c") bit<8> c_0;
    @name("ingress.inout_c") bit<8> inout_c_0;
    @name("ingress.inout_c") bit<8> inout_c_2;
    @name("ingress.do_thing") action do_thing() {
        inout_c_0 = c_0;
        h.h.a = inout_c_0;
        c_0 = inout_c_0;
    }
    @name("ingress.do_thing") action do_thing_1() {
        inout_c_2 = c_0;
        h.h.a = inout_c_2;
        c_0 = inout_c_2;
    }
    apply {
        c_0 = 8w12;
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

