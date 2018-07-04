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
    action Reject(bit<8> rej, bit<8> bar) {
        if (rej == 0) {
            pass = true;
        }
        else {
            pass = false;
        }
        if (bar == 0) {
            pass = false;
        }
    }
    table t {
        actions = {
            Reject();
        }
        default_action = Reject(1, 0);
    }
    apply {
        bool x = true;
        t.apply();
    }
}

ebpfFilter(prs(), pipe()) main;

