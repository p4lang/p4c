#include <core.p4>
#include <ebpf_model.p4>

struct Headers_t {
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    bool x_0;
    @name("pipe.Reject") action Reject() {
        pass = x_0;
    }
    @hidden action act() {
        x_0 = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_Reject {
        actions = {
            Reject();
        }
        const default_action = Reject();
    }
    apply {
        tbl_act.apply();
        tbl_Reject.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

