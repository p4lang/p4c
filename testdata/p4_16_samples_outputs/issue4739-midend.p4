#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.n") bit<8> n_0;
    @name("ingressImpl.i") bit<8> i_0;
    @hidden action issue4739l52() {
        i_0 = i_0 + 8w1;
        n_0 = n_0 + 8w1;
    }
    @hidden action issue4739l50() {
        n_0 = 8w0;
    }
    @hidden action issue4739l55() {
        hdr.ethernet.srcAddr[7:0] = n_0;
        stdmeta.egress_spec = 9w1;
    }
    @hidden table tbl_issue4739l50 {
        actions = {
            issue4739l50();
        }
        const default_action = issue4739l50();
    }
    @hidden table tbl_issue4739l52 {
        actions = {
            issue4739l52();
        }
        const default_action = issue4739l52();
    }
    @hidden table tbl_issue4739l55 {
        actions = {
            issue4739l55();
        }
        const default_action = issue4739l55();
    }
    apply {
        tbl_issue4739l50.apply();
        for (i_0 = 8w0; i_0 < 8w8; i_0 = i_0 + 8w1) {
            tbl_issue4739l52.apply();
        }
        tbl_issue4739l55.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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
