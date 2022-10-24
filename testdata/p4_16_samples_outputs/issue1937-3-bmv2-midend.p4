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
    @hidden action issue19373bmv2l26() {
        hdr.h1.f1 = hdr.h1.f1 >> 2;
        hdr.h1.f2 = 8w1;
    }
    @hidden table tbl_issue19373bmv2l26 {
        actions = {
            issue19373bmv2l26();
        }
        const default_action = issue19373bmv2l26();
    }
    apply {
        tbl_issue19373bmv2l26.apply();
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
