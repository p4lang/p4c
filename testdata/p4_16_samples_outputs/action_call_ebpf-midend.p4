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
    bool x;
    @name("pipe.Reject") action Reject_0() {
        pass = x;
    }
    @hidden action act() {
        x = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_Reject {
        actions = {
            Reject_0();
        }
        const default_action = Reject_0();
    }
    apply {
        tbl_act.apply();
        tbl_Reject.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

