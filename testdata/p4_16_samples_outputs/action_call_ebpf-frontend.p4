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
    @name("pipe.rej") bool rej_0;
    @name("pipe.Reject") action Reject() {
        rej_0 = x_0;
        pass = rej_0;
    }
    apply {
        x_0 = true;
        Reject();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
