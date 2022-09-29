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
    state next {
        pkt.extract<Header2>(hdr.u[0].h2);
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
    @name("tmp_h1") Header1 tmp_h1_0;
    @name("tmp_h2") Header2 tmp_h2_0;
    @name("tmp_h3") Header1 tmp_h3_0;
    @name("IngressI.u") Union[2] u_1;
    @hidden action invalidhdrwarnings6l61() {
        u_1[1].h1.setValid();
        u_1[1].h1 = u_1[0].h1;
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l61_0() {
        u_1[1].h1.setInvalid();
    }
    @hidden action invalidhdrwarnings6l57() {
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
    }
    @hidden action invalidhdrwarnings6l66() {
        tmp_h1_0.setValid();
        tmp_h1_0 = u_1[1w0].h1;
        tmp_h2_0.setInvalid();
        tmp_h3_0.setInvalid();
    }
    @hidden action act() {
        tmp_h1_0.setInvalid();
    }
    @hidden action invalidhdrwarnings6l62() {
        u_1[1].h1.setValid();
        u_1[1].h1.data = 32w1;
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
        u_1[0].h2.setValid();
        u_1[0].h1.setInvalid();
        u_1[0].h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_0() {
        tmp_h2_0.setValid();
        tmp_h2_0 = u_1[1w0].h2;
        tmp_h1_0.setInvalid();
        tmp_h3_0.setInvalid();
    }
    @hidden action act_0() {
        tmp_h2_0.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_1() {
        tmp_h3_0.setValid();
        tmp_h3_0 = u_1[1w0].h3;
        tmp_h1_0.setInvalid();
        tmp_h2_0.setInvalid();
    }
    @hidden action act_1() {
        tmp_h3_0.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_2() {
        u_1[1].h1.setValid();
        u_1[1].h1 = tmp_h1_0;
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_3() {
        u_1[1].h1.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_4() {
        u_1[1].h2.setValid();
        u_1[1].h2 = tmp_h2_0;
        u_1[1].h1.setInvalid();
        u_1[1].h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_5() {
        u_1[1].h2.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_6() {
        u_1[1].h3.setValid();
        u_1[1].h3 = tmp_h3_0;
        u_1[1].h1.setInvalid();
        u_1[1].h2.setInvalid();
    }
    @hidden action invalidhdrwarnings6l66_7() {
        u_1[1].h3.setInvalid();
    }
    @hidden action invalidhdrwarnings6l68() {
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
    @hidden table tbl_invalidhdrwarnings6l57 {
        actions = {
            invalidhdrwarnings6l57();
        }
        const default_action = invalidhdrwarnings6l57();
    }
    @hidden table tbl_invalidhdrwarnings6l61 {
        actions = {
            invalidhdrwarnings6l61();
        }
        const default_action = invalidhdrwarnings6l61();
    }
    @hidden table tbl_invalidhdrwarnings6l61_0 {
        actions = {
            invalidhdrwarnings6l61_0();
        }
        const default_action = invalidhdrwarnings6l61_0();
    }
    @hidden table tbl_invalidhdrwarnings6l62 {
        actions = {
            invalidhdrwarnings6l62();
        }
        const default_action = invalidhdrwarnings6l62();
    }
    @hidden table tbl_invalidhdrwarnings6l66 {
        actions = {
            invalidhdrwarnings6l66();
        }
        const default_action = invalidhdrwarnings6l66();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_invalidhdrwarnings6l66_0 {
        actions = {
            invalidhdrwarnings6l66_0();
        }
        const default_action = invalidhdrwarnings6l66_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_invalidhdrwarnings6l66_1 {
        actions = {
            invalidhdrwarnings6l66_1();
        }
        const default_action = invalidhdrwarnings6l66_1();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
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
    @hidden table tbl_invalidhdrwarnings6l66_7 {
        actions = {
            invalidhdrwarnings6l66_7();
        }
        const default_action = invalidhdrwarnings6l66_7();
    }
    @hidden table tbl_invalidhdrwarnings6l68 {
        actions = {
            invalidhdrwarnings6l68();
        }
        const default_action = invalidhdrwarnings6l68();
    }
    apply {
        tbl_invalidhdrwarnings6l57.apply();
        if (u_1[0].h1.isValid()) {
            tbl_invalidhdrwarnings6l61.apply();
        } else {
            tbl_invalidhdrwarnings6l61_0.apply();
        }
        tbl_invalidhdrwarnings6l62.apply();
        if (u_1[1w0].h1.isValid()) {
            tbl_invalidhdrwarnings6l66.apply();
        } else {
            tbl_act.apply();
        }
        if (u_1[1w0].h2.isValid()) {
            tbl_invalidhdrwarnings6l66_0.apply();
        } else {
            tbl_act_0.apply();
        }
        if (u_1[1w0].h3.isValid()) {
            tbl_invalidhdrwarnings6l66_1.apply();
        } else {
            tbl_act_1.apply();
        }
        if (tmp_h1_0.isValid()) {
            tbl_invalidhdrwarnings6l66_2.apply();
        } else {
            tbl_invalidhdrwarnings6l66_3.apply();
        }
        if (tmp_h2_0.isValid()) {
            tbl_invalidhdrwarnings6l66_4.apply();
        } else {
            tbl_invalidhdrwarnings6l66_5.apply();
        }
        if (tmp_h3_0.isValid()) {
            tbl_invalidhdrwarnings6l66_6.apply();
        } else {
            tbl_invalidhdrwarnings6l66_7.apply();
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

