#include <core.p4>
#include <ebpf_model.p4>

header test_header {
    bit<8> value;
}

header next_header {
    bit<8> value;
}

struct Headers_t {
    test_header first;
    next_header next;
}

parser prs(packet_in p, out Headers_t headers) {
    value_set<bit<8>>(4) pvs;
    state start {
        p.extract(headers.first);
        transition select(headers.first.value) {
            pvs: parse_next;
            default: accept;
        }
    }
    state parse_next {
        p.extract(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        if (headers.next.isValid()) {
            pass = true;
        } else {
            pass = false;
        }
    }
}

ebpfFilter(prs(), pipe()) main;
