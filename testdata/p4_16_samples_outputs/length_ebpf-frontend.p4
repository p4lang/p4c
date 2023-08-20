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
    @name("prs.tmp") bit<32> tmp;
    @name("prs.tmp_0") bit<32> tmp_0;
    state start {
        p.extract<first_header>(headers.first);
        tmp_0 = p.length();
        tmp = tmp_0;
        transition select(tmp) {
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
