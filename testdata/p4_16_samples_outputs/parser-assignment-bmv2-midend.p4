#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header test_h {
    bit<8> field;
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
    bit<8> tmp_3;
    bit<8> tmp_4;
    bit<8> tmp_5;
    bit<8> tmp_6;
    state start {
        meta.hs_next_index = 3w0;
        hdr.hs[3w0].setValid();
        hdr.hs[3w0].setValid();
        hdr.hs[3w0].field = 8w0;
        meta.hs_next_index = 3w1;
        hdr.hs[3w1].setValid();
        hdr.hs[3w1].field = 8w1;
        meta.hs_next_index = 3w2;
        hdr.hs[3w2].setValid();
        hdr.hs[3w2].field = 8w0;
        hdr.hs[3w2].field[3:0] = 4w2;
        meta.hs_next_index = 3w3;
        hdr.hs[hdr.hs[0].field + 8w3].setValid();
        hdr.hs[hdr.hs[1].field + 8w2].field = 8w0;
        hdr.hs[hdr.hs[2].field + 8w1].field[3:0] = 4w3;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[4].setValid();
        tmp_3 = pkt.lookahead<bit<8>>();
        tmp_0 = tmp_3;
        tmp = tmp_3;
        hdr.hs[4].setValid();
        hdr.hs[4].field = tmp_3;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[5].setValid();
        tmp_4 = pkt.lookahead<bit<8>>();
        hdr.hs[5].field = tmp_4;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        tmp_5 = pkt.lookahead<bit<8>>();
        tmp_2 = tmp_5;
        tmp_1 = tmp_5;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = tmp_1;
        meta.hs_next_index = meta.hs_next_index + 3w1;
        hdr.hs[meta.hs_next_index].setValid();
        tmp_6 = pkt.lookahead<bit<8>>();
        hdr.hs[meta.hs_next_index].field = tmp_6;
        transition accept;
    }
}

control DefaultDeparser(packet_out pkt, in header_t hdr) {
    apply {
        pkt.emit<test_h>(hdr.hs[0]);
        pkt.emit<test_h>(hdr.hs[1]);
        pkt.emit<test_h>(hdr.hs[2]);
        pkt.emit<test_h>(hdr.hs[3]);
        pkt.emit<test_h>(hdr.hs[4]);
        pkt.emit<test_h>(hdr.hs[5]);
        pkt.emit<test_h>(hdr.hs[6]);
        pkt.emit<test_h>(hdr.hs[7]);
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
