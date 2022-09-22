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
    bool hasExited;
    @name("pipe.RejectConditional") action RejectConditional(@name("condition") bit<1> condition) {
    }
    @name("pipe.act_return") action act_return() {
        mark_to_pass();
    }
    @name("pipe.act_exit") action act_exit() {
        mark_to_pass();
        hasExited = true;
    }
    @name("pipe.tbl_a") table tbl_a_0 {
        actions = {
            RejectConditional();
            act_return();
            act_exit();
        }
        default_action = RejectConditional(1w0);
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        tbl_a_0.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

