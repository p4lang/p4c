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
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        pkt.extract<Header1>(hdr.u[0].h3);
        hdr.u[0].h3.setValid();
        hdr.u[0].h3.data = 32w1;
        hdr.u[0].h1.setInvalid();
        hdr.u[0].h2.setInvalid();
        transition select(hdr.u[0].h3.isValid()) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        hdr.u[0].h1.setValid();
        hdr.u[0].h1 = hdr.u[0].h3;
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        transition start_join;
    }
    state start_false {
        hdr.u[0].h1.setInvalid();
        transition start_join;
    }
    state start_join {
        hdr.u[0].h1.setValid();
        hdr.u[0].h1.data = 32w1;
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        transition last;
    }
    state last {
        hdr.u[0].h1.setValid();
        hdr.u[0].h1.data = 32w1;
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        hdr.u[0].h2.setValid();
        hdr.u[0].h2.data = 16w1;
        hdr.u[0].h1.setInvalid();
        hdr.u[0].h3.setInvalid();
        hdr.u[0].h3.setValid();
        hdr.u[0].h3.data = 32w1;
        hdr.u[0].h1.setInvalid();
        hdr.u[0].h2.setInvalid();
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.u") Union[2] u_1;
    @hidden @name("invalidhdrwarnings6l57") action invalidhdrwarnings6l57_0() {
        u_1[0].h1.setInvalid();
        u_1[0].h2.setInvalid();
        u_1[0].h3.setInvalid();
        u_1[1].h1.setInvalid();
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
        u_1[0].h1.setValid();
        u_1[0].h2.setInvalid();
        u_1[0].h3.setInvalid();
        u_1[0].h1.setValid();
        u_1[0].h1.data = 32w1;
        u_1[0].h2.setInvalid();
        u_1[0].h3.setInvalid();
        if (u_1[0].h1.isValid()) {
            u_1[1].h1.setValid();
            u_1[1].h1 = u_1[0].h1;
            u_1[1].h2.setInvalid();
            u_1[1].h3.setInvalid();
        } else {
            u_1[1].h1.setInvalid();
        }
        u_1[1].h1.setValid();
        u_1[1].h1.data = 32w1;
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
        u_1[0].h2.setValid();
        u_1[0].h1.setInvalid();
        u_1[0].h3.setInvalid();
        if (u_1[1w0].h1.isValid()) {
            u_1[1].h1.setValid();
            u_1[1].h1 = u_1[1w0].h1;
            u_1[1].h2.setInvalid();
            u_1[1].h3.setInvalid();
        } else {
            u_1[1].h1.setInvalid();
        }
        if (u_1[1w0].h2.isValid()) {
            u_1[1].h2.setValid();
            u_1[1].h2 = u_1[1w0].h2;
            u_1[1].h1.setInvalid();
            u_1[1].h3.setInvalid();
        } else {
            u_1[1].h2.setInvalid();
        }
        if (u_1[1w0].h3.isValid()) {
            u_1[1].h3.setValid();
            u_1[1].h3 = u_1[1w0].h3;
            u_1[1].h1.setInvalid();
            u_1[1].h2.setInvalid();
        } else {
            u_1[1].h3.setInvalid();
        }
        u_1[1].h2.setValid();
        u_1[1].h2.data = 16w1;
        u_1[1].h1.setInvalid();
        u_1[1].h3.setInvalid();
        u_1[1w0].h2.setValid();
        u_1[1w0].h1.setInvalid();
        u_1[1w0].h3.setInvalid();
        u_1[1].h1.setInvalid();
        u_1[1w0].h1.setInvalid();
        u_1[1w0].h1.setValid();
        u_1[1w0].h2.setInvalid();
        u_1[1w0].h3.setInvalid();
    }
    @hidden @name("tbl_invalidhdrwarnings6l57") table tbl_invalidhdrwarnings6l57_0 {
        actions = {
            invalidhdrwarnings6l57_0();
        }
        const default_action = invalidhdrwarnings6l57_0();
    }
    apply {
        tbl_invalidhdrwarnings6l57_0.apply();
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
