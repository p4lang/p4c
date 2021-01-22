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
    @hidden action bool_ebpf32() {
        pass = true;
    }
    @hidden table tbl_bool_ebpf32 {
        actions = {
            bool_ebpf32();
        }
        const default_action = bool_ebpf32();
    }
    apply {
        tbl_bool_ebpf32.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

