#include <core.p4>
#define UBPF_MODEL_VERSION 20200515
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
        p.extract(headers.skip);
        p.advance((bit<32>) headers.skip.bits_to_skip);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    apply { }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.skip);
    }
}

ubpf(prs(), pipe(), dprs()) main;
