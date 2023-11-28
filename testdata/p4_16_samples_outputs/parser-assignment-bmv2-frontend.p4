#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<8> field_t;
header test_h {
    field_t field;
}

struct header_t {
    test_h[8] hs;
}

struct metadata_t {
    bit<3> hs_next_index;
}

parser TestParser(packet_in pkt, out header_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    @name("TestParser.tmp") bit<8> tmp;
    @name("TestParser.tmp_0") bit<8> tmp_0;
    @name("TestParser.tmp_1") bit<8> tmp_1;
    @name("TestParser.tmp_2") bit<8> tmp_2;
    state start {
        meta.hs_next_index = 3w0;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index] = (test_h){field = 8w0};
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = 8w1;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = 8w0;
        hdr.hs[meta.hs_next_index].field[3:0] = 4w2;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[hdr.hs[0].field + 8w3].setValid();
        hdr.hs[hdr.hs[1].field + 8w2].field = 8w0;
        hdr.hs[hdr.hs[2].field + 8w1].field[3:0] = 4w3;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[4].setValid();
        tmp_0 = pkt.lookahead<field_t>();
        tmp = tmp_0;
        hdr.hs[4].setValid();
        hdr.hs[4] = (test_h){field = tmp};
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[5].setValid();
        hdr.hs[5].field = pkt.lookahead<field_t>();
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        tmp_2 = pkt.lookahead<field_t>();
        tmp_1 = tmp_2;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index] = (test_h){field = tmp_1};
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = pkt.lookahead<field_t>();
        transition accept;
    }
}

control DefaultDeparser(packet_out pkt, in header_t hdr) {
    apply {
        pkt.emit<header_t>(hdr);
    }
}

control EmptyControl(inout header_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control EmptyChecksum(inout header_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<header_t, metadata_t>(TestParser(), EmptyChecksum(), EmptyControl(), EmptyControl(), EmptyChecksum(), DefaultDeparser()) main;
