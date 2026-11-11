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
    Header1 tmp;
    state start {
        hdr.u[0].h1.setValid();
        pkt.extract<Header1>(hdr.u[0].h3);
        hdr.u[0].h3.data = 32w1;
        tmp = hdr.u[0].h3;
        hdr.u[0].h1 = tmp;
        hdr.u[0].h1.data = 32w1;
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
    Header1 tmp_0_h1;
    Header2 tmp_0_h2;
    Header1 tmp_0_h3;
    @name("IngressI.u") Union[2] u_0;
    @hidden action invalidhdrwarnings6l66() {
        if (u_0[1w0].h1.isValid()) {
            tmp_0_h1.setValid();
            tmp_0_h1 = u_0[1w0].h1;
            tmp_0_h2.setInvalid();
            tmp_0_h3.setInvalid();
        } else {
            tmp_0_h1.setInvalid();
            tmp_0_h2.setInvalid();
            tmp_0_h3.setInvalid();
        }
    }
    @hidden action invalidhdrwarnings6l66_0() {
        if (u_0[1w0].h2.isValid()) {
            tmp_0_h2.setValid();
            tmp_0_h2 = u_0[1w0].h2;
            tmp_0_h1.setInvalid();
            tmp_0_h3.setInvalid();
        } else {
            tmp_0_h2.setInvalid();
            tmp_0_h1.setInvalid();
            tmp_0_h3.setInvalid();
        }
    }
    @hidden action invalidhdrwarnings6l66_1() {
        if (u_0[1w0].h3.isValid()) {
            tmp_0_h3.setValid();
            tmp_0_h3 = u_0[1w0].h3;
            tmp_0_h1.setInvalid();
            tmp_0_h2.setInvalid();
        } else {
            tmp_0_h3.setInvalid();
            tmp_0_h1.setInvalid();
            tmp_0_h2.setInvalid();
        }
    }
    @hidden action invalidhdrwarnings6l66_2() {
        tmp_0_h1.setInvalid();
        tmp_0_h2.setInvalid();
        tmp_0_h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l57() {
        u_0[0].h1.setInvalid();
        u_0[0].h2.setInvalid();
        u_0[0].h3.setInvalid();
        u_0[1].h1.setInvalid();
        u_0[1].h2.setInvalid();
        u_0[1].h3.setInvalid();
        u_0[0].h1.setValid();
        u_0[0].h1.data = 32w1;
        u_0[1].h1 = u_0[0].h1;
        u_0[1].h1.data = 32w1;
        u_0[0].h2.setValid();
    }
    @hidden action invalidhdrwarnings6l66_3() {
        u_0[1].h1 = tmp_0_h1;
    }
    @hidden action invalidhdrwarnings6l66_4() {
        u_0[1].h2 = tmp_0_h2;
    }
    @hidden action invalidhdrwarnings6l66_5() {
        u_0[1].h3 = tmp_0_h3;
    }
    @hidden action invalidhdrwarnings6l66_6() {
        u_0[1].h1.setInvalid();
    }
    @hidden action invalidhdrwarnings6l68() {
        u_0[1].h2.data = 16w1;
        u_0[1w0].h2.setValid();
        u_0[1].h1.setInvalid();
        u_0[1w0].h1.setInvalid();
        u_0[1w0].h1.setValid();
    }
    @hidden table tbl_invalidhdrwarnings6l57 {
        actions = {
            invalidhdrwarnings6l57();
        }
        const default_action = invalidhdrwarnings6l57();
    }
    @hidden table tbl_invalidhdrwarnings6l66 {
        actions = {
            invalidhdrwarnings6l66();
        }
        const default_action = invalidhdrwarnings6l66();
    }
    @hidden table tbl_invalidhdrwarnings6l66_0 {
        actions = {
            invalidhdrwarnings6l66_0();
        }
        const default_action = invalidhdrwarnings6l66_0();
    }
    @hidden table tbl_invalidhdrwarnings6l66_1 {
        actions = {
            invalidhdrwarnings6l66_1();
        }
        const default_action = invalidhdrwarnings6l66_1();
    }
    @hidden table tbl_invalidhdrwarnings6l66_2 {
        actions = {
            invalidhdrwarnings6l66_2();
        }
        const default_action = invalidhdrwarnings6l66_2();
    }
    @hidden table tbl_invalidhdrwarnings6l66_3 {
        actions = {
            invalidhdrwarnings6l66_3();
        }
        const default_action = invalidhdrwarnings6l66_3();
    }
    @hidden table tbl_invalidhdrwarnings6l66_4 {
        actions = {
            invalidhdrwarnings6l66_4();
        }
        const default_action = invalidhdrwarnings6l66_4();
    }
    @hidden table tbl_invalidhdrwarnings6l66_5 {
        actions = {
            invalidhdrwarnings6l66_5();
        }
        const default_action = invalidhdrwarnings6l66_5();
    }
    @hidden table tbl_invalidhdrwarnings6l66_6 {
        actions = {
            invalidhdrwarnings6l66_6();
        }
        const default_action = invalidhdrwarnings6l66_6();
    }
    @hidden table tbl_invalidhdrwarnings6l68 {
        actions = {
            invalidhdrwarnings6l68();
        }
        const default_action = invalidhdrwarnings6l68();
    }
    apply {
        tbl_invalidhdrwarnings6l57.apply();
        if (u_0[1w0].h1.isValid()) {
            tbl_invalidhdrwarnings6l66.apply();
        } else if (u_0[1w0].h2.isValid()) {
            tbl_invalidhdrwarnings6l66_0.apply();
        } else if (u_0[1w0].h3.isValid()) {
            tbl_invalidhdrwarnings6l66_1.apply();
        } else {
            tbl_invalidhdrwarnings6l66_2.apply();
        }
        if (tmp_0_h1.isValid()) {
            tbl_invalidhdrwarnings6l66_3.apply();
        } else if (tmp_0_h2.isValid()) {
            tbl_invalidhdrwarnings6l66_4.apply();
        } else if (tmp_0_h3.isValid()) {
            tbl_invalidhdrwarnings6l66_5.apply();
        } else {
            tbl_invalidhdrwarnings6l66_6.apply();
        }
        tbl_invalidhdrwarnings6l68.apply();
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
