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
            default: p1;
        }
    }
    state p0 {
        hdr_0 = hdr.h1;
        hdr_0.f = 8w42;
        hdr.h1 = hdr_0;
        transition accept;
    }
    state p1 {
        hdr_0 = hdr.h2;
        hdr_0.f = 8w42;
        hdr.h2 = hdr_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action parserinlinetest7l50() {
        standard_metadata.egress_spec = 9w1;
    }
    @hidden action parserinlinetest7l52() {
        standard_metadata.egress_spec = 9w10;
    }
    @hidden table tbl_parserinlinetest7l50 {
        actions = {
            parserinlinetest7l50();
        }
        const default_action = parserinlinetest7l50();
    }
    @hidden table tbl_parserinlinetest7l52 {
        actions = {
            parserinlinetest7l52();
        }
        const default_action = parserinlinetest7l52();
    }
    apply {
        if (hdr.h1.f == 8w42) {
            tbl_parserinlinetest7l50.apply();
        } else {
            tbl_parserinlinetest7l52.apply();
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
