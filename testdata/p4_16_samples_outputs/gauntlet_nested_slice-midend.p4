#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct Headers {
    H h;
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    @name("Ing.n") bit<8> n_0;
    @hidden action gauntlet_nested_slice57() {
        n_0 = 8w0b11111111;
        n_0[7:4] = 4w0;
        headers.h.a = n_0;
    }
    @hidden table tbl_gauntlet_nested_slice57 {
        actions = {
            gauntlet_nested_slice57();
        }
        const default_action = gauntlet_nested_slice57();
    }
    apply {
        tbl_gauntlet_nested_slice57.apply();
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
    }
}

control DP(packet_out b, in Headers p) {
    apply {
    }
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

