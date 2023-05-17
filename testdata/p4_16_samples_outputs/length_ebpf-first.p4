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
    state start {
        p.extract<first_header>(headers.first);
        transition select(p.length()) {
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
    apply {
        pass = true;
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
