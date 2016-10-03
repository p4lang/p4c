#include <core.p4>
#include <v1model.p4>

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
    @name("hdr_1") headers hdr_2;
    @name("meta_1") metadata meta_2;
    @name("standard_metadata_1") standard_metadata_t standard_metadata_2;
    @name("hdr_0") headers hdr_3;
    @name("meta_0") metadata meta_3;
    @name("standard_metadata_0") standard_metadata_t standard_metadata_3;
    @name("NoAction_1") action NoAction() {
    }
    @name("d.c.x") action d_c_x() {
    }
    @name("d.c.t") table d_c_t_0() {
        actions = {
            d_c_x();
            NoAction();
        }
        default_action = NoAction();
    }
    action act() {
        hdr_2 = hdr;
        meta_2 = meta;
        standard_metadata_2 = standard_metadata;
        hdr_3 = hdr_2;
        meta_3 = meta_2;
        standard_metadata_3 = standard_metadata_2;
    }
    action act_0() {
        hdr_2 = hdr_3;
        meta_2 = meta_3;
        standard_metadata_2 = standard_metadata_3;
        hdr = hdr_2;
        meta = meta_2;
        standard_metadata = standard_metadata_2;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (meta_3.m.b == 1w1) 
            d_c_t_0.apply();
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

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
