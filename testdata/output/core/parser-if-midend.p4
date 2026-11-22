#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct h {
}

struct m {
}

parser MyParser(packet_in b, out h hdrs, inout m meta, inout standard_metadata_t std) {
    state start {
        transition select((bit<1>)(std.ingress_port == 9w0)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
        }
    }
    state start_true {
        std.ingress_port = 9w2;
        transition start_join;
    }
    state start_join {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
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

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
