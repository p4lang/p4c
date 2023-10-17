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
        meta.hs_next_index = 0;
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index] = { 8w0 };
        meta.hs_next_index = meta.hs_next_index + 1;
        transition hs_1;
    }
    state hs_1 {
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = 8w1;
        meta.hs_next_index = meta.hs_next_index + 1;
        transition hs_2;
    }
    state hs_2 {
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = 8w0;
        hdr.hs[meta.hs_next_index].field[3:0] = 4w2;
        meta.hs_next_index = meta.hs_next_index + 1;
        transition hs_3;
    }
    state hs_3 {
        hdr.hs[hdr.hs[0].field + 3].setValid();
        hdr.hs[hdr.hs[1].field + 2].field = 8w0;
        hdr.hs[hdr.hs[2].field + 1].field[3:0] = 4w3;
        transition accept;
    }
}

control DefaultDeparser(packet_out pkt, in header_t hdr) {
    apply {
        pkt.emit(hdr);
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

V1Switch(TestParser(), EmptyChecksum(), EmptyControl(), EmptyControl(), EmptyChecksum(), DefaultDeparser()) main;
