#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

header word_t {
    bit<32> x;
}

struct metadata {
}

struct headers {
    data_t    data;
    word_t[8] rest;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<data_t>(hdr.data);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<8> tmp_2;
    @name("ingress.x_0") bit<8> x_2;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.rv") bit<8> rv_0;
    @name("ingress.x_1") bit<8> x_3;
    @name("ingress.retval") bit<8> retval_1;
    @name("ingress.rv") bit<8> rv_1;
    apply {
        x_2 = hdr.data.b1;
        rv_0 = x_2;
        x_2 = x_2 + 8w1;
        retval = rv_0;
        hdr.data.b1 = x_2;
        tmp = retval;
        tmp_0 = tmp & 8w7;
        hdr.rest[tmp_0].x = hdr.rest[tmp_0].x | hdr.data.f1;
        x_3 = hdr.data.b1;
        rv_1 = x_3;
        x_3 = x_3 + 8w1;
        retval_1 = rv_1;
        hdr.data.b1 = x_3;
        tmp_1 = retval_1;
        tmp_2 = tmp_1 & 8w7;
        hdr.rest[tmp_2].x = hdr.rest[tmp_2].x | hdr.data.f2;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control deparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;
