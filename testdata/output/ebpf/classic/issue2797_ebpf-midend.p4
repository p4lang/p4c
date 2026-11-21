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
    @hidden action issue2797_ebpf33() {
        pass = false;
    }
    @hidden action issue2797_ebpf31() {
        pass = true;
    }
    @hidden table tbl_issue2797_ebpf31 {
        actions = {
            issue2797_ebpf31();
        }
        const default_action = issue2797_ebpf31();
    }
    @hidden table tbl_issue2797_ebpf33 {
        actions = {
            issue2797_ebpf33();
        }
        const default_action = issue2797_ebpf33();
    }
    apply {
        tbl_issue2797_ebpf31.apply();
        if (headers.x.isValid()) {
            ;
        } else {
            tbl_issue2797_ebpf33.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
