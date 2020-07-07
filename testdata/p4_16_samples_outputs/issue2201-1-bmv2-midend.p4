#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    bool field;
}

struct tuple_1 {
    bit<1> field_0;
}

struct tuple_2 {
    int<8> field_1;
}

struct tuple_3 {
    bit<8> field_2;
}

struct tuple_4 {
    MyEnum_t field_3;
}

struct tuple_5 {
    bit<10> field_4;
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    MyEnum_t enum1_0;
    bit<10> serenum1_0;
    @hidden action issue22011bmv2l111() {
        enum1_0 = MyEnum_t.VAL1;
    }
    @hidden action issue22011bmv2l113() {
        enum1_0 = MyEnum_t.VAL2;
    }
    @hidden action issue22011bmv2l115() {
        enum1_0 = MyEnum_t.VAL3;
    }
    @hidden action issue22011bmv2l79() {
        log_msg<tuple_0>("bool1={}", { (bool)hdr.ethernet.dstAddr[0:0] });
        log_msg<tuple_1>("(bit<1>) bool1={}", { (bit<1>)(bool)hdr.ethernet.dstAddr[0:0] });
        log_msg<tuple_1>("(bit<1>) (!bool1)={}", { (bit<1>)!(bool)hdr.ethernet.dstAddr[0:0] });
        log_msg<tuple_1>("bit1={}", { hdr.ethernet.dstAddr[0:0] });
        log_msg<tuple_2>("signed1={}", { -8s128 });
        log_msg<tuple_3>("unsigned1={}", { 8w128 });
    }
    @hidden action issue22011bmv2l126() {
        serenum1_0 = 10w17;
    }
    @hidden action issue22011bmv2l128() {
        serenum1_0 = 10w23;
    }
    @hidden action issue22011bmv2l130() {
        serenum1_0 = 10w19;
    }
    @hidden action issue22011bmv2l122() {
        log_msg<tuple_4>("enum1={}", { enum1_0 });
    }
    @hidden action issue22011bmv2l134() {
        log_msg<tuple_5>("serenum1={}", { serenum1_0 });
    }
    @hidden table tbl_issue22011bmv2l79 {
        actions = {
            issue22011bmv2l79();
        }
        const default_action = issue22011bmv2l79();
    }
    @hidden table tbl_issue22011bmv2l111 {
        actions = {
            issue22011bmv2l111();
        }
        const default_action = issue22011bmv2l111();
    }
    @hidden table tbl_issue22011bmv2l113 {
        actions = {
            issue22011bmv2l113();
        }
        const default_action = issue22011bmv2l113();
    }
    @hidden table tbl_issue22011bmv2l115 {
        actions = {
            issue22011bmv2l115();
        }
        const default_action = issue22011bmv2l115();
    }
    @hidden table tbl_issue22011bmv2l122 {
        actions = {
            issue22011bmv2l122();
        }
        const default_action = issue22011bmv2l122();
    }
    @hidden table tbl_issue22011bmv2l126 {
        actions = {
            issue22011bmv2l126();
        }
        const default_action = issue22011bmv2l126();
    }
    @hidden table tbl_issue22011bmv2l128 {
        actions = {
            issue22011bmv2l128();
        }
        const default_action = issue22011bmv2l128();
    }
    @hidden table tbl_issue22011bmv2l130 {
        actions = {
            issue22011bmv2l130();
        }
        const default_action = issue22011bmv2l130();
    }
    @hidden table tbl_issue22011bmv2l134 {
        actions = {
            issue22011bmv2l134();
        }
        const default_action = issue22011bmv2l134();
    }
    apply {
        tbl_issue22011bmv2l79.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue22011bmv2l111.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue22011bmv2l113.apply();
        } else {
            tbl_issue22011bmv2l115.apply();
        }
        tbl_issue22011bmv2l122.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue22011bmv2l126.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue22011bmv2l128.apply();
        } else {
            tbl_issue22011bmv2l130.apply();
        }
        tbl_issue22011bmv2l134.apply();
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

