#include <core.p4>
#include <v1model.p4>

header H {
    bit<16> d;
    bit<16> c;
}

struct Parsed_packet {
    H h;
}

struct Metadata {

}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit(hdr.h);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.h);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
        hdr.h.d = hdr.h.d + 1;
    }
}

control cEgress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {}
}

control vc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
        verify_checksum(true, { hdr.h.d }, hdr.h.c, HashAlgorithm.csum16);
    }
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
        update_checksum(true, { hdr.h.d }, hdr.h.c, HashAlgorithm.csum16);
    }
}

V1Switch(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
