#include <core.p4>
#include <pna.p4>

struct my_struct_t {
    bit<8> type1;
    bit<8> type2;
}

header my_header_t {
    bit<8> type1;
    bit<8> type2;
    bit<8> value;
}

struct main_metadata_t {
}

struct headers_t {
    my_header_t h;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    bit<16> tmp;
    state start {
        tmp = pkt.lookahead<bit<16>>();
        transition select(tmp[7:0]) {
            8w1: parse_header;
            default: accept;
        }
    }
    state parse_header {
        pkt.extract<my_header_t>(hdr.h);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControlImpl(inout headers_t hdr, inout main_metadata_t user_meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;

