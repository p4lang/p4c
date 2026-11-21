error {
    InvalidHeaderValue
}
#include <core.p4>
#include <ebpf_model.p4>

header test_header {
    bit<8> value;
}

struct Headers_t {
    test_header first;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<test_header>(headers.first);
        verify(headers.first.value == 8w1, error.InvalidHeaderValue);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        pass = true;
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;
