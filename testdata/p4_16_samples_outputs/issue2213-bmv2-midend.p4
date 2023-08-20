#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct mystruct1_t {
    bit<16> f1;
    bit<8>  f2;
}

struct mystruct2_t {
    mystruct1_t s1;
    bit<16>     f3;
    bit<32>     f4;
}

struct metadata_t {
    bit<16> _s1_f10;
    bit<8>  _s1_f21;
    bit<16> _s2_s1_f12;
    bit<8>  _s2_s1_f23;
    bit<16> _s2_f34;
    bit<32> _s2_f45;
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @hidden action issue2213bmv2l71() {
        meta._s1_f10 = 16w2;
        meta._s1_f21 = 8w3;
        meta._s2_s1_f12 = 16w2;
        meta._s2_s1_f23 = 8w3;
        meta._s2_f34 = 16w5;
        meta._s2_f45 = 32w8;
        stdmeta.egress_spec = 9w3;
    }
    @hidden table tbl_issue2213bmv2l71 {
        actions = {
            issue2213bmv2l71();
        }
        const default_action = issue2213bmv2l71();
    }
    apply {
        tbl_issue2213bmv2l71.apply();
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
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
