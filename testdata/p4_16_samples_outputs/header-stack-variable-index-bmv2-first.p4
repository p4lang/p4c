#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header test_h {
    bit<8> field;
}

struct header_t {
    test_h[4] hs;
}

struct metadata_t {
    bit<2> hs_next_index;
}

parser TestParser(packet_in pkt, out header_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    state start {
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index] = (test_h){field = 8w0};
        meta.hs_next_index = meta.hs_next_index + 2w1;
        transition p1;
    }
    state p1 {
        hdr.hs[hdr.hs[0].field + 8w1].setValid();
        hdr.hs[hdr.hs[0].field + 8w1] = (test_h){field = 8w1};
        meta.hs_next_index = meta.hs_next_index + 2w1;
        transition accept;
    }
}

control TestControl(inout header_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    apply {
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index] = (test_h){field = 8w3};
        hdr.hs[hdr.hs[2].field + 8w1].setValid();
        hdr.hs[hdr.hs[2].field + 8w1] = (test_h){field = 8w3};
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

V1Switch<header_t, metadata_t>(TestParser(), EmptyChecksum(), TestControl(), EmptyControl(), EmptyChecksum(), DefaultDeparser()) main;
