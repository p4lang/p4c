#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct pkt_t {
    ethernet_t ethernet;
}

struct meta_t{

}

parser ParserImpl(packet_in packet, out pkt_t pkt, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(pkt.ethernet);
        transition select(pkt.ethernet.etherType) {
            default: accept;
        }
    }

}



struct tuple_0 {
    bit<16> tmp1;
}

control egress(inout pkt_t pkt, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    bit<16> tmp;
    apply {
        hash<bit<16>, bit<9>, tuple_0, bit<18>>(tmp, HashAlgorithm.crc32, 9w0, { pkt.ethernet.etherType }, 18w512);
        pkt.ethernet.etherType = tmp;
    }
}

control ingress(inout pkt_t pkt, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in pkt_t pkt) {
    apply {
        packet.emit<ethernet_t>(pkt.ethernet);
    }
}

control verifyChecksum(inout pkt_t pkt, inout meta_t meta) {
    apply {
    }
}

control computeChecksum(inout pkt_t pkt, inout meta_t meta) {
    apply {
    }
}

V1Switch<pkt_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
