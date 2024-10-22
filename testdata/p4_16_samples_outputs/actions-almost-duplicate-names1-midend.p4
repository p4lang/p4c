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

parser parserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.tmp1") bit<8> tmp1_0;
    @name("ingressImpl.tmp2") bit<8> tmp2_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".foo1") action foo1_0() {
    }
    @name("ingressImpl.foo2") action foo2_0() {
    }
    @name(".baz") action foo3_0() {
    }
    @name("ingressImpl.foo1") action a1(@name("x") bit<8> x, @name("y") bit<8> y) {
        tmp1_0 = x >> 1;
        tmp2_0 = y;
    }
    @name(".bar") action a3(@name("x") bit<8> x_4, @name("y") bit<8> y_4) {
        tmp1_0 = x_4 >> 3;
        tmp2_0 = y_4;
    }
    @name("ingressImpl.bar") action a4(@name("x") bit<8> x_5, @name("y") bit<8> y_5) {
        tmp1_0 = x_5 >> 4;
        tmp2_0 = y_5;
    }
    @name("ingressImpl.baz") action a5(@name("x") bit<8> x_6, @name("y") bit<8> y_6) {
        tmp1_0 = x_6 >> 5;
        tmp2_0 = y_6;
    }
    @name("ingressImpl.t1") table t1_0 {
        actions = {
            NoAction_1();
            a1();
            a3();
            a4();
            a5();
            foo1_0();
            foo2_0();
            foo3_0();
        }
        key = {
            hdr.ethernet.etherType: exact @name("hdr.ethernet.etherType");
        }
        default_action = NoAction_1();
        size = 512;
    }
    @hidden action actionsalmostduplicatenames1l113() {
        tmp1_0 = hdr.ethernet.srcAddr[7:0];
        tmp2_0 = hdr.ethernet.dstAddr[7:0];
    }
    @hidden action actionsalmostduplicatenames1l119() {
        hdr.ethernet.etherType = (bit<16>)(tmp1_0 - tmp2_0);
    }
    @hidden table tbl_actionsalmostduplicatenames1l113 {
        actions = {
            actionsalmostduplicatenames1l113();
        }
        const default_action = actionsalmostduplicatenames1l113();
    }
    @hidden table tbl_actionsalmostduplicatenames1l119 {
        actions = {
            actionsalmostduplicatenames1l119();
        }
        const default_action = actionsalmostduplicatenames1l119();
    }
    apply {
        tbl_actionsalmostduplicatenames1l113.apply();
        t1_0.apply();
        tbl_actionsalmostduplicatenames1l119.apply();
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

control deparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
