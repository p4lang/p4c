#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header srcRoute_t {
    bit<2>  bos;
    bit<15> port;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    srcRoute_t srcRoutes1;
    srcRoute_t srcRoutes2;
    srcRoute_t srcRoutes3;
    ipv4_t     ipv4;
    bit<32>    index;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        hdr.index = 32w0;
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0: last;
            16w1: access1;
            16w2: access2;
            16w3: access3;
            default: accept;
        }
    }
    state last {
        hdr.index = hdr.index + 32w1;
        transition accept;
    }
    state access1 {
        hdr.index = hdr.index + 32w1;
        packet.extract<srcRoute_t>(hdr.srcRoutes1);
        transition last;
    }
    state access2 {
        hdr.index = hdr.index + 32w1;
        packet.extract<srcRoute_t>(hdr.srcRoutes2);
        transition access1;
    }
    state access3 {
        hdr.index = hdr.index + 32w1;
        packet.extract<srcRoute_t>(hdr.srcRoutes3);
        transition access2;
    }
}

control mau(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control deparse(packet_out pkt, in headers hdr) {
    apply {
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), verifyChecksum(), mau(), mau(), computeChecksum(), deparse()) main;
