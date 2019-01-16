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
    apply {
        hdr tmp;
        tmp.x = false;
        h.x = true;
        tmp.f = h.f + 31w1;
        h.f = tmp.f;
        h.x = tmp.x;
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

