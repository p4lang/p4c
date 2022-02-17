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
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.h1") Header h1_0;
    @name("IngressI.h2") Header h2_0;
    @name("IngressI.s1") H s1_0;
    @name("IngressI.s2") H s2_0;
    @name("IngressI.s") H s_0;
    @name("IngressI.invalid_H") action invalid_H() {
        s_0 = hdr;
        s_0.h1.setInvalid();
        s_0.h2.setInvalid();
        hdr = s_0;
    }
    apply {
        h1_0.setInvalid();
        h2_0.setInvalid();
        s1_0.h1.setInvalid();
        s1_0.h2.setInvalid();
        s2_0.h1.setInvalid();
        s2_0.h2.setInvalid();
        h1_0.setValid();
        h1_0 = (Header){data = 32w1};
        if (hdr.h1.data == 32w0) {
            h2_0 = h1_0;
        } else {
            h2_0.setValid();
            h2_0.data = 32w0;
        }
        hdr.h2.data = h2_0.data;
        if (h2_0.data == 32w1) {
            h1_0.setInvalid();
        }
        hdr.h1.data = h1_0.data;
        h1_0 = h2_0;
        if (h1_0.data == 32w1) {
            s1_0 = (H){h1 = h1_0,h2 = h2_0};
            s2_0 = s1_0;
            if (s2_0.h1.data == 32w0 && s2_0.h2.data == 32w0) {
                if (s1_0.h1.data > 32w0) {
                    s1_0.h1.setInvalid();
                } else {
                    s1_0.h1.setInvalid();
                }
                hdr.h1.data = s1_0.h1.data;
            } else {
                s1_0.h1.setValid();
            }
            hdr.h2.data = s1_0.h1.data;
        }
        invalid_H();
        hdr.h1.data = 32w1;
        hdr.h2.data = 32w1;
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

