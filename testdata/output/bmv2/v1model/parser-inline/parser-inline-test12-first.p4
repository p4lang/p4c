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

parser Subparser(packet_in packet, out headers hdr, inout headers2 inout_hdr) {
    headers2 shdr;
    state start {
        packet.extract<data_t>(hdr.h1);
        transition select(hdr.h1.f) {
            8w1: sp1;
            8w2: sp2;
            default: accept;
        }
    }
    state sp1 {
        packet.extract<data_t>(hdr.h3);
        packet.extract<data_t>(shdr.h1);
        transition sp3;
    }
    state sp2 {
        packet.extract<data_t16>(hdr.h2);
        transition accept;
    }
    state sp3 {
        inout_hdr.h1 = shdr.h1;
        transition accept;
    }
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Subparser() subp1;
    Subparser() subp2;
    headers2 phdr;
    state start {
        packet.extract<data_t>(phdr.h1);
        transition select(standard_metadata.ingress_port) {
            9w0: p0;
            9w1: p1;
            9w2: p2;
            9w3: p3;
            default: accept;
        }
    }
    state p0 {
        subp1.apply(packet, hdr, phdr);
        transition p4;
    }
    state p1 {
        subp1.apply(packet, hdr, phdr);
        transition p4;
    }
    state p2 {
        subp2.apply(packet, hdr, phdr);
        transition p4;
    }
    state p3 {
        subp2.apply(packet, hdr, phdr);
        transition p4;
    }
    state p4 {
        hdr.h4 = phdr.h1;
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
