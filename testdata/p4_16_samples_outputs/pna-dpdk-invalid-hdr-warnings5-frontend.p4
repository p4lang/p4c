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
    Header1 h1;
    Union   u1;
    Union   u2;
}

struct M {
}

parser ParserImpl(packet_in pkt, out H hdr, inout M meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        hdr.u1.h1.setValid();
        pkt.extract<Header1>(hdr.u1.h3);
        hdr.u1.h3.data = 32w1;
        hdr.u1.h1 = hdr.u1.h3;
        hdr.u1.h1.data = 32w1;
        transition select(hdr.u1.h1.data) {
            32w0: next;
            default: last;
        }
    }
    state next {
        pkt.extract<Header2>(hdr.u1.h2);
        transition last;
    }
    state last {
        hdr.u1.h1.data = 32w1;
        hdr.u1.h2.data = 16w1;
        hdr.u1.h3.data = 32w1;
        transition accept;
    }
}

control ingress(inout H hdr, inout M meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("ingress.u1") Union u1_0;
    @name("ingress.u2") Union u2_0;
    apply {
        u1_0.h1.setInvalid();
        u1_0.h2.setInvalid();
        u1_0.h3.setInvalid();
        u2_0.h1.setInvalid();
        u2_0.h2.setInvalid();
        u2_0.h3.setInvalid();
        u1_0.h1.setValid();
        u1_0.h1 = (Header1){data = 32w1};
        u1_0.h2.setValid();
        u2_0 = u1_0;
        u2_0.h2.data = 16w1;
        if (u2_0.h2.data == 16w0) {
            u1_0.h1.setValid();
        } else {
            u1_0.h2.setValid();
        }
        u1_0.h3.setInvalid();
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
