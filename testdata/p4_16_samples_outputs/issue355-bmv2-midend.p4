#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct H {
}

struct M {
}

control DeparserI(packet_out packet, in H hdr) {
    apply {
    }
}

parser parserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t stdmeta) {
    ethernet_t tmp_0;
    bit<112> tmp;
    state start {
        tmp = pkt.lookahead<bit<112>>();
        tmp_0.setValid();
        tmp_0.dstAddr = tmp[111:64];
        tmp_0.srcAddr = tmp[63:16];
        tmp_0.etherType = tmp[15:0];
        transition select(tmp_0.etherType) {
            16w0x1000 &&& 16w0x1000: accept;
            default: noMatch;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control cIngress(inout H hdr, inout M meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control cEgress(inout H hdr, inout M meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout H hdr, inout M meta) {
    apply {
    }
}

control uc(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch<H, M>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

