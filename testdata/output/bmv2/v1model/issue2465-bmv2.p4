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
    state start {
        h l;
        l = b.lookahead<h>();
        transition select(l.h[0].f1, l.h[1].f1, l.h[2].f1) {
            (1, default, default): one;
            (0, 1, default): two;
            (0, 0, 1): three;
            default: many;
        }
    }
    state one {
        b.extract(hdrs.h.next);
        meta.h_count = 1;
        transition accept;
    }
    state two {
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        meta.h_count = 2;
        transition accept;
    }
    state three {
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        meta.h_count = 2;
        transition accept;
    }
    state many {
        meta.h_count = 255;
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
