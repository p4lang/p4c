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
        pk.extract<Ethernet>(hdr.eth);
        transition accept;
    }
}

control ingress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action act() {
        hdr.eth.setValid();
        hdr.eth.type = 16w0;
        hdr.eth.src = 48w0;
        hdr.eth.dst = 48w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        if (standard_metadata.parser_error == error.PacketTooShort) {
            tbl_act.apply();
        }
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
    apply {
        b.emit<Ethernet>(hdr.eth);
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

V1Switch<parsed_packet_t, local_metadata_t>(parse(), verify_checks(), ingress(), egress(), compute_checksum(), deparser()) main;

