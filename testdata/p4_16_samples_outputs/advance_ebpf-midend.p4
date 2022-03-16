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
    @hidden action advance_ebpf34() {
        pass = true;
    }
    @hidden action advance_ebpf36() {
        pass = false;
    }
    @hidden table tbl_advance_ebpf34 {
        actions = {
            advance_ebpf34();
        }
        const default_action = advance_ebpf34();
    }
    @hidden table tbl_advance_ebpf36 {
        actions = {
            advance_ebpf36();
        }
        const default_action = advance_ebpf36();
    }
    apply {
        if (headers.next.test_value == 8w1) {
            tbl_advance_ebpf34.apply();
        } else {
            tbl_advance_ebpf36.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

