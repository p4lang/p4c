#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header1 {
    bit<32> data;
}

header Header2 {
    bit<16> data;
}

struct H {
    Header1 h1;
    Header1 u1_h1;
    Header2 u1_h2;
    Header1 u1_h3;
    Header1 u2_h1;
    Header2 u2_h2;
    Header1 u2_h3;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        hdr.u1_h1.setValid();
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        pkt.extract<Header1>(hdr.u1_h3);
        hdr.u1_h3.setValid();
        hdr.u1_h3.data = 32w1;
        hdr.u1_h1.setInvalid();
        hdr.u1_h2.setInvalid();
        transition select(hdr.u1_h3.isValid()) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        hdr.u1_h1.setValid();
        hdr.u1_h1 = hdr.u1_h3;
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        transition start_join;
    }
    state start_false {
        hdr.u1_h1.setInvalid();
        transition start_join;
    }
    state start_join {
        hdr.u1_h1.setValid();
        hdr.u1_h1.data = 32w1;
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        transition last;
    }
    state last {
        hdr.u1_h1.setValid();
        hdr.u1_h1.data = 32w1;
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        hdr.u1_h2.setValid();
        hdr.u1_h2.data = 16w1;
        hdr.u1_h1.setInvalid();
        hdr.u1_h3.setInvalid();
        hdr.u1_h3.setValid();
        hdr.u1_h3.data = 32w1;
        hdr.u1_h1.setInvalid();
        hdr.u1_h2.setInvalid();
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("u1_0_h1") Header1 u1_0_h1_0;
    @name("u1_0_h2") Header2 u1_0_h2_0;
    @name("u1_0_h3") Header1 u1_0_h3_0;
    @name("u2_0_h1") Header1 u2_0_h1_0;
    @name("u2_0_h2") Header2 u2_0_h2_0;
    @name("u2_0_h3") Header1 u2_0_h3_0;
    @hidden @name("invalidhdrwarnings5l58") action invalidhdrwarnings5l58_0() {
        u1_0_h1_0.setInvalid();
        u1_0_h2_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u2_0_h1_0.setInvalid();
        u2_0_h2_0.setInvalid();
        u2_0_h3_0.setInvalid();
        u1_0_h1_0.setValid();
        u1_0_h2_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h1_0.setValid();
        u1_0_h1_0.data = 32w1;
        u1_0_h2_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h2_0.setValid();
        u1_0_h1_0.setInvalid();
        u1_0_h3_0.setInvalid();
        if (u1_0_h1_0.isValid()) {
            u2_0_h1_0.setValid();
            u2_0_h1_0 = u1_0_h1_0;
            u2_0_h2_0.setInvalid();
            u2_0_h3_0.setInvalid();
        } else {
            u2_0_h1_0.setInvalid();
        }
        if (u1_0_h2_0.isValid()) {
            u2_0_h2_0.setValid();
            u2_0_h2_0 = u1_0_h2_0;
            u2_0_h1_0.setInvalid();
            u2_0_h3_0.setInvalid();
        } else {
            u2_0_h2_0.setInvalid();
        }
        if (u1_0_h3_0.isValid()) {
            u2_0_h3_0.setValid();
            u2_0_h3_0 = u1_0_h3_0;
            u2_0_h1_0.setInvalid();
            u2_0_h2_0.setInvalid();
        } else {
            u2_0_h3_0.setInvalid();
        }
        u2_0_h2_0.setValid();
        u2_0_h2_0.data = 16w1;
        u2_0_h1_0.setInvalid();
        u2_0_h3_0.setInvalid();
        u1_0_h2_0.setValid();
        u1_0_h1_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h3_0.setInvalid();
    }
    @hidden @name("tbl_invalidhdrwarnings5l58") table tbl_invalidhdrwarnings5l58_0 {
        actions = {
            invalidhdrwarnings5l58_0();
        }
        const default_action = invalidhdrwarnings5l58_0();
    }
    apply {
        tbl_invalidhdrwarnings5l58_0.apply();
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
