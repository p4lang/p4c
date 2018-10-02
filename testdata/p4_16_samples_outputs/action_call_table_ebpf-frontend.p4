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
    @name("pipe.Reject") action Reject(bit<8> rej, bit<8> bar) {
        if (rej == 8w0) 
            pass = true;
        else 
            pass = false;
        if (bar == 8w0) 
            pass = false;
    }
    @name("pipe.t") table t_0 {
        actions = {
            Reject();
        }
        default_action = Reject(8w1, 8w0);
    }
    apply {
        t_0.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

