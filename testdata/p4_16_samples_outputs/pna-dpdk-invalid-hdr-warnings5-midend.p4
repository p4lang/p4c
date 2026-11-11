#include <core.p4>
#include <pna.p4>

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

parser ParserImpl(packet_in pkt, out H hdr, inout M meta, in pna_main_parser_input_metadata_t istd) {
    @name("tmp") Header1 tmp_0;
    state start {
        hdr.u1_h1.setValid();
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        pkt.extract<Header1>(hdr.u1_h3);
        hdr.u1_h1.setInvalid();
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.data = 32w1;
        tmp_0 = hdr.u1_h3;
        transition select(tmp_0.isValid()) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        hdr.u1_h1.setValid();
        hdr.u1_h1 = tmp_0;
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        transition start_join;
    }
    state start_false {
        hdr.u1_h1.setInvalid();
        hdr.u1_h2.setInvalid();
        hdr.u1_h3.setInvalid();
        transition start_join;
    }
    state start_join {
        hdr.u1_h1.data = 32w1;
        transition last;
    }
    state last {
        hdr.u1_h1.data = 32w1;
        hdr.u1_h2.data = 16w1;
        hdr.u1_h3.data = 32w1;
        transition accept;
    }
}

control ingress(inout H hdr, inout M meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("u1_0_h1") Header1 u1_0_h1_0;
    @name("u1_0_h2") Header2 u1_0_h2_0;
    @name("u1_0_h3") Header1 u1_0_h3_0;
    @name("u2_0_h1") Header1 u2_0_h1_0;
    @name("u2_0_h2") Header2 u2_0_h2_0;
    @name("u2_0_h3") Header1 u2_0_h3_0;
    @hidden @name("pnadpdkinvalidhdrwarnings5l75") action pnadpdkinvalidhdrwarnings5l75_3() {
        if (u1_0_h1_0.isValid()) {
            u2_0_h1_0.setValid();
            u2_0_h1_0 = u1_0_h1_0;
            u2_0_h2_0.setInvalid();
            u2_0_h3_0.setInvalid();
        } else {
            u2_0_h1_0.setInvalid();
            u2_0_h2_0.setInvalid();
            u2_0_h3_0.setInvalid();
        }
    }
    @hidden @name("pnadpdkinvalidhdrwarnings5l75_0") action pnadpdkinvalidhdrwarnings5l75_4() {
        if (u1_0_h2_0.isValid()) {
            u2_0_h2_0.setValid();
            u2_0_h2_0 = u1_0_h2_0;
            u2_0_h1_0.setInvalid();
            u2_0_h3_0.setInvalid();
        } else {
            u2_0_h2_0.setInvalid();
            u2_0_h1_0.setInvalid();
            u2_0_h3_0.setInvalid();
        }
    }
    @hidden @name("pnadpdkinvalidhdrwarnings5l75_1") action pnadpdkinvalidhdrwarnings5l75_5() {
        if (u1_0_h3_0.isValid()) {
            u2_0_h3_0.setValid();
            u2_0_h3_0 = u1_0_h3_0;
            u2_0_h1_0.setInvalid();
            u2_0_h2_0.setInvalid();
        } else {
            u2_0_h3_0.setInvalid();
            u2_0_h1_0.setInvalid();
            u2_0_h2_0.setInvalid();
        }
    }
    @hidden @name("pnadpdkinvalidhdrwarnings5l75_2") action pnadpdkinvalidhdrwarnings5l75_6() {
        u2_0_h1_0.setInvalid();
        u2_0_h2_0.setInvalid();
        u2_0_h3_0.setInvalid();
    }
    @hidden @name("pnadpdkinvalidhdrwarnings5l66") action pnadpdkinvalidhdrwarnings5l66_0() {
        u1_0_h1_0.setInvalid();
        u1_0_h2_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h2_0.setInvalid();
        u1_0_h1_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h1_0.setInvalid();
        u1_0_h2_0.setInvalid();
        u2_0_h1_0.setInvalid();
        u2_0_h2_0.setInvalid();
        u2_0_h3_0.setInvalid();
        u2_0_h2_0.setInvalid();
        u2_0_h1_0.setInvalid();
        u2_0_h3_0.setInvalid();
        u2_0_h3_0.setInvalid();
        u2_0_h1_0.setInvalid();
        u2_0_h2_0.setInvalid();
        u1_0_h1_0.setValid();
        u1_0_h2_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h1_0.data = 32w1;
        u1_0_h2_0.setValid();
        u1_0_h1_0.setInvalid();
        u1_0_h3_0.setInvalid();
    }
    @hidden @name("pnadpdkinvalidhdrwarnings5l77") action pnadpdkinvalidhdrwarnings5l77_0() {
        u2_0_h2_0.data = 16w1;
        u1_0_h2_0.setValid();
        u1_0_h1_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h3_0.setInvalid();
        u1_0_h1_0.setInvalid();
        u1_0_h2_0.setInvalid();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l66") table tbl_pnadpdkinvalidhdrwarnings5l66_0 {
        actions = {
            pnadpdkinvalidhdrwarnings5l66_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l66_0();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l75") table tbl_pnadpdkinvalidhdrwarnings5l75_3 {
        actions = {
            pnadpdkinvalidhdrwarnings5l75_3();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l75_3();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l75_0") table tbl_pnadpdkinvalidhdrwarnings5l75_4 {
        actions = {
            pnadpdkinvalidhdrwarnings5l75_4();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l75_4();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l75_1") table tbl_pnadpdkinvalidhdrwarnings5l75_5 {
        actions = {
            pnadpdkinvalidhdrwarnings5l75_5();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l75_5();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l75_2") table tbl_pnadpdkinvalidhdrwarnings5l75_6 {
        actions = {
            pnadpdkinvalidhdrwarnings5l75_6();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l75_6();
    }
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l77") table tbl_pnadpdkinvalidhdrwarnings5l77_0 {
        actions = {
            pnadpdkinvalidhdrwarnings5l77_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l77_0();
    }
    apply {
        tbl_pnadpdkinvalidhdrwarnings5l66_0.apply();
        if (u1_0_h1_0.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings5l75_3.apply();
        } else if (u1_0_h2_0.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings5l75_4.apply();
        } else if (u1_0_h3_0.isValid()) {
            tbl_pnadpdkinvalidhdrwarnings5l75_5.apply();
        } else {
            tbl_pnadpdkinvalidhdrwarnings5l75_6.apply();
        }
        tbl_pnadpdkinvalidhdrwarnings5l77_0.apply();
    }
}

control DeparserImpl(packet_out pk, in H hdr, in M m, in pna_main_output_metadata_t ostd) {
    apply {
    }
}

control PreControlImpl(in H hdr, inout M meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<H, M, H, M>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
