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
    @hidden action bool_ebpf31() {
        pass = true;
    }
    @hidden table tbl_bool_ebpf31 {
        actions = {
            bool_ebpf31();
        }
        const default_action = bool_ebpf31();
    }
    apply {
        tbl_bool_ebpf31.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
