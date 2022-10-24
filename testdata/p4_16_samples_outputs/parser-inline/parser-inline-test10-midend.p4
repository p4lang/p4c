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
    data_t h3;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.hdr_0") data_t hdr_0;
    state start {
        packet.extract<data_t>(hdr.h1);
        packet.extract<data_t>(hdr.h2);
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            9w2: p2;
            default: p3;
        }
    }
    state p0 {
        hdr_0 = hdr.h1;
        transition Subparser_start;
    }
    state Subparser_start {
        hdr_0.f = 8w42;
        hdr.h1 = hdr_0;
        transition accept;
    }
    state p1 {
        hdr_0 = hdr.h1;
        transition Subparser_start;
    }
    state p2 {
        hdr_0 = hdr.h2;
        hdr_0.f = 8w42;
        hdr.h2 = hdr_0;
        packet.extract<data_t>(hdr.h3);
        transition accept;
    }
    state p3 {
        hdr_0 = hdr.h2;
        hdr_0.f = 8w42;
        hdr.h2 = hdr_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action parserinlinetest10l59() {
        standard_metadata.egress_spec = 9w1;
    }
    @hidden action parserinlinetest10l61() {
        standard_metadata.egress_spec = 9w10;
    }
    @hidden table tbl_parserinlinetest10l59 {
        actions = {
            parserinlinetest10l59();
        }
        const default_action = parserinlinetest10l59();
    }
    @hidden table tbl_parserinlinetest10l61 {
        actions = {
            parserinlinetest10l61();
        }
        const default_action = parserinlinetest10l61();
    }
    apply {
        if (hdr.h1.f == 8w42) {
            tbl_parserinlinetest10l59.apply();
        } else {
            tbl_parserinlinetest10l61.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.h1);
        packet.emit<data_t>(hdr.h2);
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
