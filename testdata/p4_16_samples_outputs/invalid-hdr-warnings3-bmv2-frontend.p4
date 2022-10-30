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
        pkt.extract<Header>(hdr.h1);
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.h1") Header h1_0;
    @name("IngressI.h2") Header h2_0;
    apply {
        h1_0.setInvalid();
        h2_0.setInvalid();
        h1_0.setInvalid();
        h2_0.setValid();
        h2_0.data = 32w1;
        switch (hdr.h1.data) {
            32w0: {
                h1_0.setValid();
                h2_0.setInvalid();
            }
            default: {
                h1_0.setValid();
                h2_0.setInvalid();
            }
        }
        hdr.h1.data = h2_0.data;
        switch (hdr.h1.data) {
            32w0: {
                h1_0.setValid();
                h2_0.setValid();
            }
            default: {
                h1_0.setInvalid();
                h2_0.setInvalid();
            }
        }
        hdr.h1.data = h2_0.data;
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
