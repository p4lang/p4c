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
    headers hdr_1;
    metadata meta_1;
    standard_metadata_t standard_metadata_1;
    headers hdr_0;
    metadata meta_0;
    standard_metadata_t standard_metadata_0;
    @name("d.c.x") action d_c.x() {
    }
    @name("d.c.t") table d_c.t() {
        actions = {
            d_c.x;
            NoAction;
        }
        default_action = NoAction();
    }
    action act() {
        hdr_1 = hdr;
        meta_1 = meta;
        standard_metadata_1 = standard_metadata;
        hdr_0 = hdr_1;
        meta_0 = meta_1;
        standard_metadata_0 = standard_metadata_1;
    }
    action act_0() {
        hdr_1 = hdr_0;
        meta_1 = meta_0;
        standard_metadata_1 = standard_metadata_0;
        hdr = hdr_1;
        meta = meta_1;
        standard_metadata = standard_metadata_1;
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
        if (meta_0.m.b == 1w1) 
            d_c.t.apply();
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
