#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header1 {
    bit<32> data;
}

header Header2 {
    bit<16> data;
}

header_union Union {
    Header1 h1;
    Header2 h2;
    Header1 h3;
}

struct H {
    Header1 h1;
    Union u1;
    Union u2;
}

struct M { }

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.u1.h1.setValid();
        pkt.extract(hdr.u1.h3);
        hdr.u1.h1.data = 1;
        hdr.u1.h3.data = 1;

        hdr.u1.h1 = hdr.u1.h3;
        hdr.u1.h1.data = 1;
        hdr.u1.h3.data = 1;

        transition select (hdr.u1.h1.data) {
            0: next;
            default: last;
        }
    }

    state next {
        pkt.extract(hdr.u1.h2);
        transition last;
    }

    state last {
        hdr.u1.h1.data = 1;
        hdr.u1.h2.data = 1;
        hdr.u1.h3.data = 1;
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    Union u1;
    Union u2;

    apply {
        u1.h1 = { 1 };
        u2.h1 = u1.h1;
        u2.h1.data = 1;

        u1.h2.setValid();
        u2 = u1;
        u2.h1.data = 1;
        u2.h2.data = 1;

        if (u2.h2.data == 0) {
            u1.h1.setValid();
        } else {
            u1.h2.setValid();
        }

        u1.h1.data = 1;
        u1.h2.data = 1;

        u1.h3.setInvalid();

        u1.h1.data = 1;
        u1.h2.data = 1;
        u1.h3.data = 1;
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

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

