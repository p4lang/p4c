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
    @hidden action issue2797_ebpf27() {
        pass = false;
    }
    @hidden action issue2797_ebpf25() {
        pass = true;
    }
    @hidden table tbl_issue2797_ebpf25 {
        actions = {
            issue2797_ebpf25();
        }
        const default_action = issue2797_ebpf25();
    }
    @hidden table tbl_issue2797_ebpf27 {
        actions = {
            issue2797_ebpf27();
        }
        const default_action = issue2797_ebpf27();
    }
    apply {
        tbl_issue2797_ebpf25.apply();
        if (headers.x.isValid()) {
            ;
        } else {
            tbl_issue2797_ebpf27.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
