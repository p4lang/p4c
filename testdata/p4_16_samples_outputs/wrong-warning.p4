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
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    h1_t h1;
    h1_t h1b;
    h2_t h2;
    hu1_t hu1;
    hu1_t hu1b;
    h1_t[2] a1;
    h1_t[2] a1b;
    h2_t[2] a2;
    hu1_t[2] au1;
    hu1_t[2] au1b;
    apply {
        hu1b.h1 = { hdr.ethernet.dstAddr[45:38], hdr.ethernet.dstAddr[44:37] };
        hu1 = hu1b;
        au1b[0].h1 = { hdr.ethernet.dstAddr[37:30], hdr.ethernet.dstAddr[36:29] };
        au1b[1].h2 = { hdr.ethernet.dstAddr[35:28], hdr.ethernet.dstAddr[34:27] };
        au1 = au1b;
        a1 = a1b;
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
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
