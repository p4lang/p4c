#include <core.p4>
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
    register<S>(32w100) r;
    apply {
        S s = { 0 };
        r.write(0, s);
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

V1Switch(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

