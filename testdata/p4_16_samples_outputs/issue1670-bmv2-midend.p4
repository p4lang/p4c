#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct switch_metadata_t {
    bit<8> port;
}

header serialized_switch_metadata_t {
    bit<8> _meta_port0;
}

struct parsed_packet_t {
    serialized_switch_metadata_t mirrored_md;
}

struct local_metadata_t {
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action issue1670bmv2l29() {
        h.mirrored_md.setValid();
        h.mirrored_md._meta_port0 = 8w0;
    }
    @hidden table tbl_issue1670bmv2l29 {
        actions = {
            issue1670bmv2l29();
        }
        const default_action = issue1670bmv2l29();
    }
    apply {
        tbl_issue1670bmv2l29.apply();
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply {
    }
}

control verifyChecksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verifyChecksum(), ingress(), egress(), compute_checksum(), deparser()) main;
