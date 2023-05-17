#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header empty {
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
        transition accept;
    }
}

struct tuple_0 {
    bit<16> f0;
}

control EmptyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(false, (tuple_0){f0 = 16w0}, meta._cksum_result0, HashAlgorithm.csum16);
    }
}

control EmptyIngress(inout headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control EmptyEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @hidden action issue1079bmv2l47() {
        mark_to_drop(standard_metadata);
    }
    @hidden table tbl_issue1079bmv2l47 {
        actions = {
            issue1079bmv2l47();
        }
        const default_action = issue1079bmv2l47();
    }
    apply {
        tbl_issue1079bmv2l47.apply();
    }
}

control EmptyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(false, (tuple_0){f0 = 16w0}, meta._cksum_result0, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(hdr.e.isValid(), (tuple_0){f0 = 16w0}, meta._cksum_result0, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(meta._b1 == 1w0, (tuple_0){f0 = 16w0}, meta._cksum_result0, HashAlgorithm.csum16);
    }
}

control EmptyDeparser(packet_out b, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(EmptyParser(), EmptyVerifyChecksum(), EmptyIngress(), EmptyEgress(), EmptyComputeChecksum(), EmptyDeparser()) main;
