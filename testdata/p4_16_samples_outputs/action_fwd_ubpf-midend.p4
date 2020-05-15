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
    bool hasReturned;
    @hidden action action_fwd_ubpf21() {
        std_meta.output_port = 32w2;
    }
    @hidden action action_fwd_ubpf23() {
        std_meta.output_port = 32w1;
    }
    @hidden action action_fwd_ubpf25() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action action_fwd_ubpf27() {
        std_meta.output_action = ubpf_action.REDIRECT;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_action_fwd_ubpf21 {
        actions = {
            action_fwd_ubpf21();
        }
        const default_action = action_fwd_ubpf21();
    }
    @hidden table tbl_action_fwd_ubpf23 {
        actions = {
            action_fwd_ubpf23();
        }
        const default_action = action_fwd_ubpf23();
    }
    @hidden table tbl_action_fwd_ubpf25 {
        actions = {
            action_fwd_ubpf25();
        }
        const default_action = action_fwd_ubpf25();
    }
    @hidden table tbl_action_fwd_ubpf27 {
        actions = {
            action_fwd_ubpf27();
        }
        const default_action = action_fwd_ubpf27();
    }
    apply {
        tbl_act.apply();
        if (std_meta.input_port == 32w1) {
            tbl_action_fwd_ubpf21.apply();
        } else if (std_meta.input_port == 32w2) {
            tbl_action_fwd_ubpf23.apply();
        } else {
            tbl_action_fwd_ubpf25.apply();
        }
        if (!hasReturned) {
            tbl_action_fwd_ubpf27.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

