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
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name(".foo1") action foo1_0() {
    }
    @name(".foo2") action foo2_0() {
    }
    @name(".baz") action foo3_0() {
    }
    @name("ingressImpl.foo1") action a1(@name("x") bit<8> x, @name("y") bit<8> y) {
        tmp1_0 = x >> 1;
        tmp2_0 = y;
    }
    @name("ingressImpl.foo2") action a2(@name("x") bit<8> x_6, @name("y") bit<8> y_6) {
        tmp1_0 = x_6 >> 2;
        tmp2_0 = y_6;
    }
    @name(".bar") action a3(@name("x") bit<8> x_7, @name("y") bit<8> y_7) {
        tmp1_0 = x_7 >> 3;
        tmp2_0 = y_7;
    }
    @name("ingressImpl.bar") action a4(@name("x") bit<8> x_8, @name("y") bit<8> y_8) {
        tmp1_0 = x_8 >> 4;
        tmp2_0 = y_8;
    }
    @name("ingressImpl.baz") action a5(@name("x") bit<8> x_9, @name("y") bit<8> y_9) {
        tmp1_0 = x_9 >> 5;
        tmp2_0 = y_9;
    }
    @name("ingressImpl.t1") table t1_0 {
        actions = {
            NoAction_1();
            a1();
            a2();
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
    @name("ingressImpl.c1.bar") action c1_bar_0(@name("x") bit<8> x_10, @name("y") bit<8> y_10) {
        tmp1_0 = x_10;
        tmp2_0 = y_10 >> 3;
    }
    @name("ingressImpl.c1.t2") table c1_t2 {
        actions = {
            c1_bar_0();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        size = 32;
        default_action = NoAction_2();
    }
    apply {
        tmp1_0 = hdr.ethernet.srcAddr[7:0];
        tmp2_0 = hdr.ethernet.dstAddr[7:0];
        c1_t2.apply();
        t1_0.apply();
        hdr.ethernet.etherType = (bit<16>)(tmp1_0 - tmp2_0);
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
