#include <core.p4>
#include <v1model.p4>

header inboxes {
    bit<48> cornstarchs;
    int<16> khans;
}

struct metadata {
}

struct headers {
    @name(".extirpations") 
    inboxes extirpations;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_extirpations") state parse_extirpations {
        packet.extract(hdr.extirpations);
        transition select(hdr.extirpations.khans) {
            16s1024: accept;
        }
    }
    @name(".start") state start {
        transition parse_extirpations;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.extirpations);
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

