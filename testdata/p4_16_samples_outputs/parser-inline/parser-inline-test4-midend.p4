#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

header data_t {
    bit<8> f;
}

header data_t16 {
    bit<16> f;
}

struct headers {
    data_t   h1;
    data_t16 h2;
    data_t   h3;
    data_t   h4;
}

struct headers2 {
    data_t h1;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    data_t phdr_0_h1;
    data_t p_shdr_h1;
    state start {
        phdr_0_h1.setInvalid();
        packet.extract<data_t>(phdr_0_h1);
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            default: accept;
        }
    }
    state p0 {
        hdr.h1.setInvalid();
        hdr.h2.setInvalid();
        hdr.h3.setInvalid();
        hdr.h4.setInvalid();
        p_shdr_h1.setInvalid();
        packet.extract<data_t>(hdr.h1);
        transition select(hdr.h1.f) {
            8w1: Subparser_sp1;
            8w2: Subparser_sp2;
            default: p0_0;
        }
    }
    state Subparser_sp1 {
        packet.extract<data_t>(hdr.h3);
        packet.extract<data_t>(p_shdr_h1);
        phdr_0_h1 = p_shdr_h1;
        transition p0_0;
    }
    state Subparser_sp2 {
        packet.extract<data_t16>(hdr.h2);
        transition p0_0;
    }
    state p0_0 {
        transition select(phdr_0_h1.f) {
            8w1: p2;
            default: accept;
        }
    }
    state p1 {
        hdr.h1.setInvalid();
        hdr.h2.setInvalid();
        hdr.h3.setInvalid();
        hdr.h4.setInvalid();
        p_shdr_h1.setInvalid();
        packet.extract<data_t>(hdr.h1);
        transition select(hdr.h1.f) {
            8w1: Subparser_sp1_0;
            8w2: Subparser_sp2_0;
            default: p1_0;
        }
    }
    state Subparser_sp1_0 {
        packet.extract<data_t>(hdr.h3);
        packet.extract<data_t>(p_shdr_h1);
        phdr_0_h1 = p_shdr_h1;
        transition p1_0;
    }
    state Subparser_sp2_0 {
        packet.extract<data_t16>(hdr.h2);
        transition p1_0;
    }
    state p1_0 {
        transition select(phdr_0_h1.f) {
            8w1: reject;
            default: accept;
        }
    }
    state p2 {
        hdr.h4 = phdr_0_h1;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action parserinlinetest4l101() {
        standard_metadata.egress_spec = 9w2;
    }
    @hidden action parserinlinetest4l103() {
        standard_metadata.egress_spec = 9w3;
    }
    @hidden action parserinlinetest4l105() {
        standard_metadata.egress_spec = 9w10;
    }
    @hidden table tbl_parserinlinetest4l101 {
        actions = {
            parserinlinetest4l101();
        }
        const default_action = parserinlinetest4l101();
    }
    @hidden table tbl_parserinlinetest4l103 {
        actions = {
            parserinlinetest4l103();
        }
        const default_action = parserinlinetest4l103();
    }
    @hidden table tbl_parserinlinetest4l105 {
        actions = {
            parserinlinetest4l105();
        }
        const default_action = parserinlinetest4l105();
    }
    apply {
        if (hdr.h2.isValid()) {
            tbl_parserinlinetest4l101.apply();
        } else if (hdr.h3.isValid()) {
            tbl_parserinlinetest4l103.apply();
        } else {
            tbl_parserinlinetest4l105.apply();
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
        packet.emit<data_t16>(hdr.h2);
        packet.emit<data_t>(hdr.h3);
        packet.emit<data_t>(hdr.h4);
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
