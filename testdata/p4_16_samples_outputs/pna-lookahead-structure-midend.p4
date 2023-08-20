#include <core.p4>
#include <pna.p4>

struct my_struct_t {
    bit<16> type1;
    bit<8>  type2;
}

header my_header_t {
    bit<16> type1;
    bit<8>  type2;
    bit<32> value;
}

struct main_metadata_t {
    bit<16> _s1_type10;
    bit<8>  _s1_type21;
}

struct headers_t {
    my_header_t h1;
    my_header_t h2;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout main_metadata_t main_meta, in pna_main_parser_input_metadata_t istd) {
    bit<24> tmp;
    bit<24> tmp_1;
    state start {
        tmp = pkt.lookahead<bit<24>>();
        transition select(tmp[23:8]) {
            16w0x1234: parse_h1;
            default: accept;
        }
    }
    state parse_h1 {
        pkt.extract<my_header_t>(hdr.h1);
        tmp_1 = pkt.lookahead<bit<24>>();
        main_meta._s1_type10 = tmp_1[23:8];
        main_meta._s1_type21 = tmp_1[7:0];
        transition select(tmp_1[7:0]) {
            8w0x1: parse_h2;
            default: accept;
        }
    }
    state parse_h2 {
        pkt.extract<my_header_t>(hdr.h2);
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
