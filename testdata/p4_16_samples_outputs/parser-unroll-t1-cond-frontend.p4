#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> MacAddress;
typedef bit<32> ip4Addr_t;
typedef bit<128> IPv6Address;
header ethernet_h {
    MacAddress dst;
    MacAddress src;
    bit<16>    type;
}

header ipv6_h {
    bit<4>      version;
    bit<8>      tc;
    bit<20>     fl;
    bit<16>     plen;
    bit<8>      nh;
    bit<8>      hl;
    IPv6Address src;
    IPv6Address dst;
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

header tcp_h {
    bit<16> sport;
    bit<16> dport;
    bit<32> seq;
    bit<32> ack;
    bit<4>  dataofs;
    bit<4>  reserved;
    bit<8>  flags;
    bit<16> window;
    bit<16> chksum;
    bit<16> urgptr;
}

header udp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> hdrLength;
    bit<16> chksum;
}

struct headers {
    ethernet_h ethernet;
    ethernet_h ethernet2;
    ipv4_t     ipv4;
    ipv6_h     ipv6;
    udp_h      udp;
    tcp_h      tcp;
}

struct metadata {
    bool test;
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out pkt, in headers hdr) {
    apply {
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

parser MyParser_0(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        pkt.extract<ethernet_h>(hdr.ethernet);
        transition select(hdr.ethernet.type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv6 {
        pkt.extract<ipv6_h>(hdr.ipv6);
        transition select(hdr.ipv6.nh) {
            8w0x11: parseOther;
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            8w0x11: parseOther;
            default: accept;
        }
    }
    state parseOther {
        transition accept;
    }
    state parse_tcp {
        pkt.extract<tcp_h>(hdr.tcp);
        transition accept;
    }
}

V1Switch<headers, metadata>(MyParser_0(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
