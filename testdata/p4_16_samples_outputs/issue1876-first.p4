#include <core.p4>
#include <v1model.p4>

struct H {
}

struct M {
}

parser P(packet_in pkt, out H h, inout M m, inout standard_metadata_t meta) {
    state start {
        transition accept;
    }
}

control G(inout H h, inout M m, inout standard_metadata_t meta) {
    apply {
    }
}

control C(inout H h, inout M m) {
    apply {
    }
}

control D(packet_out pkt, in H h) {
    apply {
    }
}

V1Switch<H, M>(P(), C(), G(), G(), C(), D()) main;

