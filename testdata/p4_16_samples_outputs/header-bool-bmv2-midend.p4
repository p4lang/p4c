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
    @hidden action act() {
        h.f = h.f + 31w1;
        h.x = false;
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

