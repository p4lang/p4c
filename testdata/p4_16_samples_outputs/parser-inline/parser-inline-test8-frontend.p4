#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header data_t {
    bit<8> f;
}

struct headers {
    data_t h1;
    data_t h2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.hdr_0") data_t hdr_0;
    state start {
        packet.extract<data_t>(hdr.h1);
        packet.extract<data_t>(hdr.h2);
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            default: p2;
        }
    }
    state p0 {
        hdr_0 = hdr.h1;
        transition Subparser_start;
    }
    state Subparser_start {
        hdr_0.f = 8w42;
        transition p0_0;
    }
    state p0_0 {
        hdr.h1 = hdr_0;
        transition accept;
    }
    state p1 {
        hdr_0 = hdr.h1;
        transition Subparser_start;
    }
    state p2 {
        hdr_0 = hdr.h2;
        transition Subparser_start_0;
    }
    state Subparser_start_0 {
        hdr_0.f = 8w42;
        transition p2_0;
    }
    state p2_0 {
        hdr.h2 = hdr_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.h1.f == 8w42) {
            standard_metadata.egress_spec = 9w1;
        } else {
            standard_metadata.egress_spec = 9w10;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
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
