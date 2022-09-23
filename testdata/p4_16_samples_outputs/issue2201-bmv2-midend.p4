#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

enum MyEnum_t {
    VAL1,
    VAL2,
    VAL3
}

struct tuple_0 {
    bool f0;
}

struct tuple_1 {
    bit<1> f0;
}

struct tuple_2 {
    int<8> f0;
}

struct tuple_3 {
    bit<8> f0;
}

struct tuple_4 {
    MyEnum_t f0;
}

struct tuple_5 {
    bit<10> f0;
}

struct tuple_6 {
    ethernet_t f0;
}

struct tuple_7 {
    standard_metadata_t f0;
}

struct tuple_8 {
    error f0;
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.enum1") MyEnum_t enum1_0;
    @name("ingressImpl.serenum1") bit<10> serenum1_0;
    @hidden action issue2201bmv2l111() {
        enum1_0 = MyEnum_t.VAL1;
    }
    @hidden action issue2201bmv2l113() {
        enum1_0 = MyEnum_t.VAL2;
    }
    @hidden action issue2201bmv2l115() {
        enum1_0 = MyEnum_t.VAL3;
    }
    @hidden action issue2201bmv2l79() {
        log_msg<tuple_0>("bool1={}", (tuple_0){f0 = (bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("(bit<1>) bool1={}", (tuple_1){f0 = (bit<1>)(bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("(bit<1>) (!bool1)={}", (tuple_1){f0 = (bit<1>)!(bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("bit1={}", (tuple_1){f0 = hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_2>("signed1={}", (tuple_2){f0 = -8s128});
        log_msg<tuple_3>("unsigned1={}", (tuple_3){f0 = 8w128});
    }
    @hidden action issue2201bmv2l126() {
        serenum1_0 = 10w17;
    }
    @hidden action issue2201bmv2l128() {
        serenum1_0 = 10w23;
    }
    @hidden action issue2201bmv2l130() {
        serenum1_0 = 10w19;
    }
    @hidden action issue2201bmv2l122() {
        log_msg<tuple_4>("enum1={}", (tuple_4){f0 = enum1_0});
    }
    @hidden action issue2201bmv2l134() {
        log_msg<tuple_5>("serenum1={}", (tuple_5){f0 = serenum1_0});
        log_msg<tuple_6>("hdr.ethernet={}", (tuple_6){f0 = hdr.ethernet});
        log_msg<tuple_7>("stdmeta={}", (tuple_7){f0 = stdmeta});
        log_msg<tuple_8>("error.PacketTooShort={}", (tuple_8){f0 = error.PacketTooShort});
    }
    @hidden table tbl_issue2201bmv2l79 {
        actions = {
            issue2201bmv2l79();
        }
        const default_action = issue2201bmv2l79();
    }
    @hidden table tbl_issue2201bmv2l111 {
        actions = {
            issue2201bmv2l111();
        }
        const default_action = issue2201bmv2l111();
    }
    @hidden table tbl_issue2201bmv2l113 {
        actions = {
            issue2201bmv2l113();
        }
        const default_action = issue2201bmv2l113();
    }
    @hidden table tbl_issue2201bmv2l115 {
        actions = {
            issue2201bmv2l115();
        }
        const default_action = issue2201bmv2l115();
    }
    @hidden table tbl_issue2201bmv2l122 {
        actions = {
            issue2201bmv2l122();
        }
        const default_action = issue2201bmv2l122();
    }
    @hidden table tbl_issue2201bmv2l126 {
        actions = {
            issue2201bmv2l126();
        }
        const default_action = issue2201bmv2l126();
    }
    @hidden table tbl_issue2201bmv2l128 {
        actions = {
            issue2201bmv2l128();
        }
        const default_action = issue2201bmv2l128();
    }
    @hidden table tbl_issue2201bmv2l130 {
        actions = {
            issue2201bmv2l130();
        }
        const default_action = issue2201bmv2l130();
    }
    @hidden table tbl_issue2201bmv2l134 {
        actions = {
            issue2201bmv2l134();
        }
        const default_action = issue2201bmv2l134();
    }
    apply {
        tbl_issue2201bmv2l79.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue2201bmv2l111.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue2201bmv2l113.apply();
        } else {
            tbl_issue2201bmv2l115.apply();
        }
        tbl_issue2201bmv2l122.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue2201bmv2l126.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue2201bmv2l128.apply();
        } else {
            tbl_issue2201bmv2l130.apply();
        }
        tbl_issue2201bmv2l134.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

