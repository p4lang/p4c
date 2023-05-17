#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Metadata {
}

struct S {
    bit<32> f;
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    @name("Ing.s") S s_0;
    @name("Ing.r") register<S>(32w100) r_0;
    @hidden action issue907bmv2l22() {
        s_0.f = 32w0;
        r_0.write(32w0, s_0);
    }
    @hidden table tbl_issue907bmv2l22 {
        actions = {
            issue907bmv2l22();
        }
        const default_action = issue907bmv2l22();
    }
    apply {
        tbl_issue907bmv2l22.apply();
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
