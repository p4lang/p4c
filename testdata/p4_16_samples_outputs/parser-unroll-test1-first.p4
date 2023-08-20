#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 16w0x800;
const bit<16> TYPE_SRCROUTING = 16w0x1234;
const bit<16> MAX_HOPS = 16w3;
typedef bit<9> egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header srcRoute_t {
    bit<1>  bos;
    bit<15> port;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct metadata {
}

struct headers {
    ethernet_t       ethernet;
    srcRoute_t[16w3] srcRoutes;
    ipv4_t           ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    int<32> index;
    @name(".start") state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        index = 32s0;
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x1234: parse_srcRouting;
            default: accept;
        }
    }
    @name(".parse_srcRouting") state parse_srcRouting {
        packet.extract<srcRoute_t>(hdr.srcRoutes[index]);
        index = index + 32s1;
        transition select(hdr.srcRoutes[index + -32s1].bos) {
            1w1: parse_ipv4;
            default: parse_srcRouting;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
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
