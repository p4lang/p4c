#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct h {
}

struct m {
}

parser MyParser(packet_in b, out h hdrs, inout m meta, inout standard_metadata_t std) {
    state start {
        if (std.ingress_port == 0) {
            std.ingress_port = 2;
        }
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
