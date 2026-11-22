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
    U    u;
    U[1] s;
}

struct metadata_t {
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    U l;
    state start {
        pkt.extract<H>(l.b);
        pkt.extract<H>(l.a);
        pkt.extract<H>(hdr.u.b);
        pkt.extract<H>(hdr.u.a);
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
    apply {
        send_to_port((PortId_t)32w0);
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<U>(hdr.u);
        pkt.emit<U[1]>(hdr.s);
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
