#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

struct h {
    bit<1> b;
}

struct metadata {
    @name("m") 
    h m;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("hdr_1") headers hdr_1_0;
    @name("meta_1") metadata meta_1_0;
    @name("standard_metadata_1") standard_metadata_t standard_metadata_1_0;
    @name("hdr_0") headers hdr_0_0;
    @name("meta_0") metadata meta_0_0;
    @name("standard_metadata_0") standard_metadata_t standard_metadata_0_0;
    @name("x") action d_c_x() {
    }
    @name("t") table d_c_t() {
        actions = {
            d_c_x;
            NoAction;
        }
        default_action = NoAction();
    }
    action act() {
        hdr_1_0 = hdr;
        meta_1_0 = meta;
        standard_metadata_1_0 = standard_metadata;
        hdr_0_0 = hdr_1_0;
        meta_0_0 = meta_1_0;
        standard_metadata_0_0 = standard_metadata_1_0;
    }
    action act_0() {
        hdr_1_0 = hdr_0_0;
        meta_1_0 = meta_0_0;
        standard_metadata_1_0 = standard_metadata_0_0;
        hdr = hdr_1_0;
        meta = meta_1_0;
        standard_metadata = standard_metadata_1_0;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (meta_0_0.m.b == 1w1) 
            d_c_t.apply();
        tbl_act_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
