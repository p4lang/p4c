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
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<8> tmp_2;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.tmp_4") bit<8> tmp_4;
    @name("ingress.tmp_5") bit<8> tmp_5;
    apply {
        tmp = h.h.a;
        tmp_0 = h.h.a;
        tmp_1 = h.h.a;
        tmp_2 = h.h.a;
        {
            @name("ingress.x_0") bit<8> x_0 = tmp_1;
            @name("ingress.y_0") bit<8> y_0 = tmp_2;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<8> retval;
            x_0 = x_0 + y_0;
            hasReturned = true;
            retval = 8w4;
            tmp_1 = x_0;
            tmp_3 = retval;
        }
        {
            @name("ingress.x_1") bit<8> x_1 = tmp_0;
            @name("ingress.y_1") bit<8> y_1 = tmp_3;
            @name("ingress.hasReturned") bool hasReturned_1 = false;
            @name("ingress.retval") bit<8> retval_1;
            x_1 = x_1 + y_1;
            hasReturned_1 = true;
            retval_1 = 8w4;
            tmp_0 = x_1;
            tmp_4 = retval_1;
        }
        {
            @name("ingress.x_2") bit<8> x_2 = tmp;
            @name("ingress.y_2") bit<8> y_2 = tmp_4;
            @name("ingress.hasReturned") bool hasReturned_2 = false;
            @name("ingress.retval") bit<8> retval_2;
            x_2 = x_2 + y_2;
            hasReturned_2 = true;
            retval_2 = 8w4;
            tmp = x_2;
            tmp_5 = retval_2;
        }
        h.h.a = tmp;
        h.h.b = tmp_5;
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

