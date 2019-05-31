#include <core.p4>
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
    @name(".foo") action foo(out bit<8> x, in bit<8> y=5) {
        x = y >> 2;
    }
    @name(".foo") action foo_0(out bit<8> x_1, in bit<8> y_1=5) {
        x_1 = y_1 >> 2;
    }
    bit<8> tmp;
    bit<8> tmp_0;
    apply {
        tmp_0 = hdr.h1.f1;
        foo(tmp, tmp_0);
        hdr.h1.f1 = tmp;
        foo_0(x_1 = hdr.h1.f2, y_1 = 8w5);
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

