#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
}

enum bit<7> X {
    Zero = 7w0,
    One = 7w1
}

header BITS {
    bit<8> bt;
    int<8> it;
    bool   b;
    X      x;
}

struct headers {
    BITS   bits;
    bit<8> bt;
    int<8> it;
    bool   b;
    X      x;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<BITS>(hdr.bits);
        hdr.bt = hdr.bits.bt;
        hdr.it = hdr.bits.it;
        hdr.b = hdr.bits.b;
        hdr.x = hdr.bits.x;
        transition accept;
    }
}

control mauif(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
    apply {
        if (hdr.b) {
            hdr.x = X.One;
            hdr.bt = 8w1;
            hdr.it = 8s1;
        } else {
            hdr.bt = 8w7;
            hdr.it = -8s1;
            hdr.x = X.Zero;
        }
    }
}

control mau(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control deparse(packet_out pkt, in headers hdr) {
    apply {
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

V1Switch<headers, metadata>(MyParser(), verifyChecksum(), mauif(), mau(), computeChecksum(), deparse()) main;

