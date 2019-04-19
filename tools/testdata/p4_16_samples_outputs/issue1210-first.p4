#include <core.p4>
#include <v1model.p4>

struct PortId_t {
    bit<9> _v;
}

const PortId_t PSA_CPU_PORT = { 9w192 };
struct parsed_headers_t {
}

struct metadata_t {
    PortId_t foo;
    PortId_t bar;
}

parser ParserImpl(packet_in packet, out parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
        if (meta.foo == meta.bar) 
            meta.foo._v = meta.foo._v + 9w1;
        if (meta.foo == { 9w192 }) 
            meta.foo._v = meta.foo._v + 9w1;
    }
}

control EgressImpl(inout parsed_headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in parsed_headers_t hdr) {
    apply {
    }
}

control VerifyChecksumImpl(inout parsed_headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ComputeChecksumImpl(inout parsed_headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<parsed_headers_t, metadata_t>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;

