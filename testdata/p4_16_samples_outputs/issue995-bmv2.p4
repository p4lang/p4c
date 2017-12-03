#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    bit<16> transition_taken;
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.srcAddr, hdr.ethernet.dstAddr) {
            (0x12f0000, 0x456): a1;
            (0x12f0000 &&& 0xffff0000, 0x456): a2;
            (0x12f0000, 0x456 &&& 0xfff): a3;
            (0x12f0000 &&& 0xffff0000, 0x456 &&& 0xfff): a4;
            default: a5;
        }
    }
    state a1 {
        meta.transition_taken = 1;
        transition accept;
    }
    state a2 {
        meta.transition_taken = 2;
        transition accept;
    }
    state a3 {
        meta.transition_taken = 3;
        transition accept;
    }
    state a4 {
        meta.transition_taken = 4;
        transition accept;
    }
    state a5 {
        meta.transition_taken = 5;
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        hdr.ethernet.etherType = meta.transition_taken;
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

