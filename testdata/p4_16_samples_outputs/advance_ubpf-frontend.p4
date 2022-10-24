#include <core.p4>
#include <ubpf_model.p4>

header test_header {
    bit<8> bits_to_skip;
}

struct Headers_t {
    test_header skip;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<test_header>(headers.skip);
        p.advance((bit<32>)headers.skip.bits_to_skip);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    apply {
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<test_header>(headers.skip);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
