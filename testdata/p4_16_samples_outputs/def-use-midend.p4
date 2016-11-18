#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
struct H {
}

struct M {
}

parser ParserI(packet_in b, out H h, inout M m, inout std_meta_t s) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    H tmp_2;
    M tmp_3;
    standard_metadata_t tmp_4;
    H hdr_1;
    M meta_1;
    std_meta_t std_meta_1;
    bool hasReturned_0;
    @name("a") action a_0() {
    }
    @name("t") table t() {
        key = {
        }
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    action act() {
        hasReturned_0 = true;
    }
    action act_0() {
        tmp_4.ingress_port = std_meta.ingress_port;
        tmp_4.egress_spec = std_meta.egress_spec;
        tmp_4.egress_port = std_meta.egress_port;
        tmp_4.clone_spec = std_meta.clone_spec;
        tmp_4.instance_type = std_meta.instance_type;
        tmp_4.drop = std_meta.drop;
        tmp_4.recirculate_port = std_meta.recirculate_port;
        tmp_4.packet_length = std_meta.packet_length;
        std_meta_1.ingress_port = tmp_4.ingress_port;
        std_meta_1.egress_spec = tmp_4.egress_spec;
        std_meta_1.egress_port = tmp_4.egress_port;
        std_meta_1.clone_spec = tmp_4.clone_spec;
        std_meta_1.instance_type = tmp_4.instance_type;
        std_meta_1.drop = tmp_4.drop;
        std_meta_1.recirculate_port = tmp_4.recirculate_port;
        std_meta_1.packet_length = tmp_4.packet_length;
        tmp_4.ingress_port = std_meta_1.ingress_port;
        tmp_4.egress_spec = std_meta_1.egress_spec;
        tmp_4.egress_port = std_meta_1.egress_port;
        tmp_4.clone_spec = std_meta_1.clone_spec;
        tmp_4.instance_type = std_meta_1.instance_type;
        tmp_4.drop = std_meta_1.drop;
        tmp_4.recirculate_port = std_meta_1.recirculate_port;
        tmp_4.packet_length = std_meta_1.packet_length;
        std_meta.ingress_port = tmp_4.ingress_port;
        std_meta.egress_spec = tmp_4.egress_spec;
        std_meta.egress_port = tmp_4.egress_port;
        std_meta.clone_spec = tmp_4.clone_spec;
        std_meta.instance_type = tmp_4.instance_type;
        std_meta.drop = tmp_4.drop;
        std_meta.recirculate_port = tmp_4.recirculate_port;
        std_meta.packet_length = tmp_4.packet_length;
    }
    action act_1() {
        hasReturned_0 = false;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        switch (t.apply().action_run) {
            a_0: {
                tbl_act_0.apply();
            }
            default: {
                tbl_act_1.apply();
            }
        }

    }
}

control VerifyChecksumI(in H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
