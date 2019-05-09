#include <core.p4>
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
    bitvec_hdr bh;
    apply {
        bh.x = h.bvh0.x;
        clone3(CloneType.I2E, 0, h);
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

V1Switch(parse(), verifyChecksum(), ingress(), egress(), compute_checksum(), deparser()) main;

