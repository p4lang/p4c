#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header bitvec_hdr {
    bool   x;
    bit<7> pp;
}

struct local_metadata_t {
    bit<8> row0;
}

struct parsed_packet_t {
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action issue1653bmv2l49() {
        clone3<parsed_packet_t>(CloneType.I2E, 32w0, h);
    }
    @hidden table tbl_issue1653bmv2l49 {
        actions = {
            issue1653bmv2l49();
        }
        const default_action = issue1653bmv2l49();
    }
    apply {
        tbl_issue1653bmv2l49.apply();
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

