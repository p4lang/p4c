#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct H {
}

struct M {
}

control DeparserI(packet_out packet, in H hdr) {
    apply {
    }
}

parser parserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t stdmeta) {
    bit<16> tmp;
    bit<112> tmp_0;
    state start {
        tmp_0 = pkt.lookahead<bit<112>>();
        tmp = tmp_0[111:96];
        transition select(tmp) {
            16w0x1000 &&& 16w0x1000: accept;
        }
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

