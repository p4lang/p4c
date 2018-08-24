#include <core.p4>
#include <v1model.p4>

header Ethernet {
    bit<48> src;
    bit<48> dst;
    bit<16> type;
}

struct parsed_packet_t {
    Ethernet eth;
}

struct local_metadata_t {
}

parser parse(packet_in pk, out parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        pk.extract(hdr.eth);
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.parser_error == error.PacketTooShort) {
            hdr.eth.setValid();
            hdr.eth.type = 0;
            hdr.eth.src = 0;
            hdr.eth.dst = 0;
        }
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
    apply {
        b.emit(hdr);
    }
}

control verify_checks(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum(inout parsed_packet_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

V1Switch(parse(), verify_checks(), ingress(), egress(), compute_checksum(), deparser()) main;

