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
        pkt.extract(hdr.ethernet);
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
            hdr.h1 = {  };
            hdr.h2 = {  };
            hdr.h0 = { 4 };
            hdr.h1 = { 1, 2 };
            hdr.h2 = { 3 };
            hdr.hstructs.s0 = { 1 };
            hdr.hstructs.s1 = { 1, 2 };
            hdr.hstructs.s2 = { 3 };
            hdr.h1 = {f2 = 5,f1 = 2};
            hdr.h1 = {f2 = 5};
            hdr.hstructs.s1 = {f2 = 5,f1 = 2};
            hdr.hstructs.s1 = {f2 = 5};
            hdr.h2 = {f2 = 5};
            hdr.hstructs.s2 = {f2 = 5};
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
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.h0);
        pkt.emit(hdr.h1);
        pkt.emit(hdr.h2);
        pkt.emit(hdr.hstructs);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
