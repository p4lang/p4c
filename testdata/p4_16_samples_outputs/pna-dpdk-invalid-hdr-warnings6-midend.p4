#include <core.p4>
#include <pna.p4>

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

parser ParserImpl(packet_in pkt, out H hdr, inout M meta, in pna_main_parser_input_metadata_t istd) {
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

control ingress(inout H hdr, inout M meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("tmp_h1") Header1 tmp_h1_0;
    @name("tmp_h2") Header2 tmp_h2_0;
    @name("tmp_h3") Header1 tmp_h3_0;
    @name("ingress.u") Union[2] u_1;
    @hidden action pnadpdkinvalidhdrwarnings6l63() {
        u_1[1].h1.setValid();
        u_1[1].h1 = u_1[0].h1;
        u_1[1].h2.setInvalid();
        u_1[1].h3.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l63_0() {
        u_1[1].h1.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l59() {
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
    @hidden action pnadpdkinvalidhdrwarnings6l68() {
        tmp_h2_0.setValid();
        tmp_h2_0 = u_1[1w0].h2;
        tmp_h1_0.setInvalid();
        tmp_h3_0.setInvalid();
    }
    @hidden action act() {
        tmp_h2_0.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l67() {
        u_1[0].h2.setValid();
        u_1[0].h1.setInvalid();
        u_1[0].h3.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l68_0() {
        tmp_h3_0.setValid();
        tmp_h3_0 = u_1[1w0].h3;
        tmp_h1_0.setInvalid();
        tmp_h2_0.setInvalid();
    }
    @hidden action act_0() {
        tmp_h3_0.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l68_1() {
        u_1[1].h2.setValid();
        u_1[1].h2 = tmp_h2_0;
        u_1[1].h1.setInvalid();
        u_1[1].h3.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l68_2() {
        u_1[1].h2.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l70() {
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
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l59 {
        actions = {
            pnadpdkinvalidhdrwarnings6l59();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l59();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l63 {
        actions = {
            pnadpdkinvalidhdrwarnings6l63();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l63();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l63_0 {
        actions = {
            pnadpdkinvalidhdrwarnings6l63_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l63_0();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l67 {
        actions = {
            pnadpdkinvalidhdrwarnings6l67();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l67();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l68 {
        actions = {
            pnadpdkinvalidhdrwarnings6l68();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l68();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l68_0 {
        actions = {
            pnadpdkinvalidhdrwarnings6l68_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l68_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l68_1 {
        actions = {
            pnadpdkinvalidhdrwarnings6l68_1();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l68_1();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l68_2 {
        actions = {
            pnadpdkinvalidhdrwarnings6l68_2();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l68_2();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l70 {
        actions = {
            pnadpdkinvalidhdrwarnings6l70();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l70();
    }
    apply {
        tbl_pnadpdkinvalidhdrwarnings6l59.apply();
        if (u_1[0].h1.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l63.apply();
        } else {
            tbl_pnadpdkinvalidhdrwarnings6l63_0.apply();
        }
        tbl_pnadpdkinvalidhdrwarnings6l67.apply();
        if (u_1[1w0].h2.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l68.apply();
        } else {
            tbl_act.apply();
        }
        if (u_1[1w0].h3.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l68_0.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp_h2_0.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l68_1.apply();
        } else {
            tbl_pnadpdkinvalidhdrwarnings6l68_2.apply();
        }
        tbl_pnadpdkinvalidhdrwarnings6l70.apply();
    }
}

control DeparserImpl(packet_out pk, in H hdr, in M meta, in pna_main_output_metadata_t ostd) {
    apply {
    }
}

control PreControlImpl(in H hdr, inout M meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<H, M, H, M>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;

