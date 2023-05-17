#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header data_t {
    bit<8> f;
}

struct headers {
    data_t[3] hs;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<data_t>(hdr.hs[0]);
        hdr.hs.next.setValid();
        hdr.hs.next.f = 8w0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action nextdefuse34() {
        hdr.hs[2].f = 8w10;
    }
    @hidden action nextdefuse36() {
        standard_metadata.egress_spec = 9w3;
    }
    @hidden table tbl_nextdefuse34 {
        actions = {
            nextdefuse34();
        }
        const default_action = nextdefuse34();
    }
    @hidden table tbl_nextdefuse36 {
        actions = {
            nextdefuse36();
        }
        const default_action = nextdefuse36();
    }
    apply {
        if (hdr.hs[1].isValid()) {
            tbl_nextdefuse34.apply();
        } else if (hdr.hs[0].isValid()) {
            tbl_nextdefuse36.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.hs[0]);
        packet.emit<data_t>(hdr.hs[1]);
        packet.emit<data_t>(hdr.hs[2]);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
