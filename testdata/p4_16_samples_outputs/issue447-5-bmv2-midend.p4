#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    state start {
        pkt.extract<S>(hdr.s1);
        pkt.extract<H>(hdr.h1, hdr.s1.size);
        pkt.extract<H>(hdr.h2, hdr.s1.size);
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
    @name("ingress.s") varbit<32> s_0;
    @hidden action issue4475bmv2l41() {
        s_0 = hdr.h1.var;
        hdr.h1.var = hdr.h2.var;
        hdr.h2.var = s_0;
    }
    @hidden table tbl_issue4475bmv2l41 {
        actions = {
            issue4475bmv2l41();
        }
        const default_action = issue4475bmv2l41();
    }
    apply {
        tbl_issue4475bmv2l41.apply();
    }
}

control egress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), ingress(), egress(), uc(), DeparserI()) main;
