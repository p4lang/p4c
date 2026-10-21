#include <core.p4>
#include <pna.p4>

header H {
    bit<8> x;
}

header_union U {
    H a;
    H b;
}

struct headers_t {
    H    u_a;
    H    u_b;
    U[1] s;
}

struct metadata_t {
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    H l_0_a;
    H l_0_b;
    state start {
        l_0_a.setInvalid();
        l_0_b.setInvalid();
        pkt.extract<H>(l_0_b);
        l_0_a.setInvalid();
        pkt.extract<H>(l_0_a);
        l_0_b.setInvalid();
        pkt.extract<H>(hdr.u_b);
        hdr.u_a.setInvalid();
        pkt.extract<H>(hdr.u_a);
        hdr.u_b.setInvalid();
        pkt.extract<H>(hdr.s[0].b);
        pkt.extract<H>(hdr.s[0].a);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkheaderunionextract53() {
        send_to_port(32w0);
    }
    @hidden table tbl_pnadpdkheaderunionextract53 {
        actions = {
            pnadpdkheaderunionextract53();
        }
        const default_action = pnadpdkheaderunionextract53();
    }
    apply {
        tbl_pnadpdkheaderunionextract53.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkheaderunionextract60() {
        pkt.emit<H>(hdr.u_a);
        pkt.emit<H>(hdr.u_b);
        pkt.emit<H>(hdr.s[0].a);
        pkt.emit<H>(hdr.s[0].b);
    }
    @hidden table tbl_pnadpdkheaderunionextract60 {
        actions = {
            pnadpdkheaderunionextract60();
        }
        const default_action = pnadpdkheaderunionextract60();
    }
    apply {
        tbl_pnadpdkheaderunionextract60.apply();
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
