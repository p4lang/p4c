#include <core.p4>
#include <ebpf_model.p4>

header X {
    bit<1>  dc;
    bit<3>  cpd;
    bit<2>  snpadding;
    bit<16> sn;
    bit<2>  e;
}

struct Headers_t {
    X x;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<X>(headers.x);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        pass = true;
        if (headers.x.isValid()) {
            ;
        } else {
            pass = false;
            return;
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

