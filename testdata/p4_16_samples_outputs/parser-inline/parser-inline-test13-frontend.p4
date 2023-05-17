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
    @name("ParserImpl.phdr") headers2 phdr_0;
    @name("ParserImpl.phdr") headers2 phdr_1;
    @name("ParserImpl.hdr_0") headers hdr_0;
    @name("ParserImpl.p.shdr") headers2 p_shdr;
    state start {
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            default: accept;
        }
    }
    state p0 {
        phdr_0.h1.setInvalid();
        packet.extract<data_t>(phdr_0.h1);
        hdr_0.h1.setInvalid();
        hdr_0.h2.setInvalid();
        hdr_0.h3.setInvalid();
        hdr_0.h4.setInvalid();
        transition Subparser_start;
    }
    state Subparser_start {
        p_shdr.h1.setInvalid();
        packet.extract<data_t>(hdr_0.h1);
        transition select(hdr_0.h1.f) {
            8w1: Subparser_sp1;
            8w2: Subparser_sp2;
            default: p0_0;
        }
    }
    state Subparser_sp1 {
        packet.extract<data_t>(hdr_0.h3);
        packet.extract<data_t>(p_shdr.h1);
        transition p0_0;
    }
    state Subparser_sp2 {
        packet.extract<data_t16>(hdr_0.h2);
        transition p0_0;
    }
    state p0_0 {
        hdr = hdr_0;
        transition p2;
    }
    state p1 {
        phdr_1.h1.setInvalid();
        packet.extract<data_t>(phdr_1.h1);
        hdr_0.h1.setInvalid();
        hdr_0.h2.setInvalid();
        hdr_0.h3.setInvalid();
        hdr_0.h4.setInvalid();
        transition Subparser_start_0;
    }
    state Subparser_start_0 {
        p_shdr.h1.setInvalid();
        packet.extract<data_t>(hdr_0.h1);
        transition select(hdr_0.h1.f) {
            8w1: Subparser_sp1_0;
            8w2: Subparser_sp2_0;
            default: p1_0;
        }
    }
    state Subparser_sp1_0 {
        packet.extract<data_t>(hdr_0.h3);
        packet.extract<data_t>(p_shdr.h1);
        transition p1_0;
    }
    state Subparser_sp2_0 {
        packet.extract<data_t16>(hdr_0.h2);
        transition p1_0;
    }
    state p1_0 {
        hdr = hdr_0;
        transition p2;
    }
    state p2 {
        packet.extract<data_t>(hdr.h4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.h2.isValid()) {
            standard_metadata.egress_spec = 9w2;
        } else if (hdr.h3.isValid()) {
            standard_metadata.egress_spec = 9w3;
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
