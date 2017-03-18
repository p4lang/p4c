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
    h tmp_m;
    standard_metadata_t tmp_0;
    h d_tmp_m;
    standard_metadata_t d_tmp_2;
    @name("NoAction") action NoAction_0() {
    }
    @name("d.c.x") action d_c_x() {
    }
    @name("d.c.t") table d_c_t_0 {
        actions = {
            d_c_x();
            @default_only NoAction_0();
        }
        default_action = NoAction_0();
    }
    action act() {
        tmp_m.b = meta.m.b;
        tmp_0.ingress_port = standard_metadata.ingress_port;
        tmp_0.egress_spec = standard_metadata.egress_spec;
        tmp_0.egress_port = standard_metadata.egress_port;
        tmp_0.clone_spec = standard_metadata.clone_spec;
        tmp_0.instance_type = standard_metadata.instance_type;
        tmp_0.drop = standard_metadata.drop;
        tmp_0.recirculate_port = standard_metadata.recirculate_port;
        tmp_0.packet_length = standard_metadata.packet_length;
        d_tmp_m.b = tmp_m.b;
        d_tmp_2.ingress_port = tmp_0.ingress_port;
        d_tmp_2.egress_spec = tmp_0.egress_spec;
        d_tmp_2.egress_port = tmp_0.egress_port;
        d_tmp_2.clone_spec = tmp_0.clone_spec;
        d_tmp_2.instance_type = tmp_0.instance_type;
        d_tmp_2.drop = tmp_0.drop;
        d_tmp_2.recirculate_port = tmp_0.recirculate_port;
        d_tmp_2.packet_length = tmp_0.packet_length;
    }
    action act_0() {
        tmp_m.b = d_tmp_m.b;
        tmp_0.ingress_port = d_tmp_2.ingress_port;
        tmp_0.egress_spec = d_tmp_2.egress_spec;
        tmp_0.egress_port = d_tmp_2.egress_port;
        tmp_0.clone_spec = d_tmp_2.clone_spec;
        tmp_0.instance_type = d_tmp_2.instance_type;
        tmp_0.drop = d_tmp_2.drop;
        tmp_0.recirculate_port = d_tmp_2.recirculate_port;
        tmp_0.packet_length = d_tmp_2.packet_length;
        meta.m.b = tmp_m.b;
        standard_metadata.ingress_port = tmp_0.ingress_port;
        standard_metadata.egress_spec = tmp_0.egress_spec;
        standard_metadata.egress_port = tmp_0.egress_port;
        standard_metadata.clone_spec = tmp_0.clone_spec;
        standard_metadata.instance_type = tmp_0.instance_type;
        standard_metadata.drop = tmp_0.drop;
        standard_metadata.recirculate_port = tmp_0.recirculate_port;
        standard_metadata.packet_length = tmp_0.packet_length;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (d_tmp_m.b == 1w1)
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
