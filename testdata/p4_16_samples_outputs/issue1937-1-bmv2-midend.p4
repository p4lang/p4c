#include <core.p4>
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
    bit<8> tmp;
    bit<8> tmp_0;
    @name(".foo") action foo() {
        tmp = tmp_0 >> 2;
    }
    @name(".foo") action foo_0() {
        hdr.h1.f2 = 8w1;
    }
    @hidden action act() {
        tmp_0 = hdr.h1.f1;
    }
    @hidden action act_0() {
        hdr.h1.f1 = tmp;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_foo {
        actions = {
            foo();
        }
        const default_action = foo();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_foo_0 {
        actions = {
            foo_0();
        }
        const default_action = foo_0();
    }
    apply {
        tbl_act.apply();
        tbl_foo.apply();
        tbl_act_0.apply();
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

