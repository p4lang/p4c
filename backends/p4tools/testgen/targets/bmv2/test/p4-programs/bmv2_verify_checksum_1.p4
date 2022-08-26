#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ethertype;
    bit<16> fcs;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.checksum_error == 1w1) {
            mark_to_drop(standard_metadata);
            exit;
        }
        if (hdr.ethernet.ethertype == 0xAAAA) {
            hdr.ethernet.ethertype = 0xBBBB;
        }
        standard_metadata.egress_spec = 9w1;
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        verify_checksum(hdr.ethernet.isValid(), { hdr.ethernet.dst_addr, hdr.ethernet.src_addr, hdr.ethernet.ethertype}, hdr.ethernet.fcs, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum(hdr.ethernet.isValid(), { hdr.ethernet.dst_addr, hdr.ethernet.src_addr, hdr.ethernet.ethertype}, hdr.ethernet.fcs, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

