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
    state start {
        p.extract(headers.first);
        p.advance((bit<32>)128);
        transition parse_next;
    }
    state parse_next {
        p.extract(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        bit<8> val = 1;
        if (headers.next.value == val) {
            pass = true;
        } else {
            pass = false;
        }
    }
}

ebpfFilter(prs(), pipe()) main;
