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
    headers hdr_2;
    h meta_2_m;
    standard_metadata_t standard_metadata_2;
    headers hdr_3;
    h meta_3_m;
    standard_metadata_t standard_metadata_3;
    @name("NoAction_1") action NoAction_0() {
    }
    @name("d.c.x") action d_c_x() {
    }
    @name("d.c.t") table d_c_t_0() {
        actions = {
            d_c_x();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    action act() {
        meta_2_m.b = meta.m.b;
        standard_metadata_2.ingress_port = standard_metadata.ingress_port;
        standard_metadata_2.egress_spec = standard_metadata.egress_spec;
        standard_metadata_2.egress_port = standard_metadata.egress_port;
        standard_metadata_2.clone_spec = standard_metadata.clone_spec;
        standard_metadata_2.instance_type = standard_metadata.instance_type;
        standard_metadata_2.drop = standard_metadata.drop;
        standard_metadata_2.recirculate_port = standard_metadata.recirculate_port;
        standard_metadata_2.packet_length = standard_metadata.packet_length;
        meta_3_m.b = meta_2_m.b;
        standard_metadata_3.ingress_port = standard_metadata_2.ingress_port;
        standard_metadata_3.egress_spec = standard_metadata_2.egress_spec;
        standard_metadata_3.egress_port = standard_metadata_2.egress_port;
        standard_metadata_3.clone_spec = standard_metadata_2.clone_spec;
        standard_metadata_3.instance_type = standard_metadata_2.instance_type;
        standard_metadata_3.drop = standard_metadata_2.drop;
        standard_metadata_3.recirculate_port = standard_metadata_2.recirculate_port;
        standard_metadata_3.packet_length = standard_metadata_2.packet_length;
    }
    action act_0() {
        meta_2_m.b = meta_3_m.b;
        standard_metadata_2.ingress_port = standard_metadata_3.ingress_port;
        standard_metadata_2.egress_spec = standard_metadata_3.egress_spec;
        standard_metadata_2.egress_port = standard_metadata_3.egress_port;
        standard_metadata_2.clone_spec = standard_metadata_3.clone_spec;
        standard_metadata_2.instance_type = standard_metadata_3.instance_type;
        standard_metadata_2.drop = standard_metadata_3.drop;
        standard_metadata_2.recirculate_port = standard_metadata_3.recirculate_port;
        standard_metadata_2.packet_length = standard_metadata_3.packet_length;
        meta.m.b = meta_2_m.b;
        standard_metadata.ingress_port = standard_metadata_2.ingress_port;
        standard_metadata.egress_spec = standard_metadata_2.egress_spec;
        standard_metadata.egress_port = standard_metadata_2.egress_port;
        standard_metadata.clone_spec = standard_metadata_2.clone_spec;
        standard_metadata.instance_type = standard_metadata_2.instance_type;
        standard_metadata.drop = standard_metadata_2.drop;
        standard_metadata.recirculate_port = standard_metadata_2.recirculate_port;
        standard_metadata.packet_length = standard_metadata_2.packet_length;
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
        if (meta_3_m.b == 1w1) 
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
