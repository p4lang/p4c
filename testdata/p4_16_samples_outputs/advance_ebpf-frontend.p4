#include <core.p4>
#include <ebpf_model.p4>

header test_header {
    bit<8> bits_to_skip;
}

header next_header {
    bit<8> test_value;
}

struct Headers_t {
    test_header skip;
    next_header next;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<test_header>(headers.skip);
        p.advance((bit<32>)headers.skip.bits_to_skip);
        p.extract<next_header>(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    @name("pipe.val") bit<8> val_0;
    apply {
        val_0 = 8w1;
        if (headers.next.test_value == val_0) {
            pass = true;
        } else {
            pass = false;
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

