#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header[2] h1;
    Header[2] h2;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        pkt.extract<Header>(hdr.h1[0]);
        hdr.h1[0].data = 32w1;
        hdr.h1[1].data = 32w1;
        transition init;
    }
    state init {
        pkt.extract<Header>(hdr.h1[1]);
        hdr.h1[0].data = 32w1;
        hdr.h1[1].data = 32w1;
        transition next;
    }
    state next {
        hdr.h1[0].setInvalid();
        pkt.extract<Header>(hdr.h1.next);
        hdr.h1[0].data = 32w1;
        hdr.h1[1].data = 32w1;
        hdr.h1[0].setInvalid();
        transition select(hdr.h1[1].data) {
            32w0: init;
            default: accept;
        }
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    H h;
    H h_copy;
    H h_copy2;
    action validateHeader(inout Header hd) {
        hd.setValid();
    }
    action invalidateHeader(inout Header hd) {
        hd.setInvalid();
    }
    action invalidateStack(inout Header[2] stack) {
        invalidateHeader(stack[0]);
        invalidateHeader(stack[1]);
    }
    apply {
        validateHeader(h.h1[0]);
        h.h1[0].data = 32w1;
        h.h1[1] = h.h1[0];
        h.h1[1].data = 32w1;
        h.h2 = h.h1;
        h.h2[0].data = 32w1;
        h.h2[1].data = 32w1;
        h_copy = h;
        h_copy.h1[0].data = 32w1;
        h_copy.h1[1].data = 32w1;
        h_copy.h2[0].data = 32w1;
        h_copy.h2[1].data = 32w1;
        invalidateHeader(h.h2[0]);
        h_copy2 = (H){h1 = h.h1,h2 = h.h2};
        h_copy2.h1[0].data = 32w1;
        h_copy2.h1[1].data = 32w1;
        h_copy2.h2[0].data = 32w1;
        h_copy2.h2[1].data = 32w1;
        bit<1> i = 1w1;
        h_copy2.h2[i] = h.h2[0];
        h_copy2.h1[0].data = 32w1;
        h_copy2.h1[1].data = 32w1;
        h_copy2.h2[0].data = 32w1;
        h_copy2.h2[1].data = 32w1;
        validateHeader(h.h2[i]);
        h_copy2.h2[i] = h.h2[0];
        h_copy2.h1[0].data = 32w1;
        h_copy2.h1[1].data = 32w1;
        h_copy2.h2[0].data = 32w1;
        h_copy2.h2[1].data = 32w1;
        invalidateStack(h.h1);
        invalidateStack(h_copy.h1);
        h_copy.h1[0] = h.h1[i];
        h_copy.h1[0].data = 32w1;
        h_copy.h1[1].data = 32w1;
        h.h1[1].setValid();
        h_copy.h1[0] = h.h1[i];
        h_copy.h1[0].data = 32w1;
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
