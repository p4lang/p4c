#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t eth;
}

struct metadata_t {
}

struct tuple_0 {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.eth);
        log_msg<tuple_0>("i={} out1={} out2={} out3={} out4={}", (tuple_0){f0 = hdr.eth.srcAddr[15:8],f1 = hdr.eth.srcAddr[15:8],f2 = hdr.eth.srcAddr[15:8] + 8w1,f3 = hdr.eth.srcAddr[15:8] + 8w1 + 8w2,f4 = hdr.eth.srcAddr[15:8] + 8w1});
        transition accept;
    }
}

struct tuple_1 {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @hidden action shadowing1l110() {
        log_msg<tuple_0>("i={} out1={} out2={} out3={} out4={}", (tuple_0){f0 = hdr.eth.srcAddr[7:0],f1 = hdr.eth.srcAddr[7:0],f2 = hdr.eth.srcAddr[7:0] + 8w1,f3 = hdr.eth.srcAddr[7:0] + 8w1 + 8w2,f4 = hdr.eth.srcAddr[7:0] + 8w1});
        log_msg<tuple_1>("i={} out1={} out2={} out3={}", (tuple_1){f0 = hdr.eth.srcAddr[7:0],f1 = hdr.eth.srcAddr[7:0],f2 = 8w8,f3 = 8w6});
    }
    @hidden table tbl_shadowing1l110 {
        actions = {
            shadowing1l110();
        }
        const default_action = shadowing1l110();
    }
    apply {
        tbl_shadowing1l110.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
