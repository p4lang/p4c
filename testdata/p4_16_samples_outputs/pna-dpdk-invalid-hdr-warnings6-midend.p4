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

control ingress(inout H hdr, inout M meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    Header1 tmp_0_h1;
    Header2 tmp_0_h2;
    Header1 tmp_0_h3;
    @name("ingress.u") Union[2] u_0;
    @hidden action pnadpdkinvalidhdrwarnings6l74() {
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
    @hidden action pnadpdkinvalidhdrwarnings6l74_0() {
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
    @hidden action pnadpdkinvalidhdrwarnings6l74_1() {
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
    @hidden action pnadpdkinvalidhdrwarnings6l74_2() {
        tmp_0_h1.setInvalid();
        tmp_0_h2.setInvalid();
        tmp_0_h3.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l65() {
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
    @hidden action pnadpdkinvalidhdrwarnings6l74_3() {
        u_0[1].h1 = tmp_0_h1;
    }
    @hidden action pnadpdkinvalidhdrwarnings6l74_4() {
        u_0[1].h2 = tmp_0_h2;
    }
    @hidden action pnadpdkinvalidhdrwarnings6l74_5() {
        u_0[1].h3 = tmp_0_h3;
    }
    @hidden action pnadpdkinvalidhdrwarnings6l74_6() {
        u_0[1].h1.setInvalid();
    }
    @hidden action pnadpdkinvalidhdrwarnings6l76() {
        u_0[1].h2.data = 16w1;
        u_0[1w0].h2.setValid();
        u_0[1].h1.setInvalid();
        u_0[1w0].h1.setInvalid();
        u_0[1w0].h1.setValid();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l65 {
        actions = {
            pnadpdkinvalidhdrwarnings6l65();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l65();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_0 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_0();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_1 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_1();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_1();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_2 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_2();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_2();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_3 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_3();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_3();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_4 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_4();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_4();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_5 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_5();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_5();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l74_6 {
        actions = {
            pnadpdkinvalidhdrwarnings6l74_6();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l74_6();
    }
    @hidden table tbl_pnadpdkinvalidhdrwarnings6l76 {
        actions = {
            pnadpdkinvalidhdrwarnings6l76();
        }
        const default_action = pnadpdkinvalidhdrwarnings6l76();
    }
    apply {
        tbl_pnadpdkinvalidhdrwarnings6l65.apply();
        if (u_0[1w0].h1.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74.apply();
        } else if (u_0[1w0].h2.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74_0.apply();
        } else if (u_0[1w0].h3.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74_1.apply();
        } else {
            tbl_pnadpdkinvalidhdrwarnings6l74_2.apply();
        }
        if (tmp_0_h1.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74_3.apply();
        } else if (tmp_0_h2.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74_4.apply();
        } else if (tmp_0_h3.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings6l74_5.apply();
        } else {
            tbl_pnadpdkinvalidhdrwarnings6l74_6.apply();
        }
        tbl_pnadpdkinvalidhdrwarnings6l76.apply();
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
