#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header srcRoute_t {
    bit<1>  bos;
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
    ethernet_t       ethernet;
    srcRoute_t[16w3] srcRoutes;
    ipv4_t           ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("MyParser.index") int<32> index_0;
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".parse_srcRouting") state parse_srcRouting {
        packet.extract<srcRoute_t>(hdr.srcRoutes[32s0]);
        index_0 = index_0 + 32s1;
        transition select(hdr.srcRoutes[32s0].bos) {
            1w1: parse_ipv4;
            default: parse_srcRouting1;
        }
    }
    state parse_srcRouting1 {
        packet.extract<srcRoute_t>(hdr.srcRoutes[32s1]);
        index_0 = index_0 + 32s1;
        transition select(hdr.srcRoutes[32s1].bos) {
            1w1: parse_ipv4;
            default: parse_srcRouting2;
        }
    }
    state parse_srcRouting2 {
        packet.extract<srcRoute_t>(hdr.srcRoutes[32s2]);
        index_0 = index_0 + 32s1;
        transition select(hdr.srcRoutes[32s2].bos) {
            1w1: parse_ipv4;
            default: parse_srcRouting3;
        }
    }
    state parse_srcRouting3 {
        transition stateOutOfBound;
    }
    @name(".start") state start {
        index_0 = 32s0;
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: parse_srcRouting;
            default: accept;
        }
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
