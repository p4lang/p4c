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
    data_t phdr_1_h1;
    data_t hdr_0_h1;
    data_t16 hdr_0_h2;
    data_t hdr_0_h3;
    data_t hdr_0_h4;
    data_t p_shdr_h1;
    state start {
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            default: accept;
        }
    }
    state p0 {
        phdr_0_h1.setInvalid();
        packet.extract<data_t>(phdr_0_h1);
        hdr_0_h1.setInvalid();
        hdr_0_h2.setInvalid();
        hdr_0_h3.setInvalid();
        hdr_0_h4.setInvalid();
        p_shdr_h1.setInvalid();
        packet.extract<data_t>(hdr_0_h1);
        transition select(hdr_0_h1.f) {
            8w1: Subparser_sp1;
            8w2: Subparser_sp2;
            default: p0_0;
        }
    }
    state Subparser_sp1 {
        packet.extract<data_t>(hdr_0_h3);
        packet.extract<data_t>(p_shdr_h1);
        transition p0_0;
    }
    state Subparser_sp2 {
        packet.extract<data_t16>(hdr_0_h2);
        transition p0_0;
    }
    state p0_0 {
        hdr.h1 = hdr_0_h1;
        hdr.h2 = hdr_0_h2;
        hdr.h3 = hdr_0_h3;
        hdr.h4 = hdr_0_h4;
        transition p2;
    }
    state p1 {
        phdr_1_h1.setInvalid();
        packet.extract<data_t>(phdr_1_h1);
        hdr_0_h1.setInvalid();
        hdr_0_h2.setInvalid();
        hdr_0_h3.setInvalid();
        hdr_0_h4.setInvalid();
        p_shdr_h1.setInvalid();
        packet.extract<data_t>(hdr_0_h1);
        transition select(hdr_0_h1.f) {
            8w1: Subparser_sp1_0;
            8w2: Subparser_sp2_0;
            default: p1_0;
        }
    }
    state Subparser_sp1_0 {
        packet.extract<data_t>(hdr_0_h3);
        packet.extract<data_t>(p_shdr_h1);
        transition p1_0;
    }
    state Subparser_sp2_0 {
        packet.extract<data_t16>(hdr_0_h2);
        transition p1_0;
    }
    state p1_0 {
        hdr.h1 = hdr_0_h1;
        hdr.h2 = hdr_0_h2;
        hdr.h3 = hdr_0_h3;
        hdr.h4 = hdr_0_h4;
        transition p2;
    }
    state p2 {
        packet.extract<data_t>(hdr.h4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action parserinlinetest13l98() {
        standard_metadata.egress_spec = 9w2;
    }
    @hidden action parserinlinetest13l100() {
        standard_metadata.egress_spec = 9w3;
    }
    @hidden action parserinlinetest13l102() {
        standard_metadata.egress_spec = 9w10;
    }
    @hidden table tbl_parserinlinetest13l98 {
        actions = {
            parserinlinetest13l98();
        }
        const default_action = parserinlinetest13l98();
    }
    @hidden table tbl_parserinlinetest13l100 {
        actions = {
            parserinlinetest13l100();
        }
        const default_action = parserinlinetest13l100();
    }
    @hidden table tbl_parserinlinetest13l102 {
        actions = {
            parserinlinetest13l102();
        }
        const default_action = parserinlinetest13l102();
    }
    apply {
        if (hdr.h2.isValid()) {
            tbl_parserinlinetest13l98.apply();
        } else if (hdr.h3.isValid()) {
            tbl_parserinlinetest13l100.apply();
        } else {
            tbl_parserinlinetest13l102.apply();
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
