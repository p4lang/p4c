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
    H h;
    S s2;
}

struct Metadata {
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.s1);
        pkt.extract(hdr.h, hdr.s1.size);
        pkt.extract(hdr.s2);
        transition accept;
    }
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit(hdr.h);
        packet.emit(hdr.s2);
    }
}

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    varbit<32> s;
    apply {
        s = hdr.h.var;
        hdr.h.var = s;
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
