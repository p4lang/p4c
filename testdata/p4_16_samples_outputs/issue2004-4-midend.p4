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
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
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
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr = hdr.ipv4);
        transition accept;
    }
    state parse_ethernet_second {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w8: parse_ipv4;
            default: accept;
        }
    }
    state start {
        packet.extract<ethernet_t>(hdr = hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w4 &&& 16w65532: parse_ipv4;
            16w8 &&& 16w65532: parse_ipv4;
            16w12 &&& 16w65535: parse_ipv4;
            16w0x32 &&& 16w0xfffe: parse_ethernet_second;
            16w0x34 &&& 16w0xfffc: parse_ethernet_second;
            16w0x38 &&& 16w0xfff8: parse_ethernet_second;
            16w0x40 &&& 16w0xffe0: parse_ethernet_second;
            16w0x60 &&& 16w0xfffc: parse_ethernet_second;
            16w0x64 &&& 16w0xfffe: parse_ethernet_second;
            16w0x66 &&& 16w0xffff: parse_ethernet_second;
            16w0x800: parse_ipv4;
            16w3 &&& 16w10: parse_ethernet_second;
            16w1 &&& 16w65535: parse_ipv4;
            16w2 &&& 16w65534: parse_ipv4;
            16w13 &&& 16w65535: parse_ipv4;
            16w14 &&& 16w65534: parse_ipv4;
            16w16 &&& 16w65520: parse_ipv4;
            16w32 &&& 16w65532: parse_ipv4;
            16w103 &&& 16w65535: parse_ipv4;
            16w104 &&& 16w65528: parse_ipv4;
            16w112 &&& 16w65528: parse_ipv4;
            16w120 &&& 16w65535: parse_ipv4;
            default: accept;
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

