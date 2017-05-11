#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct my_headers_t {
    ethernet_t ethernet;
}

struct my_metadata_t {
    error parser_error;
}

parser MyParser(packet_in packet, out my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(in my_headers_t hdr, inout my_metadata_t meta) {
    apply {
    }
}

control MyIngress(inout my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyEgress(inout my_headers_t hdr, inout my_metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout my_headers_t hdr, inout my_metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in my_headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
