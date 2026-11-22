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

parser Subparser(packet_in packet, inout data_t hdr) {
    state start {
        hdr.f = 42;
        transition accept;
    }
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Subparser() p;
    state start {
        packet.extract(hdr.h1);
        packet.extract(hdr.h2);
        transition select(standard_metadata.ingress_port) {
            0: p0;
            1: p1;
            default: p2;
        }
    }
    state p0 {
        p.apply(packet, hdr.h1);
        transition accept;
    }
    state p1 {
        p.apply(packet, hdr.h1);
        transition accept;
    }
    state p2 {
        p.apply(packet, hdr.h2);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.h1.f == 42) {
            standard_metadata.egress_spec = 1;
        } else {
            standard_metadata.egress_spec = 10;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
