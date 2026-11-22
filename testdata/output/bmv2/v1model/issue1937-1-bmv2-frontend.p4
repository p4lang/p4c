#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    h1_t h1;
}

struct metadata_t {
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.tmp") bit<8> tmp;
    @name("ingressImpl.x") bit<8> x_0;
    @name("ingressImpl.y") bit<8> y_0;
    @name("ingressImpl.x") bit<8> x_2;
    @name("ingressImpl.y") bit<8> y_2;
    @name(".foo") action foo_1() {
        y_0 = tmp;
        x_0 = y_0 >> 2;
        hdr.h1.f1 = x_0;
    }
    @name(".foo") action foo_2() {
        y_2 = 8w5;
        x_2 = y_2 >> 2;
        hdr.h1.f2 = x_2;
    }
    apply {
        tmp = hdr.h1.f1;
        foo_1();
        foo_2();
    }
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
