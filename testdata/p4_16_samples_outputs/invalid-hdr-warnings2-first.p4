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
    Header h1;
    Header h2;
    H s1;
    H s2;
    action invalid_H(inout H s) {
        s.h1.setInvalid();
        s.h2.setInvalid();
    }
    apply {
        h1 = (Header){data = 32w1};
        if (hdr.h1.data == 32w0) {
            h2 = h1;
        } else {
            h2.setValid();
            h2.data = 32w0;
        }
        hdr.h1.data = h1.data;
        hdr.h2.data = h2.data;
        if (h2.data == 32w1) {
            h1.setInvalid();
        }
        hdr.h1.data = h1.data;
        h1 = h2;
        if (h1.data == 32w1) {
            s1 = (H){h1 = h1,h2 = h2};
            s2 = s1;
            if (s2.h1.data == 32w0 && s2.h2.data == 32w0) {
                if (s1.h1.data > 32w0) {
                    s1.h1.setInvalid();
                } else {
                    s1.h1.setInvalid();
                }
                hdr.h1.data = s1.h1.data;
            } else {
                s1.h1.setValid();
            }
            hdr.h2.data = s1.h1.data;
        }
        invalid_H(hdr);
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

