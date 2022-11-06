#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Parsed_packet {
    Ethernet_h ethernet;
}

struct Metadata {
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        transition accept;
    }
}

struct tuple_0 {
    bit<48> f0;
}

control cIngress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    @hidden action issue4301bmv2l44() {
        digest<tuple_0>(32w5, (tuple_0){f0 = hdr.ethernet.srcAddr});
        hdr.ethernet.srcAddr = 48w0;
    }
    @hidden table tbl_issue4301bmv2l44 {
        actions = {
            issue4301bmv2l44();
        }
        const default_action = issue4301bmv2l44();
    }
    apply {
        tbl_issue4301bmv2l44.apply();
    }
}

control cEgress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
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

V1Switch<Parsed_packet, Metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
