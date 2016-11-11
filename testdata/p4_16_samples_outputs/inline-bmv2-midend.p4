#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;
header ipv4_t {
}

struct H {
    ipv4_t ipv4;
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start {
        transition accept;
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

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    M tmp_1;
    H tmp_2;
    M meta_1;
    H hdr_1;
    @name("NoAction_1") action NoAction() {
    }
    @name("do_aux.adjust_lkp_fields") table do_aux_adjust_lkp_fields_0() {
        key = {
            hdr_1.ipv4.isValid(): exact;
        }
        actions = {
            NoAction();
        }
    }
    action act() {
        tmp_2 = hdr;
        meta_1 = tmp_1;
        hdr_1 = tmp_2;
    }
    action act_0() {
        tmp_1 = meta_1;
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
        do_aux_adjust_lkp_fields_0.apply();
        tbl_act_0.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
    }
}

control DeparserI(packet_out b, in H hdr) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
