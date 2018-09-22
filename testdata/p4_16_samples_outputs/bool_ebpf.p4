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
    apply {
        bool x = true;
        pass = x != false;
    }
}

ebpfFilter(prs(), pipe()) main;

