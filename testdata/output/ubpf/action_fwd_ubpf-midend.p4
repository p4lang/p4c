#include <core.p4>
#include <ubpf_model.p4>

struct Headers_t {
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.hasReturned") bool hasReturned;
    @hidden action action_fwd_ubpf22() {
        std_meta.output_port = 32w2;
    }
    @hidden action action_fwd_ubpf24() {
        std_meta.output_port = 32w1;
    }
    @hidden action action_fwd_ubpf26() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action action_fwd_ubpf28() {
        std_meta.output_action = ubpf_action.REDIRECT;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_action_fwd_ubpf22 {
        actions = {
            action_fwd_ubpf22();
        }
        const default_action = action_fwd_ubpf22();
    }
    @hidden table tbl_action_fwd_ubpf24 {
        actions = {
            action_fwd_ubpf24();
        }
        const default_action = action_fwd_ubpf24();
    }
    @hidden table tbl_action_fwd_ubpf26 {
        actions = {
            action_fwd_ubpf26();
        }
        const default_action = action_fwd_ubpf26();
    }
    @hidden table tbl_action_fwd_ubpf28 {
        actions = {
            action_fwd_ubpf28();
        }
        const default_action = action_fwd_ubpf28();
    }
    apply {
        tbl_act.apply();
        if (std_meta.input_port == 32w1) {
            tbl_action_fwd_ubpf22.apply();
        } else if (std_meta.input_port == 32w2) {
            tbl_action_fwd_ubpf24.apply();
        } else {
            tbl_action_fwd_ubpf26.apply();
        }
        if (hasReturned) {
            ;
        } else {
            tbl_action_fwd_ubpf28.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
