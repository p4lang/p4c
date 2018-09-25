#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct meta_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        bit<16> rand_int;
        random<bit<16>>(rand_int, 16w0, 16w49151);
        if (rand_int < 16w32768) 
            mark_to_drop();
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

