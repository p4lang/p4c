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
    @name("pipe.x") bool x_0;
    @name("pipe.Reject") action Reject() {
        pass = x_0;
    }
    @hidden action action_call_ebpf34() {
        x_0 = true;
    }
    @hidden table tbl_action_call_ebpf34 {
        actions = {
            action_call_ebpf34();
        }
        const default_action = action_call_ebpf34();
    }
    @hidden table tbl_Reject {
        actions = {
            Reject();
        }
        const default_action = Reject();
    }
    apply {
        tbl_action_call_ebpf34.apply();
        tbl_Reject.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

