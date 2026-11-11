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
    @name("ingress.u") Union[2] u_0;
    @hidden action pnadpdkheaderunionstack2l73() {
        u_0[1].h1 = u_0[0].h1;
    }
    @hidden action pnadpdkheaderunionstack2l73_0() {
        u_0[1].h2 = u_0[0].h2;
    }
    @hidden action pnadpdkheaderunionstack2l73_1() {
        u_0[1].h3 = u_0[0].h3;
    }
    @hidden action pnadpdkheaderunionstack2l73_2() {
        u_0[1].h1.setInvalid();
    }
    @hidden action pnadpdkheaderunionstack2l65() {
        u_0[0].h1.setInvalid();
        u_0[0].h2.setInvalid();
        u_0[0].h3.setInvalid();
        u_0[1].h1.setInvalid();
        u_0[1].h2.setInvalid();
        u_0[1].h3.setInvalid();
        u_0[0].h1.setValid();
        u_0[0].h1.data = 32w1;
        u_0[0].h2.setValid();
    }
    @hidden action pnadpdkheaderunionstack2l75() {
        u_0[1].h2.data = 16w1;
        u_0[0].h2.setValid();
        u_0[0].h3.setInvalid();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l65 {
        actions = {
            pnadpdkheaderunionstack2l65();
        }
        const default_action = pnadpdkheaderunionstack2l65();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l73 {
        actions = {
            pnadpdkheaderunionstack2l73();
        }
        const default_action = pnadpdkheaderunionstack2l73();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l73_0 {
        actions = {
            pnadpdkheaderunionstack2l73_0();
        }
        const default_action = pnadpdkheaderunionstack2l73_0();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l73_1 {
        actions = {
            pnadpdkheaderunionstack2l73_1();
        }
        const default_action = pnadpdkheaderunionstack2l73_1();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l73_2 {
        actions = {
            pnadpdkheaderunionstack2l73_2();
        }
        const default_action = pnadpdkheaderunionstack2l73_2();
    }
    @hidden table tbl_pnadpdkheaderunionstack2l75 {
        actions = {
            pnadpdkheaderunionstack2l75();
        }
        const default_action = pnadpdkheaderunionstack2l75();
    }
    apply {
        tbl_pnadpdkheaderunionstack2l65.apply();
        if (u_0[0].h1.isValid()) {
            tbl_pnadpdkheaderunionstack2l73.apply();
        } else if (u_0[0].h2.isValid()) {
            tbl_pnadpdkheaderunionstack2l73_0.apply();
        } else if (u_0[0].h3.isValid()) {
            tbl_pnadpdkheaderunionstack2l73_1.apply();
        } else {
            tbl_pnadpdkheaderunionstack2l73_2.apply();
        }
        tbl_pnadpdkheaderunionstack2l75.apply();
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
