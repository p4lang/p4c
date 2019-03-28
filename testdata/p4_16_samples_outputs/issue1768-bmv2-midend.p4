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
        packet.lookahead<bit<64>>();
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".markToDrop") action markToDrop() {
        standard_metadata.egress_spec = 9w511;
        standard_metadata.mcast_grp = 16w0;
    }
    @hidden table tbl_markToDrop {
        actions = {
            markToDrop();
        }
        const default_action = markToDrop();
    }
    apply {
        if (standard_metadata.parser_error != error.NoError) 
            tbl_markToDrop.apply();
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

