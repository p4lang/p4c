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
        pkt.extract(hdr.ethernet);
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
control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<8> tmp1;
    bit<8> tmp2;
    @name("foo1") action a1(bit<8> x, bit<8> y) {
        tmp1 = x >> 1;
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
            NoAction;
            a1;
            a3;
            a4;
            a5;
            foo1;
            foo2;
            foo3;
        }
        key = {
            hdr.ethernet.etherType: exact;
        }
        default_action = NoAction();
        size = 512;
    }
    apply {
        tmp1 = hdr.ethernet.srcAddr[7:0];
        tmp2 = hdr.ethernet.dstAddr[7:0];
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
        pkt.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
