#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct my_headers_t {
    ethernet_t ethernet;
}

typedef my_headers_t headers_t;
struct local_metadata_t {
    bit<16> f16;
    bit<16> m16;
    bit<16> d16;
    int<16> x16;
    int<16> a16;
    int<16> b16;
}

parser parser_impl(packet_in packet, out headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control ingress_impl(inout headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control egress_impl(inout headers_t hdr, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @hidden action strength4l44() {
        local_metadata.m16 = local_metadata.f16[14:0] ++ 1w0;
        local_metadata.d16 = 1w0 ++ local_metadata.f16[15:1];
        local_metadata.a16 = (int<16>)(16s0 ++ local_metadata.x16 << 1)[15:0];
        local_metadata.b16 = (int<16>)(16s0 ++ local_metadata.x16 >> 1)[15:0];
    }
    @hidden table tbl_strength4l44 {
        actions = {
            strength4l44();
        }
        const default_action = strength4l44();
    }
    apply {
        tbl_strength4l44.apply();
    }
}

control verify_checksum_impl(inout headers_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_checksum_impl(inout headers_t hdr, inout local_metadata_t local_metadata) {
    apply {
    }
}

control deparser_impl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

V1Switch<my_headers_t, local_metadata_t>(parser_impl(), verify_checksum_impl(), ingress_impl(), egress_impl(), compute_checksum_impl(), deparser_impl()) main;

