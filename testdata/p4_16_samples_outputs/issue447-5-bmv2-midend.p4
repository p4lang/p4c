#include <core.p4>
#include <v1model.p4>

header S {
    bit<32> size;
}

header H {
    varbit<32> var;
}

struct Parsed_packet {
    S s1;
    H h1;
    H h2;
}

struct Metadata {
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    H tmp_3;
    bit<32> tmp_4;
    H tmp_5;
    bit<32> tmp_6;
    state start {
        pkt.extract<S>(hdr.s1);
        tmp_4 = hdr.s1.size;
        pkt.extract<H>(tmp_3, tmp_4);
        hdr.h1 = tmp_3;
        tmp_6 = hdr.s1.size;
        pkt.extract<H>(tmp_5, tmp_6);
        hdr.h2 = tmp_5;
        transition accept;
    }
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<H>(hdr.h1);
        packet.emit<H>(hdr.h2);
    }
}

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    varbit<32> s;
    @hidden action act() {
        s = hdr.h1.var;
        hdr.h1.var = hdr.h2.var;
        hdr.h2.var = s;
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

control egress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(in Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), ingress(), egress(), uc(), DeparserI()) main;
