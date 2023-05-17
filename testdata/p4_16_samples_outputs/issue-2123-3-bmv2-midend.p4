#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header h0_t {
    bit<8> f0;
}

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

header h3_t {
    bit<8> f3;
}

header h4_t {
    bit<8> f4;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    h0_t       h0;
    h1_t       h1;
    h2_t       h2;
    h3_t       h3;
    h4_t       h4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.srcAddr[7:0], hdr.ethernet.etherType) {
            (8w0x61, 16w0x800 &&& 16w0xfffc): parse_h0;
            (8w0x61, 16w0x804 &&& 16w0xfffe): parse_h0;
            (8w0x61, 16w0x806): parse_h0;
            (8w0x62 &&& 8w0xfe, 16w0x800 &&& 16w0xfffc): parse_h0;
            (8w0x62 &&& 8w0xfe, 16w0x804 &&& 16w0xfffe): parse_h0;
            (8w0x62 &&& 8w0xfe, 16w0x806): parse_h0;
            (8w0x64 &&& 8w0xfc, 16w0x800 &&& 16w0xfffc): parse_h0;
            (8w0x64 &&& 8w0xfc, 16w0x804 &&& 16w0xfffe): parse_h0;
            (8w0x64 &&& 8w0xfc, 16w0x806): parse_h0;
            (8w0x61, 16w0x901): parse_h1;
            (8w0x61, 16w0x902): parse_h1;
            (8w0x62 &&& 8w0xfe, 16w0x901): parse_h1;
            (8w0x62 &&& 8w0xfe, 16w0x902): parse_h1;
            (8w0x64 &&& 8w0xfc, 16w0x901): parse_h1;
            (8w0x64 &&& 8w0xfc, 16w0x902): parse_h1;
            (8w0x77, 16w0x801): parse_h2;
            (8w0x77, 16w0x802 &&& 16w0xfffe): parse_h2;
            (8w0x77, 16w0x804 &&& 16w0xfffe): parse_h2;
            (8w0x77, 16w0x806): parse_h2;
            (8w0x78 &&& 8w0xfc, 16w0x801): parse_h2;
            (8w0x78 &&& 8w0xfc, 16w0x802 &&& 16w0xfffe): parse_h2;
            (8w0x78 &&& 8w0xfc, 16w0x804 &&& 16w0xfffe): parse_h2;
            (8w0x78 &&& 8w0xfc, 16w0x806): parse_h2;
            (8w0x77, 16w0xa00 &&& 16w0xff80): parse_h3;
            (8w0x77, 16w0xa80 &&& 16w0xffe0): parse_h3;
            (8w0x77, 16w0xaa0 &&& 16w0xfff8): parse_h3;
            (8w0x77, 16w0xaa8 &&& 16w0xfffe): parse_h3;
            (8w0x77, 16w0xaaa): parse_h3;
            (8w0x78 &&& 8w0xfc, 16w0xa00 &&& 16w0xff80): parse_h3;
            (8w0x78 &&& 8w0xfc, 16w0xa80 &&& 16w0xffe0): parse_h3;
            (8w0x78 &&& 8w0xfc, 16w0xaa0 &&& 16w0xfff8): parse_h3;
            (8w0x78 &&& 8w0xfc, 16w0xaa8 &&& 16w0xfffe): parse_h3;
            (8w0x78 &&& 8w0xfc, 16w0xaaa): parse_h3;
            (default, 16w0xa00 &&& 16w0xff80): parse_h4;
            (default, 16w0xa80 &&& 16w0xffe0): parse_h4;
            (default, 16w0xaa0 &&& 16w0xfff8): parse_h4;
            (default, 16w0xaa8 &&& 16w0xfffe): parse_h4;
            (default, 16w0xaaa): parse_h4;
            default: accept;
        }
    }
    state parse_h0 {
        packet.extract<h0_t>(hdr.h0);
        transition accept;
    }
    state parse_h1 {
        packet.extract<h1_t>(hdr.h1);
        transition accept;
    }
    state parse_h2 {
        packet.extract<h2_t>(hdr.h2);
        transition accept;
    }
    state parse_h3 {
        packet.extract<h3_t>(hdr.h3);
        transition accept;
    }
    state parse_h4 {
        packet.extract<h4_t>(hdr.h4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.tmp") bit<1> tmp;
    @name("ingress.tmp_0") bit<1> tmp_0;
    @name("ingress.tmp_1") bit<1> tmp_1;
    @name("ingress.tmp_2") bit<1> tmp_2;
    @name("ingress.tmp_3") bit<1> tmp_3;
    @hidden action issue21233bmv2l108() {
        tmp = 1w1;
    }
    @hidden action issue21233bmv2l108_0() {
        tmp = 1w0;
    }
    @hidden action issue21233bmv2l109() {
        tmp_0 = 1w1;
    }
    @hidden action issue21233bmv2l109_0() {
        tmp_0 = 1w0;
    }
    @hidden action issue21233bmv2l108_1() {
        hdr.ethernet.dstAddr[44:44] = tmp;
    }
    @hidden action issue21233bmv2l110() {
        tmp_1 = 1w1;
    }
    @hidden action issue21233bmv2l110_0() {
        tmp_1 = 1w0;
    }
    @hidden action issue21233bmv2l109_1() {
        hdr.ethernet.dstAddr[43:43] = tmp_0;
    }
    @hidden action issue21233bmv2l111() {
        tmp_2 = 1w1;
    }
    @hidden action issue21233bmv2l111_0() {
        tmp_2 = 1w0;
    }
    @hidden action issue21233bmv2l110_1() {
        hdr.ethernet.dstAddr[42:42] = tmp_1;
    }
    @hidden action issue21233bmv2l112() {
        tmp_3 = 1w1;
    }
    @hidden action issue21233bmv2l112_0() {
        tmp_3 = 1w0;
    }
    @hidden action issue21233bmv2l111_1() {
        hdr.ethernet.dstAddr[41:41] = tmp_2;
    }
    @hidden action issue21233bmv2l112_1() {
        hdr.ethernet.dstAddr[40:40] = tmp_3;
        standard_metadata.egress_spec = 9w3;
    }
    @hidden table tbl_issue21233bmv2l108 {
        actions = {
            issue21233bmv2l108();
        }
        const default_action = issue21233bmv2l108();
    }
    @hidden table tbl_issue21233bmv2l108_0 {
        actions = {
            issue21233bmv2l108_0();
        }
        const default_action = issue21233bmv2l108_0();
    }
    @hidden table tbl_issue21233bmv2l108_1 {
        actions = {
            issue21233bmv2l108_1();
        }
        const default_action = issue21233bmv2l108_1();
    }
    @hidden table tbl_issue21233bmv2l109 {
        actions = {
            issue21233bmv2l109();
        }
        const default_action = issue21233bmv2l109();
    }
    @hidden table tbl_issue21233bmv2l109_0 {
        actions = {
            issue21233bmv2l109_0();
        }
        const default_action = issue21233bmv2l109_0();
    }
    @hidden table tbl_issue21233bmv2l109_1 {
        actions = {
            issue21233bmv2l109_1();
        }
        const default_action = issue21233bmv2l109_1();
    }
    @hidden table tbl_issue21233bmv2l110 {
        actions = {
            issue21233bmv2l110();
        }
        const default_action = issue21233bmv2l110();
    }
    @hidden table tbl_issue21233bmv2l110_0 {
        actions = {
            issue21233bmv2l110_0();
        }
        const default_action = issue21233bmv2l110_0();
    }
    @hidden table tbl_issue21233bmv2l110_1 {
        actions = {
            issue21233bmv2l110_1();
        }
        const default_action = issue21233bmv2l110_1();
    }
    @hidden table tbl_issue21233bmv2l111 {
        actions = {
            issue21233bmv2l111();
        }
        const default_action = issue21233bmv2l111();
    }
    @hidden table tbl_issue21233bmv2l111_0 {
        actions = {
            issue21233bmv2l111_0();
        }
        const default_action = issue21233bmv2l111_0();
    }
    @hidden table tbl_issue21233bmv2l111_1 {
        actions = {
            issue21233bmv2l111_1();
        }
        const default_action = issue21233bmv2l111_1();
    }
    @hidden table tbl_issue21233bmv2l112 {
        actions = {
            issue21233bmv2l112();
        }
        const default_action = issue21233bmv2l112();
    }
    @hidden table tbl_issue21233bmv2l112_0 {
        actions = {
            issue21233bmv2l112_0();
        }
        const default_action = issue21233bmv2l112_0();
    }
    @hidden table tbl_issue21233bmv2l112_1 {
        actions = {
            issue21233bmv2l112_1();
        }
        const default_action = issue21233bmv2l112_1();
    }
    apply {
        if (hdr.h4.isValid()) {
            tbl_issue21233bmv2l108.apply();
        } else {
            tbl_issue21233bmv2l108_0.apply();
        }
        tbl_issue21233bmv2l108_1.apply();
        if (hdr.h3.isValid()) {
            tbl_issue21233bmv2l109.apply();
        } else {
            tbl_issue21233bmv2l109_0.apply();
        }
        tbl_issue21233bmv2l109_1.apply();
        if (hdr.h2.isValid()) {
            tbl_issue21233bmv2l110.apply();
        } else {
            tbl_issue21233bmv2l110_0.apply();
        }
        tbl_issue21233bmv2l110_1.apply();
        if (hdr.h1.isValid()) {
            tbl_issue21233bmv2l111.apply();
        } else {
            tbl_issue21233bmv2l111_0.apply();
        }
        tbl_issue21233bmv2l111_1.apply();
        if (hdr.h0.isValid()) {
            tbl_issue21233bmv2l112.apply();
        } else {
            tbl_issue21233bmv2l112_0.apply();
        }
        tbl_issue21233bmv2l112_1.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<h0_t>(hdr.h0);
        packet.emit<h1_t>(hdr.h1);
        packet.emit<h2_t>(hdr.h2);
        packet.emit<h3_t>(hdr.h3);
        packet.emit<h4_t>(hdr.h4);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
