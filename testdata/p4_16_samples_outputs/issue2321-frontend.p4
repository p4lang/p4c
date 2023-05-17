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

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.x") bit<48> x;
    @name("ingressImpl.y") bit<16> y_0;
    @name("ingressImpl.z") bit<16> z_0;
    @name("ingressImpl.x_0") bit<16> x_3;
    @name("ingressImpl.retval") bit<16> retval;
    @name("ingressImpl.x_2") bit<16> x_4;
    @name("ingressImpl.retval") bit<16> retval_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingressImpl.my_drop") action my_drop() {
        mark_to_drop(stdmeta);
    }
    @name("ingressImpl.a1") action a1() {
        x = hdr.ethernet.dstAddr;
        hdr.ethernet.srcAddr = 48w5;
        x = x + 48w281474976710649;
        hdr.ethernet.dstAddr = x;
    }
    @name("ingressImpl.a2") action a2() {
        x_3 = hdr.ethernet.etherType;
        x_3 = x_3 | 16w7;
        retval = x_3 >> 1;
        hdr.ethernet.etherType = x_3;
        y_0 = retval;
        x_4 = hdr.ethernet.etherType;
        x_4 = x_4 | 16w7;
        retval_1 = x_4 >> 1;
        hdr.ethernet.etherType = x_4;
        z_0 = retval_1;
        hdr.ethernet.srcAddr[15:0] = y_0 + z_0;
    }
    @name("ingressImpl.t1") table t1_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            a1();
            a2();
            my_drop();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        t1_0.apply();
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
