#include <core.p4>
#include <v1model.p4>

header test_header_t {
    bit<8> value;
}

struct headers_t {
    test_header_t[2] test;
}

struct metadata_t {
}

parser TestParser(packet_in b, out headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    bit<32> test_f;
    state start {
        b.extract<test_header_t>(headers.test.next);
        test_f = headers.test.lastIndex << 1;
        transition select(test_f) {
            32w0: f;
            default: a;
        }
    }
    state a {
        transition accept;
    }
    state f {
        transition reject;
    }
}

control TestVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control TestIngress(inout headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control TestEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control TestComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control TestDeparser(packet_out b, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(TestParser(), TestVerifyChecksum(), TestIngress(), TestEgress(), TestComputeChecksum(), TestDeparser()) main;

