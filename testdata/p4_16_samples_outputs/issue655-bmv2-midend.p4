#include <core.p4>
#define V1MODEL_VERSION 20180101
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
        packet.emit<H>(hdr.h);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    @hidden action issue655bmv2l32() {
        hdr.h.d = hdr.h.d + 16w1;
    }
    @hidden table tbl_issue655bmv2l32 {
        actions = {
            issue655bmv2l32();
        }
        const default_action = issue655bmv2l32();
    }
    apply {
        tbl_issue655bmv2l32.apply();
    }
}

control cEgress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

struct tuple_0 {
    bit<16> field;
}

control vc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(true, { hdr.h.d }, hdr.h.c, HashAlgorithm.csum16);
    }
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(true, { hdr.h.d }, hdr.h.c, HashAlgorithm.csum16);
    }
}

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

