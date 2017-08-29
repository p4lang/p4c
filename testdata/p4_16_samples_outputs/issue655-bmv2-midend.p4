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
    @hidden action act() {
        hdr.h.d = hdr.h.d + 16w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control cEgress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

struct tuple_0 {
    bit<16> field;
    bit<16> field_0;
}

control vc(in Parsed_packet hdr, inout Metadata meta) {
    bit<16> tmp_4;
    @name("ck") Checksum16() ck_2;
    apply {
        tmp_4 = ck_2.get<tuple_0>({ hdr.h.d, hdr.h.c });
        if (16w0 != tmp_4) 
            mark_to_drop();
    }
}

struct tuple_1 {
    bit<16> field_1;
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    bit<16> tmp_6;
    @name("ck") Checksum16() ck_3;
    apply {
        tmp_6 = ck_3.get<tuple_1>({ hdr.h.d });
        hdr.h.c = tmp_6;
    }
}

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
