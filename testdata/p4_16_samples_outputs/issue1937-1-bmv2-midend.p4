#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    h1_t h1;
}

struct metadata_t {
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.tmp") bit<8> tmp;
    @name(".foo") action foo_1() {
        hdr.h1.f1 = tmp >> 2;
    }
    @name(".foo") action foo_2() {
        hdr.h1.f2 = 8w1;
    }
    @hidden action act() {
        tmp = hdr.h1.f1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_foo {
        actions = {
            foo_1();
        }
        const default_action = foo_1();
    }
    @hidden table tbl_foo_0 {
        actions = {
            foo_2();
        }
        const default_action = foo_2();
    }
    apply {
        tbl_act.apply();
        tbl_foo.apply();
        tbl_foo_0.apply();
    }
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
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
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
