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
    Header1  h1;
    Union[2] u;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.u[0].h1.setValid();
        pkt.extract<Header1>(hdr.u[0].h3);
        hdr.u[0].h3.data = 32w1;
        hdr.u[0].h1 = hdr.u[0].h3;
        hdr.u[0].h1.data = 32w1;
        transition select(hdr.u[0].h1.data) {
            32w0: next;
            default: last;
        }
    }
    state next {
        pkt.extract<Header2>(hdr.u[0].h2);
        transition last;
    }
    state last {
        hdr.u[0].h1.data = 32w1;
        hdr.u[0].h2.data = 16w1;
        hdr.u[0].h3.data = 32w1;
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.u") Union[2] u_0;
    @name("IngressI.i") bit<1> i_0;
    apply {
        u_0[0].h1.setInvalid();
        u_0[0].h2.setInvalid();
        u_0[0].h3.setInvalid();
        u_0[1].h1.setInvalid();
        u_0[1].h2.setInvalid();
        u_0[1].h3.setInvalid();
        u_0[0].h1.setValid();
        u_0[0].h1 = (Header1){data = 32w1};
        u_0[1].h1 = u_0[0].h1;
        u_0[1].h1.data = 32w1;
        i_0 = 1w0;
        u_0[0].h2.setValid();
        u_0[1] = u_0[i_0];
        u_0[1].h2.data = 16w1;
        if (u_0[1].h2.data == 16w0) {
            u_0[i_0].h2.setValid();
        }
        if (u_0[1].h2.data == 16w0) {
            u_0[i_0].h1.setValid();
        } else {
            u_0[i_0].h2.setValid();
        }
        u_0[1].h1.setInvalid();
        u_0[i_0].h1.setInvalid();
        u_0[i_0].h1.setValid();
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
