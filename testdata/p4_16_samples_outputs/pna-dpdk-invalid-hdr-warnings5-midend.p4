#include <core.p4>
#include <dpdk/pna.p4>

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

control ingress(inout H hdr, inout M meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("u1_0_h1") Header1 u1_0_h1_0;
    @name("u1_0_h2") Header2 u1_0_h2_0;
    @name("u1_0_h3") Header1 u1_0_h3_0;
    @name("u2_0_h1") Header1 u2_0_h1_0;
    @name("u2_0_h2") Header2 u2_0_h2_0;
    @name("u2_0_h3") Header1 u2_0_h3_0;
    @hidden @name("pnadpdkinvalidhdrwarnings5l60") action pnadpdkinvalidhdrwarnings5l60_0() {
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
    @hidden @name("tbl_pnadpdkinvalidhdrwarnings5l60") table tbl_pnadpdkinvalidhdrwarnings5l60_0 {
        actions = {
            pnadpdkinvalidhdrwarnings5l60_0();
        }
        const default_action = pnadpdkinvalidhdrwarnings5l60_0();
    }
    apply {
        tbl_pnadpdkinvalidhdrwarnings5l60_0.apply();
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
