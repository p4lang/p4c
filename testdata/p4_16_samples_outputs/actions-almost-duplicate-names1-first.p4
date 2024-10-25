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

action foo1() {
}
@name("foo2") action foo2() {
}
@name(".baz") action foo3() {
}
control c1(inout headers_t hdr, inout bit<8> tmp1, inout bit<8> tmp2) {
    @name("bar") action suba1(bit<8> x, bit<8> y) {
        tmp1 = x;
        tmp2 = y >> 3;
    }
    table t2 {
        actions = {
            suba1();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        size = 32;
        default_action = NoAction();
    }
    apply {
        t2.apply();
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<8> tmp1;
    bit<8> tmp2;
    @name("foo1") action a1(bit<8> x, bit<8> y) {
        tmp1 = x >> 1;
        tmp2 = y;
    }
    @name("foo2") action a2(bit<8> x, bit<8> y) {
        tmp1 = x >> 2;
        tmp2 = y;
    }
    @name(".bar") action a3(bit<8> x, bit<8> y) {
        tmp1 = x >> 3;
        tmp2 = y;
    }
    @name("bar") action a4(bit<8> x, bit<8> y) {
        tmp1 = x >> 4;
        tmp2 = y;
    }
    @name("baz") action a5(bit<8> x, bit<8> y) {
        tmp1 = x >> 5;
        tmp2 = y;
    }
    table t1 {
        actions = {
            NoAction();
            a1();
            a2();
            a3();
            a4();
            a5();
            foo1();
            foo2();
            foo3();
        }
        key = {
            hdr.ethernet.etherType: exact @name("hdr.ethernet.etherType");
        }
        default_action = NoAction();
        size = 512;
    }
    @name("c1") c1() c1_inst;
    apply {
        tmp1 = hdr.ethernet.srcAddr[7:0];
        tmp2 = hdr.ethernet.dstAddr[7:0];
        c1_inst.apply(hdr, tmp1, tmp2);
        t1.apply();
        hdr.ethernet.etherType = (bit<16>)(tmp1 - tmp2);
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
