// Test of subparser inlining with following characteristics:
// - one subparser instance
// - two invocations of the same instance with the same arguments
// - a statement (identical) after both invocations
// - transition to the same state after both invocations

#include <v1model.p4>

struct metadata { }

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

parser Subparser(      packet_in packet,
                 out   headers   hdr,
                 inout headers2  inout_hdr) {
    headers2 shdr;

    state start {
        packet.extract(hdr.h1);
        transition select(hdr.h1.f) {
            1: sp1;
            2: sp2;
            default: accept;
        }
    }

    state sp1 {
        packet.extract(hdr.h3);
        packet.extract(shdr.h1);
        transition sp3;
    }

    state sp2 {
        packet.extract(hdr.h2);
        transition accept;
    }

    state sp3 {
        inout_hdr.h1 = shdr.h1;
        transition accept;
    }
}

parser ParserImpl(      packet_in           packet,
                  out   headers             hdr,
                  inout metadata            meta,
                  inout standard_metadata_t standard_metadata) {
    Subparser() p;
    headers2 phdr;

    state start {
        packet.extract(phdr.h1);
        transition select(standard_metadata.ingress_port) {
            0: p0;
            1: p1;
            default: accept;
        }
    }

    state p0 {
        p.apply(packet, hdr, phdr);
        packet.extract(hdr.h4);
        transition accept;
    }

    state p1 {
        p.apply(packet, hdr, phdr);
        packet.extract(hdr.h4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.h2.isValid()) {
            standard_metadata.egress_spec = 2;
        } else if (hdr.h3.isValid()) {
            standard_metadata.egress_spec = 3;
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