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
    @name("pipe.Reject") action Reject() {
        pass = true;
    }
    @hidden table tbl_Reject {
        actions = {
            Reject();
        }
        const default_action = Reject();
    }
    apply {
        tbl_Reject.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
