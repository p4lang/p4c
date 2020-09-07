#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct ingress_metadata_t {
    bit<12> vrf;
    bit<16> bd;
    bit<16> nexthop_index;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct metadata {
    bit<12> _ingress_metadata_vrf0;
    bit<16> _ingress_metadata_bd1;
    bit<16> _ingress_metadata_nexthop_index2;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr = hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w4: accept;
            16w6: accept;
            16w10: accept;
            16w20 &&& 16w65532: accept;
            16w24 &&& 16w65532: accept;
            16w28 &&& 16w65534: accept;
            16w30 &&& 16w65535: accept;
            16w40 &&& 16w65528: accept;
            16w48 &&& 16w65520: accept;
            16w64 &&& 16w65472: accept;
            16w128 &&& 16w65408: accept;
            16w256 &&& 16w65280: accept;
            16w512 &&& 16w65280: accept;
            16w768 &&& 16w65408: accept;
            16w896 &&& 16w65472: accept;
            16w960 &&& 16w65504: accept;
            16w992 &&& 16w65528: accept;
            16w1000 &&& 16w65535: accept;
            16w31: accept;
            default: reject;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(p = ParserImpl(), ig = ingress(), vr = verifyChecksum(), eg = egress(), ck = computeChecksum(), dep = DeparserImpl()) main;

