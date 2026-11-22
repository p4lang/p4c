#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    mystruct1_t s1;
    mystruct2_t s2;
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
    apply {
        meta.s1 = (mystruct1_t){f1 = 16w2,f2 = 8w3};
        meta.s2.s1 = meta.s1;
        meta.s2 = (mystruct2_t){s1 = meta.s1,f3 = 16w5,f4 = 32w8};
        meta.s2 = (mystruct2_t){s1 = (mystruct1_t){f1 = 16w2,f2 = 8w3},f3 = 16w5,f4 = 32w8};
        stdmeta.egress_spec = (bit<9>)meta.s2.s1.f2;
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
