#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header h0_t {
}

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s0_t {
}

struct s1_t {
    bit<8> f1;
}

struct s2_t {
    bit<8> f1;
    bit<8> f2;
}

header hstructs_t {
    s0_t s0;
    s1_t s1;
    s2_t s2;
}

struct headers_t {
    ethernet_t ethernet;
    h0_t       h0;
    h1_t       h1;
    h2_t       h2;
    hstructs_t hstructs;
}

struct metadata_t {
}

parser parserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        if (hdr.ethernet.isValid()) {
            hdr.hstructs.setValid();
            hdr.h0.setValid();
            hdr.h0 = (h0_t){};
            hdr.hstructs.s0 = (s0_t){};
            if (hdr.ethernet.etherType == 16w0) {
                hdr.h1.setValid();
                hdr.h1 = (h1_t){f1 = 8w42};
                hdr.h2.setValid();
                hdr.h2 = (h2_t){f1 = 8w43,f2 = 8w44};
                hdr.hstructs.s1 = (s1_t){f1 = 8w5};
                hdr.hstructs.s2 = (s2_t){f1 = 8w5,f2 = 8w10};
            } else {
                hdr.h1.setValid();
                hdr.h1 = (h1_t){f1 = 8w52};
                hdr.h2.setValid();
                hdr.h2 = (h2_t){f2 = 8w53,f1 = 8w54};
                hdr.hstructs.s1 = (s1_t){f1 = 8w6};
                hdr.hstructs.s2 = (s2_t){f2 = 8w11,f1 = 8w8};
            }
            hdr.ethernet.dstAddr = (bit<48>)(bit<1>)hdr.h0.isValid();
        }
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

control deparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<h0_t>(hdr.h0);
        pkt.emit<h1_t>(hdr.h1);
        pkt.emit<h2_t>(hdr.h2);
        pkt.emit<hstructs_t>(hdr.hstructs);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
