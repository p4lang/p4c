#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Header {
    bit<16> x;
}

struct headers {
    Header h;
}

struct metadata {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        hdr.h.x = 16w0;
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control C(inout headers hdr, inout metadata meta)(bool b) {
    register<bit<16>>(32w8) r;
    apply {
        r.read(hdr.h.x, 32w0);
    }
}

control H(inout headers hdr, inout metadata meta) {
    C(true) c1;
    C(false) c2;
    apply {
        c1.apply(hdr, meta);
        c2.apply(hdr, meta);
    }
}

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    H() h;
    apply {
        h.apply(hdr, meta);
    }
}

control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

