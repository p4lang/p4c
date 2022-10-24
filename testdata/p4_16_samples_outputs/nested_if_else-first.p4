#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 16w0x800;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
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
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            default: accept;
        }
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bool c = true;
    bool c1 = true;
    bool c2 = true;
    bool c3 = true;
    action if_testing(out bit<16> value, in bit<8> offset) {
        value = 16w0;
        bit<16> x = hdr.ipv4.identification;
        bit<16> y = hdr.ipv4.hdrChecksum;
        bit<16> z = hdr.ipv4.totalLen;
        c = hdr.ipv4.identification > 16w0;
        c1 = hdr.ipv4.identification > 16w1;
        c2 = hdr.ipv4.identification > 16w2;
        c3 = hdr.ipv4.identification > 16w3;
        if (c) {
            x = 16w1;
            if (c1) {
                x = x + 16w2;
            } else {
                x = x + 16w3;
            }
            x = x + 16w4;
        } else if (c2) {
            x = x + 16w5;
        } else {
            x = x + 16w6;
        }
        value = z + x + y;
    }
    action ipv4_forward() {
        if_testing(hdr.ipv4.totalLen, hdr.ipv4.protocol);
    }
    action drop() {
    }
    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            ipv4_forward();
            drop();
            NoAction();
        }
        size = 1024;
        default_action = NoAction();
    }
    apply {
        ipv4_lpm.apply();
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
