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
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set1") action set1(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f1_1 = v1;
        hdr.data.f1_2 = v2;
        hdr.data.f1_3 = v3;
        hdr.data.f1_4 = v4;
        hdr.data.f1_5 = v5;
    }
    @name(".noop") action noop() {
    }
    @name(".set2") action set2(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f2_1 = v1;
        hdr.data.f2_2 = v2;
        hdr.data.f2_3 = v3;
        hdr.data.f2_4 = v4;
        hdr.data.f2_5 = v5;
    }
    @name(".set3") action set3(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f3_1 = v1;
        hdr.data.f3_2 = v2;
        hdr.data.f3_3 = v3;
        hdr.data.f3_4 = v4;
        hdr.data.f3_5 = v5;
    }
    @name(".set4") action set4(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f4_1 = v1;
        hdr.data.f4_2 = v2;
        hdr.data.f4_3 = v3;
        hdr.data.f4_4 = v4;
        hdr.data.f4_5 = v5;
    }
    @name(".set5") action set5(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f5_1 = v1;
        hdr.data.f5_2 = v2;
        hdr.data.f5_3 = v3;
        hdr.data.f5_4 = v4;
        hdr.data.f5_5 = v5;
    }
    @name(".set6") action set6(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f6_1 = v1;
        hdr.data.f6_2 = v2;
        hdr.data.f6_3 = v3;
        hdr.data.f6_4 = v4;
        hdr.data.f6_5 = v5;
    }
    @name(".set7") action set7(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f7_1 = v1;
        hdr.data.f7_2 = v2;
        hdr.data.f7_3 = v3;
        hdr.data.f7_4 = v4;
        hdr.data.f7_5 = v5;
    }
    @name(".set8") action set8(bit<32> v1, bit<32> v2, bit<32> v3, bit<32> v4, bit<32> v5) {
        hdr.data.f8_1 = v1;
        hdr.data.f8_2 = v2;
        hdr.data.f8_3 = v3;
        hdr.data.f8_4 = v4;
        hdr.data.f8_5 = v5;
    }
    @name(".tbl1") table tbl1 {
        actions = {
            set1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1_1: exact @name("data.f1_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl2") table tbl2 {
        actions = {
            set2();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2_1: exact @name("data.f2_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl3") table tbl3 {
        actions = {
            set3();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f3_1: exact @name("data.f3_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl4") table tbl4 {
        actions = {
            set4();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f4_1: exact @name("data.f4_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl5") table tbl5 {
        actions = {
            set5();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f5_1: exact @name("data.f5_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl6") table tbl6 {
        actions = {
            set6();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f6_1: exact @name("data.f6_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl7") table tbl7 {
        actions = {
            set7();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f7_1: exact @name("data.f7_1") ;
        }
        default_action = NoAction();
    }
    @name(".tbl8") table tbl8 {
        actions = {
            set8();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f8_1: exact @name("data.f8_1") ;
        }
        default_action = NoAction();
    }
    apply {
        tbl1.apply();
        tbl2.apply();
        tbl3.apply();
        tbl4.apply();
        tbl5.apply();
        tbl6.apply();
        tbl7.apply();
        tbl8.apply();
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

