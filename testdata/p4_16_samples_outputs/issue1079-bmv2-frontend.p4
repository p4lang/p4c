#include <core.p4>
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
    cksum_t cksum;
    bit<1>  b;
}

parser EmptyParser(packet_in b, out headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control EmptyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        verify_checksum<tuple<bit<16>>, bit<16>>(false, { 16w0 }, meta.cksum.result, HashAlgorithm.csum16);
    }
}

control EmptyIngress(inout headers_t headers, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control EmptyEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        mark_to_drop();
    }
}

control EmptyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum<tuple<bit<16>>, bit<16>>(false, { 16w0 }, meta.cksum.result, HashAlgorithm.csum16);
        update_checksum<tuple<bit<16>>, bit<16>>(hdr.e.isValid(), { 16w0 }, meta.cksum.result, HashAlgorithm.csum16);
        update_checksum<tuple<bit<16>>, bit<16>>(meta.b == 1w0, { 16w0 }, meta.cksum.result, HashAlgorithm.csum16);
    }
}

control EmptyDeparser(packet_out b, in headers_t hdr) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(EmptyParser(), EmptyVerifyChecksum(), EmptyIngress(), EmptyEgress(), EmptyComputeChecksum(), EmptyDeparser()) main;

