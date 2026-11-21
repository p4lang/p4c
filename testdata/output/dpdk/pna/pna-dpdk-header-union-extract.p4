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
        pkt.extract(l.b);
        pkt.extract(l.a);
        pkt.extract(hdr.u.b);
        pkt.extract(hdr.u.a);
        pkt.extract(hdr.s[0].b);
        pkt.extract(hdr.s[0].a);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        send_to_port((PortId_t)0);
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.u);
        pkt.emit(hdr.s);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
