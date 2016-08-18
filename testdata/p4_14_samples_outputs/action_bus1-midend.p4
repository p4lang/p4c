#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1_1;
    bit<32> f1_2;
    bit<32> f1_3;
    bit<32> f1_4;
    bit<32> f1_5;
    bit<32> f2_1;
    bit<32> f2_2;
    bit<32> f2_3;
    bit<32> f2_4;
    bit<32> f2_5;
    bit<32> f3_1;
    bit<32> f3_2;
    bit<32> f3_3;
    bit<32> f3_4;
    bit<32> f3_5;
    bit<32> f4_1;
    bit<32> f4_2;
    bit<32> f4_3;
    bit<32> f4_4;
    bit<32> f4_5;
    bit<32> f5_1;
    bit<32> f5_2;
    bit<32> f5_3;
    bit<32> f5_4;
    bit<32> f5_5;
    bit<32> f6_1;
    bit<32> f6_2;
    bit<32> f6_3;
    bit<32> f6_4;
    bit<32> f6_5;
    bit<32> f7_1;
    bit<32> f7_2;
    bit<32> f7_3;
    bit<32> f7_4;
    bit<32> f7_5;
    bit<32> f8_1;
    bit<32> f8_2;
    bit<32> f8_3;
    bit<32> f8_4;
    bit<32> f8_5;
}

struct metadata {
}

struct headers {
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    action NoAction_7() {
    }
    action NoAction_8() {
    }
    @name("set1") action set1(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f1_1 = v1;
        hdr.data.f1_2 = v2;
        hdr.data.f1_3 = v3;
        hdr.data.f1_4 = v4;
        hdr.data.f1_5 = v5;
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_1() {
    }
    @name("noop") action noop_2() {
    }
    @name("noop") action noop_3() {
    }
    @name("noop") action noop_4() {
    }
    @name("noop") action noop_5() {
    }
    @name("noop") action noop_6() {
    }
    @name("noop") action noop_7() {
    }
    @name("set2") action set2(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f2_1 = v1;
        hdr.data.f2_2 = v2;
        hdr.data.f2_3 = v3;
        hdr.data.f2_4 = v4;
        hdr.data.f2_5 = v5;
    }
    @name("set3") action set3(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f3_1 = v1;
        hdr.data.f3_2 = v2;
        hdr.data.f3_3 = v3;
        hdr.data.f3_4 = v4;
        hdr.data.f3_5 = v5;
    }
    @name("set4") action set4(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f4_1 = v1;
        hdr.data.f4_2 = v2;
        hdr.data.f4_3 = v3;
        hdr.data.f4_4 = v4;
        hdr.data.f4_5 = v5;
    }
    @name("set5") action set5(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f5_1 = v1;
        hdr.data.f5_2 = v2;
        hdr.data.f5_3 = v3;
        hdr.data.f5_4 = v4;
        hdr.data.f5_5 = v5;
    }
    @name("set6") action set6(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f6_1 = v1;
        hdr.data.f6_2 = v2;
        hdr.data.f6_3 = v3;
        hdr.data.f6_4 = v4;
        hdr.data.f6_5 = v5;
    }
    @name("set7") action set7(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f7_1 = v1;
        hdr.data.f7_2 = v2;
        hdr.data.f7_3 = v3;
        hdr.data.f7_4 = v4;
        hdr.data.f7_5 = v5;
    }
    @name("set8") action set8(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f8_1 = v1;
        hdr.data.f8_2 = v2;
        hdr.data.f8_3 = v3;
        hdr.data.f8_4 = v4;
        hdr.data.f8_5 = v5;
    }
    @name("tbl1") table tbl1_0() {
        actions = {
            set1();
            noop();
            NoAction_1();
        }
        key = {
            hdr.data.f1_1: exact;
        }
        default_action = NoAction_1();
    }
    @name("tbl2") table tbl2_0() {
        actions = {
            set2();
            noop_1();
            NoAction_2();
        }
        key = {
            hdr.data.f2_1: exact;
        }
        default_action = NoAction_2();
    }
    @name("tbl3") table tbl3_0() {
        actions = {
            set3();
            noop_2();
            NoAction_3();
        }
        key = {
            hdr.data.f3_1: exact;
        }
        default_action = NoAction_3();
    }
    @name("tbl4") table tbl4_0() {
        actions = {
            set4();
            noop_3();
            NoAction_4();
        }
        key = {
            hdr.data.f4_1: exact;
        }
        default_action = NoAction_4();
    }
    @name("tbl5") table tbl5_0() {
        actions = {
            set5();
            noop_4();
            NoAction_5();
        }
        key = {
            hdr.data.f5_1: exact;
        }
        default_action = NoAction_5();
    }
    @name("tbl6") table tbl6_0() {
        actions = {
            set6();
            noop_5();
            NoAction_6();
        }
        key = {
            hdr.data.f6_1: exact;
        }
        default_action = NoAction_6();
    }
    @name("tbl7") table tbl7_0() {
        actions = {
            set7();
            noop_6();
            NoAction_7();
        }
        key = {
            hdr.data.f7_1: exact;
        }
        default_action = NoAction_7();
    }
    @name("tbl8") table tbl8_0() {
        actions = {
            set8();
            noop_7();
            NoAction_8();
        }
        key = {
            hdr.data.f8_1: exact;
        }
        default_action = NoAction_8();
    }
    apply {
        tbl1_0.apply();
        tbl2_0.apply();
        tbl3_0.apply();
        tbl4_0.apply();
        tbl5_0.apply();
        tbl6_0.apply();
        tbl7_0.apply();
        tbl8_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
