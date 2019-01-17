#include <core.p4>
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

control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("MyIngress.h.c1.r") register<bit<16>>(32w8) h_c1_r;
    @name("MyIngress.h.c2.r") register<bit<16>>(32w8) h_c2_r;
    apply {
        h_c1_r.read(hdr.h.x, 32w0);
        h_c2_r.read(hdr.h.x, 32w0);
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

