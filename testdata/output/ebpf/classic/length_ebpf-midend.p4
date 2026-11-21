#include <core.p4>
#include <ebpf_model.p4>

header first_header {
    bit<8> value;
}

header next_header {
    bit<32> value;
}

struct Headers_t {
    first_header first;
    next_header  next;
}

parser prs(packet_in p, out Headers_t headers) {
    @name("prs.tmp_0") bit<32> tmp_0;
    state start {
        p.extract<first_header>(headers.first);
        tmp_0 = p.length();
        transition select(tmp_0) {
            32w16: parse_next;
            default: reject;
        }
    }
    state parse_next {
        p.extract<next_header>(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    @hidden action length_ebpf34() {
        pass = true;
    }
    @hidden table tbl_length_ebpf34 {
        actions = {
            length_ebpf34();
        }
        const default_action = length_ebpf34();
    }
    apply {
        tbl_length_ebpf34.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
