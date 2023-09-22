#include <ebpf_model.p4>
#include <core.p4>

header ThreeBit_h {
    bit<3> x;
}

struct Headers_t {
    ThreeBit_h foo;
    ThreeBit_h bar;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.foo);
        p.extract(headers.bar);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {

    apply {
        pass = true;
    }
}

ebpfFilter(prs(), pipe()) main;
