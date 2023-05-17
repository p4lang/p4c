#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h1 {
    bit<8> f1;
}

struct h {
    h1[3] h;
}

struct m {
    bit<8> h_count;
}

parser MyParser(packet_in b, out h hdrs, inout m meta, inout standard_metadata_t std) {
    @name("MyParser.l") h l_0;
    state start {
        l_0.h[0].setInvalid();
        l_0.h[1].setInvalid();
        l_0.h[2].setInvalid();
        l_0 = b.lookahead<h>();
        transition select(l_0.h[0].f1, l_0.h[1].f1, l_0.h[2].f1) {
            (8w1, default, default): one;
            (8w0, 8w1, default): two;
            (8w0, 8w0, 8w1): three;
            default: many;
        }
    }
    state one {
        b.extract<h1>(hdrs.h.next);
        meta.h_count = 8w1;
        transition accept;
    }
    state two {
        b.extract<h1>(hdrs.h.next);
        b.extract<h1>(hdrs.h.next);
        meta.h_count = 8w2;
        transition accept;
    }
    state three {
        b.extract<h1>(hdrs.h.next);
        b.extract<h1>(hdrs.h.next);
        b.extract<h1>(hdrs.h.next);
        meta.h_count = 8w2;
        transition accept;
    }
    state many {
        meta.h_count = 8w255;
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

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
