#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header empty {
    bit<8> j;
}

struct headers_t {
    empty e;
}

struct cksum_t {
    bit<16> result;
}

struct metadata_t {
    bit<16> _cksum_result0;
    bit<1>  _b1;
}

parser EmptyParser(packet_in b, out headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        b.extract<empty>(headers.e);
        verify(headers.e.j == 8w10 || headers.e.j == 8w18 || headers.e.j == 8w26 || headers.e.j == 8w34, error.NoError);
        transition accept;
    }
}

control EmptyVerify(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control EmptyIngress(inout headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control EmptyEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @hidden action verify_disjunction54() {
        mark_to_drop(standard_metadata);
    }
    @hidden table tbl_verify_disjunction54 {
        actions = {
            verify_disjunction54();
        }
        const default_action = verify_disjunction54();
    }
    apply {
        tbl_verify_disjunction54.apply();
    }
}

control EmptyCompute(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control EmptyDeparser(packet_out b, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(EmptyParser(), EmptyVerify(), EmptyIngress(), EmptyEgress(), EmptyCompute(), EmptyDeparser()) main;
