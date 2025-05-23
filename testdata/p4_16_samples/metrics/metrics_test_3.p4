#include <core.p4>
#include <v1model.p4>

// Headers
header Ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  tos;
    bit<16> length;
    bit<16> id;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> checksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header TCP_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNum;
    bit<32> ackNum;
    bit<4>  dataOffset;
    bit<6>  flags;
    bit<6>  reserved;
    bit<16> windowSize;
    bit<16> checksum;
    bit<16> urgentPointer;
}

header UDP_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length;
    bit<16> checksum;
}

struct headers {
    Ethernet_t ethernet;
    IPv4_t ipv4;
    TCP_t tcp;
    UDP_t udp;
}

struct metadata {
    bit<9> ingress_port;
    bit<8> hop_count;
}

parser MyParser(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t sm) {

    state start {
        pkt.extract(hdr.ethernet);
        hdr.ethernet.dstAddr = 0xAA11BB22CC33;
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        hdr.ipv4.tos = 16;
        transition select(hdr.ipv4.protocol, meta.hop_count) {
            (0xFD, 0): parse_tcp;
            (0xFD, 1): parse_udp;
            default: accept;
        }
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        hdr.tcp.flags = 0x02;
        transition parse_ipv4; // Loop back
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        hdr.udp.length = 10;
        transition parse_ipv4; // Loop back again
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) { apply { } }
control MyComputeChecksum(inout headers hdr, inout metadata meta) { apply { } }
control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) { apply { } }
control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) { apply { } }

control MyDeparser(packet_out pkt, in headers hdr) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.udp);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

