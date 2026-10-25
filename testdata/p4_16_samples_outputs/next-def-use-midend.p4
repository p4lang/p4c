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
    @hidden action nextdefuse40() {
        hdr.hs[2].f = 8w10;
    }
    @hidden action nextdefuse42() {
        standard_metadata.egress_spec = 9w3;
    }
    @hidden table tbl_nextdefuse40 {
        actions = {
            nextdefuse40();
        }
        const default_action = nextdefuse40();
    }
    @hidden table tbl_nextdefuse42 {
        actions = {
            nextdefuse42();
        }
        const default_action = nextdefuse42();
    }
    apply {
        if (hdr.hs[1].isValid()) {
            tbl_nextdefuse40.apply();
        } else if (hdr.hs[0].isValid()) {
            tbl_nextdefuse42.apply();
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
