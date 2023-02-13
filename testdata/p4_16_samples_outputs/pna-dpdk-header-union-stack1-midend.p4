#include <core.p4>
#include <dpdk/pna.p4>

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
    Header1    h1;
    Header2[2] h2;
    Union[2]   u;
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
        hdr.h2[0].data = 16w1;
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
    @hidden @name("pnadpdkheaderunionstack1l62") action pnadpdkheaderunionstack1l62_0() {
        hdr.u[0].h1.setValid();
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        hdr.u[0].h1.setValid();
        hdr.u[0].h1.data = 32w1;
        hdr.u[0].h2.setInvalid();
        hdr.u[0].h3.setInvalid();
        hdr.u[0].h2.setValid();
        hdr.u[0].h1.setInvalid();
        hdr.u[0].h3.setInvalid();
        if (hdr.u[0].h1.isValid()) {
            hdr.u[1].h1.setValid();
            hdr.u[1].h1 = hdr.u[0].h1;
            hdr.u[1].h2.setInvalid();
            hdr.u[1].h3.setInvalid();
        } else {
            hdr.u[1].h1.setInvalid();
        }
        if (hdr.u[0].h2.isValid()) {
            hdr.u[1].h2.setValid();
            hdr.u[1].h2 = hdr.u[0].h2;
            hdr.u[1].h1.setInvalid();
            hdr.u[1].h3.setInvalid();
        } else {
            hdr.u[1].h2.setInvalid();
        }
        if (hdr.u[0].h3.isValid()) {
            hdr.u[1].h3.setValid();
            hdr.u[1].h3 = hdr.u[0].h3;
            hdr.u[1].h1.setInvalid();
            hdr.u[1].h2.setInvalid();
        } else {
            hdr.u[1].h3.setInvalid();
        }
        hdr.u[1].h1.setValid();
        hdr.u[1].h1.data = 32w1;
        hdr.u[1].h2.setInvalid();
        hdr.u[1].h3.setInvalid();
        hdr.u[1].h2.setValid();
        hdr.u[1].h2.data = 16w1;
        hdr.u[1].h1.setInvalid();
        hdr.u[1].h3.setInvalid();
        hdr.u[0].h2.setValid();
        hdr.u[0].h1.setInvalid();
        hdr.u[0].h3.setInvalid();
        hdr.u[0].h3.setInvalid();
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
    }
    @hidden @name("tbl_pnadpdkheaderunionstack1l62") table tbl_pnadpdkheaderunionstack1l62_0 {
        actions = {
            pnadpdkheaderunionstack1l62_0();
        }
        const default_action = pnadpdkheaderunionstack1l62_0();
    }
    apply {
        tbl_pnadpdkheaderunionstack1l62_0.apply();
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
