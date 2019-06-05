#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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

struct headers_t {
    ethernet_t ethernet_1;
    ipv4_t     ipv4_1;
    ethernet_t ethernet_2;
    ipv4_t     ipv4_2;
}

struct metadata_t {
}

extern E {
    E();
    void g();
}

parser subParser(packet_in packet, inout ethernet_t eth, inout ipv4_t ipv4) {
    value_set<bit<16>>(8) ipv4_ethertypes;
    state start {
        packet.extract(eth);
        transition select(eth.etherType) {
            ipv4_ethertypes: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(ipv4);
        transition accept;
    }
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    subParser() p1;
    subParser() p2;
    state start {
        p1.apply(packet, hdr.ethernet_1, hdr.ipv4_1);
        p2.apply(packet, hdr.ethernet_2, hdr.ipv4_2);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
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
        packet.emit(hdr.ethernet_1);
        packet.emit(hdr.ipv4_1);
        packet.emit(hdr.ethernet_2);
        packet.emit(hdr.ipv4_2);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

