#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<31> f;
    bool    x;
}

struct metadata {
}

parser parserI(packet_in pkt, out hdr h, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<hdr>(h);
        transition accept;
    }
}

control cIngressI(inout hdr h, inout metadata meta, inout standard_metadata_t stdmeta) {
    hdr tmp_0;
    apply {
        tmp_0.x = false;
        tmp_0.f = h.f + 31w1;
        h.f = tmp_0.f;
        h.x = tmp_0.x;
    }
}

control cEgress(inout hdr h, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout hdr h, inout metadata meta) {
    apply {
    }
}

control uc(inout hdr h, inout metadata meta) {
    apply {
    }
}

control DeparserI(packet_out packet, in hdr h) {
    apply {
        packet.emit<hdr>(h);
    }
}

V1Switch<hdr, metadata>(parserI(), vc(), cIngressI(), cEgress(), uc(), DeparserI()) main;

