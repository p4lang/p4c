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
        hdr.hs[meta.hs_next_index].setValid();
        hdr.hs[meta.hs_next_index].field = 8w0;
        meta.hs_next_index = meta.hs_next_index + 2w1;
        hdr.hs[hdr.hs[0].field + 8w1].setValid();
        hdr.hs[hdr.hs[0].field + 8w1].setValid();
        hdr.hs[hdr.hs[0].field + 8w1].field = 8w1;
        meta.hs_next_index = meta.hs_next_index + 2w1;
        transition accept;
    }
}

control TestControl(inout header_t hdr, inout metadata_t meta, inout standard_metadata_t std_meta) {
    bit<2> hsiVar;
    bit<8> hsiVar_0;
    @hidden action headerstackvariableindexbmv2l44() {
        hdr.hs[2w0].setValid();
    }
    @hidden action headerstackvariableindexbmv2l44_0() {
        hdr.hs[2w1].setValid();
    }
    @hidden action headerstackvariableindexbmv2l44_1() {
        hdr.hs[2w2].setValid();
    }
    @hidden action headerstackvariableindexbmv2l44_2() {
        hdr.hs[2w3].setValid();
    }
    @hidden action headerstackvariableindexbmv2l44_3() {
        hsiVar = meta.hs_next_index;
    }
    @hidden action headerstackvariableindexbmv2l45() {
        hdr.hs[2w0].setValid();
    }
    @hidden action headerstackvariableindexbmv2l45_0() {
        hdr.hs[2w1].setValid();
    }
    @hidden action headerstackvariableindexbmv2l45_1() {
        hdr.hs[2w2].setValid();
    }
    @hidden action headerstackvariableindexbmv2l45_2() {
        hdr.hs[2w3].setValid();
    }
    @hidden action headerstackvariableindexbmv2l45_3() {
        hsiVar = meta.hs_next_index;
    }
    @hidden action headerstackvariableindexbmv2l45_4() {
        hdr.hs[2w0].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l45_5() {
        hdr.hs[2w1].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l45_6() {
        hdr.hs[2w2].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l45_7() {
        hdr.hs[2w3].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l45_8() {
        hsiVar = meta.hs_next_index;
    }
    @hidden action headerstackvariableindexbmv2l48() {
        hdr.hs[8w0].setValid();
    }
    @hidden action headerstackvariableindexbmv2l48_0() {
        hdr.hs[8w1].setValid();
    }
    @hidden action headerstackvariableindexbmv2l48_1() {
        hdr.hs[8w2].setValid();
    }
    @hidden action headerstackvariableindexbmv2l48_2() {
        hdr.hs[8w3].setValid();
    }
    @hidden action headerstackvariableindexbmv2l48_3() {
        hsiVar_0 = hdr.hs[2].field + 8w1;
    }
    @hidden action headerstackvariableindexbmv2l49() {
        hdr.hs[8w0].setValid();
    }
    @hidden action headerstackvariableindexbmv2l49_0() {
        hdr.hs[8w1].setValid();
    }
    @hidden action headerstackvariableindexbmv2l49_1() {
        hdr.hs[8w2].setValid();
    }
    @hidden action headerstackvariableindexbmv2l49_2() {
        hdr.hs[8w3].setValid();
    }
    @hidden action headerstackvariableindexbmv2l49_3() {
        hsiVar_0 = hdr.hs[2].field + 8w1;
    }
    @hidden action headerstackvariableindexbmv2l49_4() {
        hdr.hs[8w0].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l49_5() {
        hdr.hs[8w1].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l49_6() {
        hdr.hs[8w2].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l49_7() {
        hdr.hs[8w3].field = 8w3;
    }
    @hidden action headerstackvariableindexbmv2l49_8() {
        hsiVar_0 = hdr.hs[2].field + 8w1;
    }
    @hidden table tbl_headerstackvariableindexbmv2l44 {
        actions = {
            headerstackvariableindexbmv2l44_3();
        }
        const default_action = headerstackvariableindexbmv2l44_3();
    }
    @hidden table tbl_headerstackvariableindexbmv2l44_0 {
        actions = {
            headerstackvariableindexbmv2l44();
        }
        const default_action = headerstackvariableindexbmv2l44();
    }
    @hidden table tbl_headerstackvariableindexbmv2l44_1 {
        actions = {
            headerstackvariableindexbmv2l44_0();
        }
        const default_action = headerstackvariableindexbmv2l44_0();
    }
    @hidden table tbl_headerstackvariableindexbmv2l44_2 {
        actions = {
            headerstackvariableindexbmv2l44_1();
        }
        const default_action = headerstackvariableindexbmv2l44_1();
    }
    @hidden table tbl_headerstackvariableindexbmv2l44_3 {
        actions = {
            headerstackvariableindexbmv2l44_2();
        }
        const default_action = headerstackvariableindexbmv2l44_2();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45 {
        actions = {
            headerstackvariableindexbmv2l45_3();
        }
        const default_action = headerstackvariableindexbmv2l45_3();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_0 {
        actions = {
            headerstackvariableindexbmv2l45();
        }
        const default_action = headerstackvariableindexbmv2l45();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_1 {
        actions = {
            headerstackvariableindexbmv2l45_0();
        }
        const default_action = headerstackvariableindexbmv2l45_0();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_2 {
        actions = {
            headerstackvariableindexbmv2l45_1();
        }
        const default_action = headerstackvariableindexbmv2l45_1();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_3 {
        actions = {
            headerstackvariableindexbmv2l45_2();
        }
        const default_action = headerstackvariableindexbmv2l45_2();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_4 {
        actions = {
            headerstackvariableindexbmv2l45_8();
        }
        const default_action = headerstackvariableindexbmv2l45_8();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_5 {
        actions = {
            headerstackvariableindexbmv2l45_4();
        }
        const default_action = headerstackvariableindexbmv2l45_4();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_6 {
        actions = {
            headerstackvariableindexbmv2l45_5();
        }
        const default_action = headerstackvariableindexbmv2l45_5();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_7 {
        actions = {
            headerstackvariableindexbmv2l45_6();
        }
        const default_action = headerstackvariableindexbmv2l45_6();
    }
    @hidden table tbl_headerstackvariableindexbmv2l45_8 {
        actions = {
            headerstackvariableindexbmv2l45_7();
        }
        const default_action = headerstackvariableindexbmv2l45_7();
    }
    @hidden table tbl_headerstackvariableindexbmv2l48 {
        actions = {
            headerstackvariableindexbmv2l48_3();
        }
        const default_action = headerstackvariableindexbmv2l48_3();
    }
    @hidden table tbl_headerstackvariableindexbmv2l48_0 {
        actions = {
            headerstackvariableindexbmv2l48();
        }
        const default_action = headerstackvariableindexbmv2l48();
    }
    @hidden table tbl_headerstackvariableindexbmv2l48_1 {
        actions = {
            headerstackvariableindexbmv2l48_0();
        }
        const default_action = headerstackvariableindexbmv2l48_0();
    }
    @hidden table tbl_headerstackvariableindexbmv2l48_2 {
        actions = {
            headerstackvariableindexbmv2l48_1();
        }
        const default_action = headerstackvariableindexbmv2l48_1();
    }
    @hidden table tbl_headerstackvariableindexbmv2l48_3 {
        actions = {
            headerstackvariableindexbmv2l48_2();
        }
        const default_action = headerstackvariableindexbmv2l48_2();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49 {
        actions = {
            headerstackvariableindexbmv2l49_3();
        }
        const default_action = headerstackvariableindexbmv2l49_3();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_0 {
        actions = {
            headerstackvariableindexbmv2l49();
        }
        const default_action = headerstackvariableindexbmv2l49();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_1 {
        actions = {
            headerstackvariableindexbmv2l49_0();
        }
        const default_action = headerstackvariableindexbmv2l49_0();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_2 {
        actions = {
            headerstackvariableindexbmv2l49_1();
        }
        const default_action = headerstackvariableindexbmv2l49_1();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_3 {
        actions = {
            headerstackvariableindexbmv2l49_2();
        }
        const default_action = headerstackvariableindexbmv2l49_2();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_4 {
        actions = {
            headerstackvariableindexbmv2l49_8();
        }
        const default_action = headerstackvariableindexbmv2l49_8();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_5 {
        actions = {
            headerstackvariableindexbmv2l49_4();
        }
        const default_action = headerstackvariableindexbmv2l49_4();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_6 {
        actions = {
            headerstackvariableindexbmv2l49_5();
        }
        const default_action = headerstackvariableindexbmv2l49_5();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_7 {
        actions = {
            headerstackvariableindexbmv2l49_6();
        }
        const default_action = headerstackvariableindexbmv2l49_6();
    }
    @hidden table tbl_headerstackvariableindexbmv2l49_8 {
        actions = {
            headerstackvariableindexbmv2l49_7();
        }
        const default_action = headerstackvariableindexbmv2l49_7();
    }
    apply {
        tbl_headerstackvariableindexbmv2l44.apply();
        if (hsiVar == 2w0) {
            tbl_headerstackvariableindexbmv2l44_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_headerstackvariableindexbmv2l44_1.apply();
        } else if (hsiVar == 2w2) {
            tbl_headerstackvariableindexbmv2l44_2.apply();
        } else if (hsiVar == 2w3) {
            tbl_headerstackvariableindexbmv2l44_3.apply();
        }
        tbl_headerstackvariableindexbmv2l45.apply();
        if (hsiVar == 2w0) {
            tbl_headerstackvariableindexbmv2l45_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_headerstackvariableindexbmv2l45_1.apply();
        } else if (hsiVar == 2w2) {
            tbl_headerstackvariableindexbmv2l45_2.apply();
        } else if (hsiVar == 2w3) {
            tbl_headerstackvariableindexbmv2l45_3.apply();
        }
        tbl_headerstackvariableindexbmv2l45_4.apply();
        if (hsiVar == 2w0) {
            tbl_headerstackvariableindexbmv2l45_5.apply();
        } else if (hsiVar == 2w1) {
            tbl_headerstackvariableindexbmv2l45_6.apply();
        } else if (hsiVar == 2w2) {
            tbl_headerstackvariableindexbmv2l45_7.apply();
        } else if (hsiVar == 2w3) {
            tbl_headerstackvariableindexbmv2l45_8.apply();
        }
        tbl_headerstackvariableindexbmv2l48.apply();
        if (hsiVar_0 == 8w0) {
            tbl_headerstackvariableindexbmv2l48_0.apply();
        } else if (hsiVar_0 == 8w1) {
            tbl_headerstackvariableindexbmv2l48_1.apply();
        } else if (hsiVar_0 == 8w2) {
            tbl_headerstackvariableindexbmv2l48_2.apply();
        } else if (hsiVar_0 == 8w3) {
            tbl_headerstackvariableindexbmv2l48_3.apply();
        }
        tbl_headerstackvariableindexbmv2l49.apply();
        if (hsiVar_0 == 8w0) {
            tbl_headerstackvariableindexbmv2l49_0.apply();
        } else if (hsiVar_0 == 8w1) {
            tbl_headerstackvariableindexbmv2l49_1.apply();
        } else if (hsiVar_0 == 8w2) {
            tbl_headerstackvariableindexbmv2l49_2.apply();
        } else if (hsiVar_0 == 8w3) {
            tbl_headerstackvariableindexbmv2l49_3.apply();
        }
        tbl_headerstackvariableindexbmv2l49_4.apply();
        if (hsiVar_0 == 8w0) {
            tbl_headerstackvariableindexbmv2l49_5.apply();
        } else if (hsiVar_0 == 8w1) {
            tbl_headerstackvariableindexbmv2l49_6.apply();
        } else if (hsiVar_0 == 8w2) {
            tbl_headerstackvariableindexbmv2l49_7.apply();
        } else if (hsiVar_0 == 8w3) {
            tbl_headerstackvariableindexbmv2l49_8.apply();
        }
    }
}

control DefaultDeparser(packet_out pkt, in header_t hdr) {
    apply {
        pkt.emit<test_h>(hdr.hs[0]);
        pkt.emit<test_h>(hdr.hs[1]);
        pkt.emit<test_h>(hdr.hs[2]);
        pkt.emit<test_h>(hdr.hs[3]);
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
