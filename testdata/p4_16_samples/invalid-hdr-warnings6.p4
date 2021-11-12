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
    Union[2] u;
}

struct M { }

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.u[0].h1.setValid();
        pkt.extract(hdr.u[0].h3);
        hdr.u[0].h1.data = 1;
        hdr.u[0].h3.data = 1;

        hdr.u[0].h1 = hdr.u[0].h3;
        hdr.u[0].h1.data = 1;
        hdr.u[0].h3.data = 1;

        transition select (hdr.u[0].h1.data) {
            0: next;
            default: last;
        }
    }

    state next {
        pkt.extract(hdr.u[0].h2);
        transition last;
    }

    state last {
        hdr.u[0].h1.data = 1;
        hdr.u[0].h2.data = 1;
        hdr.u[0].h3.data = 1;
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    Union[2] u;

    apply {
        u[0].h1 = { 1 };
        u[1].h1 = u[0].h1;
        u[1].h1.data = 1;

        bit i = 0;
        u[0].h2.setValid();     // u[0].h1 invalid from this point
        u[1] = u[i];
        u[1].h1.data = 1;
        u[1].h2.data = 1;

        if (u[1].h2.data == 0) {
            u[i].h2.setValid();
        }

        u[0].h1.data = 1;       // u[0].h1 is invalid either if the then branch is executed (for any i) or not
        
        if (u[1].h2.data == 0) {
            u[i].h1.setValid();
        } else {
            u[i].h2.setValid();
        }

        u[0].h1.data = 1;
        u[1].h1.setInvalid();

        u[i].h1.setInvalid();   // no effect
        u[0].h1.data = 1;
        u[1].h1.data = 1;

        u[i].h1.setValid();
        u[0].h1.data = 1;
        u[1].h1.data = 1;
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

