#include <core.p4>
#include <v1model.p4>

struct B8 {
    bit<64> bits;
}

struct headers {
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.lookahead<B8>();
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".markToDrop") action markToDrop(inout standard_metadata_t standard_metadata_1) {
        standard_metadata_1.egress_spec = 9w511;
        standard_metadata_1.mcast_grp = 16w0;
    }
    apply {
        if (standard_metadata.parser_error != error.NoError) 
            markToDrop(standard_metadata);
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

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

