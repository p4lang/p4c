#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct S {
    bit<32> x;
}

header T {
    bit<32> y;
}

struct H {
    T[2] hstack;
}

struct M {
    bit<32> _s_x0;
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

parser ParserI(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t std_meta) {
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state p0 {
        b.extract<T>(parsedHdr.hstack[32w0]);
        transition select(parsedHdr.hstack[32w1].y) {
            32w0: p01;
            default: accept;
        }
    }
    state p01 {
        transition stateOutOfBound;
    }
    state start {
        transition p0;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
