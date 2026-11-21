#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header h1;
    Header h2;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.h2.data = 32w1;
        transition next;
    }
    state next {
        transition select((bit<1>)(hdr.h2.data == 32w1)) {
            1w1: next_true;
            1w0: next_join;
            default: noMatch;
        }
    }
    state next_true {
        transition next_join;
    }
    state next_join {
        pkt.extract<Header>(hdr.h1);
        hdr.h2.setInvalid();
        transition select(hdr.h1.data) {
            32w0: state0;
            32w1: state1;
            default: accept;
        }
    }
    state state0 {
        hdr.h1.setInvalid();
        transition select(hdr.h2.data) {
            32w2: state1;
            default: next;
        }
    }
    state state1 {
        hdr.h2.data = hdr.h2.data + 32w1;
        hdr.h2.setValid();
        transition select(hdr.h2.data) {
            32w1: state1;
            32w2: state0;
            default: accept;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
