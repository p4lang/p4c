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
    apply {
        hdr.h.d = hdr.h.d + 16w1;
    }
}

control cEgress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(in Parsed_packet hdr, inout Metadata meta) {
    bit<16> tmp_0;
    bit<16> tmp;
    @name("ck") Checksum16() ck_0;
    apply {
        tmp = ck_0.get<tuple<bit<16>, bit<16>>>({ hdr.h.d, hdr.h.c });
        tmp_0 = tmp;
        if (16w0 != tmp_0) 
            mark_to_drop();
    }
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    bit<16> tmp_1;
    bit<16> tmp_2;
    @name("ck") Checksum16() ck_1;
    apply {
        tmp_2 = ck_1.get<tuple<bit<16>>>({ hdr.h.d });
        tmp_1 = tmp_2;
        hdr.h.c = tmp_1;
    }
}

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
