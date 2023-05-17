#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header h0_t {
    bit<8> f0;
}

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

header h3_t {
    bit<8> f3;
}

header h4_t {
    bit<8> f4;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    h0_t       h0;
    h1_t       h1;
    h2_t       h2;
    h3_t       h3;
    h4_t       h4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800 .. 16w0x806: parse_h0;
            16w0x808: parse_h1;
            16w0xfff1 .. 16w0xfffe: parse_h2;
            16w0x900: parse_h3;
            16w0x8ff .. 16w0x901: parse_h4;
            default: accept;
        }
    }
    state parse_h0 {
        packet.extract<h0_t>(hdr.h0);
        transition accept;
    }
    state parse_h1 {
        packet.extract<h1_t>(hdr.h1);
        transition accept;
    }
    state parse_h2 {
        packet.extract<h2_t>(hdr.h2);
        transition accept;
    }
    state parse_h3 {
        packet.extract<h3_t>(hdr.h3);
        transition accept;
    }
    state parse_h4 {
        packet.extract<h4_t>(hdr.h4);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.tmp") bit<1> tmp;
    @name("ingress.tmp_0") bit<1> tmp_0;
    @name("ingress.tmp_1") bit<1> tmp_1;
    @name("ingress.tmp_2") bit<1> tmp_2;
    @name("ingress.tmp_3") bit<1> tmp_3;
    apply {
        if (hdr.h4.isValid()) {
            tmp = 1w1;
        } else {
            tmp = 1w0;
        }
        hdr.ethernet.dstAddr[44:44] = tmp;
        if (hdr.h3.isValid()) {
            tmp_0 = 1w1;
        } else {
            tmp_0 = 1w0;
        }
        hdr.ethernet.dstAddr[43:43] = tmp_0;
        if (hdr.h2.isValid()) {
            tmp_1 = 1w1;
        } else {
            tmp_1 = 1w0;
        }
        hdr.ethernet.dstAddr[42:42] = tmp_1;
        if (hdr.h1.isValid()) {
            tmp_2 = 1w1;
        } else {
            tmp_2 = 1w0;
        }
        hdr.ethernet.dstAddr[41:41] = tmp_2;
        if (hdr.h0.isValid()) {
            tmp_3 = 1w1;
        } else {
            tmp_3 = 1w0;
        }
        hdr.ethernet.dstAddr[40:40] = tmp_3;
        standard_metadata.egress_spec = 9w3;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<h0_t>(hdr.h0);
        packet.emit<h1_t>(hdr.h1);
        packet.emit<h2_t>(hdr.h2);
        packet.emit<h3_t>(hdr.h3);
        packet.emit<h4_t>(hdr.h4);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
