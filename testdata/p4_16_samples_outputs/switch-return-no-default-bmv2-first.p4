#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control IngressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
        sm.egress_spec = 9w1;
        if (hdr.ethernet.dst == 48w0x1122334455) {
            switch (hdr.ethernet.etherType) {
                16w0x800: {
                    return;
                }
            }
        }
        mark_to_drop(sm);
    }
}

control EgressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
