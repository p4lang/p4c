#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct simple_struct {
    bit<128> a;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Metadata {
}

parser p(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    @name("ingress.pointless_action") action pointless_action() {
    }
    @hidden action struct_assignment_optimization43() {
        hdr.eth_hdr.eth_type = 16w1;
    }
    @hidden table tbl_pointless_action {
        actions = {
            pointless_action();
        }
        const default_action = pointless_action();
    }
    @hidden table tbl_struct_assignment_optimization43 {
        actions = {
            struct_assignment_optimization43();
        }
        const default_action = struct_assignment_optimization43();
    }
    apply {
        tbl_pointless_action.apply();
        tbl_struct_assignment_optimization43.apply();
    }
}

control deparser(packet_out packet, in Headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.eth_hdr);
    }
}

control egress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vrfy(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control update(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
