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

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

header_union hu1_t {
    h1_t h1;
    h2_t h2;
}

struct metadata_t {
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
    @name("ingressImpl.hu1") hu1_t hu1_0;
    @name("ingressImpl.hu1b") hu1_t hu1b_0;
    @name("ingressImpl.a1") h1_t[2] a1_0;
    @name("ingressImpl.a1b") h1_t[2] a1b_0;
    @name("ingressImpl.au1") hu1_t[2] au1_0;
    @name("ingressImpl.au1b") hu1_t[2] au1b_0;
    apply {
        hu1_0.h1.setInvalid();
        hu1_0.h2.setInvalid();
        hu1b_0.h1.setInvalid();
        hu1b_0.h2.setInvalid();
        a1_0[0].setInvalid();
        a1_0[1].setInvalid();
        a1b_0[0].setInvalid();
        a1b_0[1].setInvalid();
        au1_0[0].h1.setInvalid();
        au1_0[0].h2.setInvalid();
        au1_0[1].h1.setInvalid();
        au1_0[1].h2.setInvalid();
        au1b_0[0].h1.setInvalid();
        au1b_0[0].h2.setInvalid();
        au1b_0[1].h1.setInvalid();
        au1b_0[1].h2.setInvalid();
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
