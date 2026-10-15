#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_t {
    int<8>  s;
    bit<8>  u;
    bit<16> r1;
    bit<16> r2;
}

struct headers {
    h_t h;
}

struct meta {
}

parser p(packet_in pk, out headers hdr, inout meta m, inout standard_metadata_t sm) {
    state start {
        pk.extract<h_t>(hdr.h);
        transition accept;
    }
}

control vc(inout headers hdr, inout meta m) {
    apply {
    }
}

control ig(inout headers hdr, inout meta m, inout standard_metadata_t sm) {
    apply {
        int<16> a = hdr.h.s ++ hdr.h.u;
        bit<16> b = hdr.h.u ++ hdr.h.s;
        hdr.h.r1 = (bit<16>)a;
        hdr.h.r2 = b;
        sm.egress_spec = 9w0;
    }
}

control eg(inout headers hdr, inout meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control ck(inout headers hdr, inout meta m) {
    apply {
    }
}

control dp(packet_out pk, in headers hdr) {
    apply {
        pk.emit<h_t>(hdr.h);
    }
}

V1Switch<headers, meta>(p(), vc(), ig(), eg(), ck(), dp()) main;
