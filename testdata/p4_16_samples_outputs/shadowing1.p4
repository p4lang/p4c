#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t eth;
}

struct metadata_t {
}

const bit<8> i = 5;
bit<8> foofunc(in bit<8> j, out bit<8> out1, out bit<8> out2) {
    bit<8> tmp = j;
    bit<8> i = i + 1;
    {
        out1 = tmp;
        bit<8> i = i + 2;
        out2 = i;
    }
    return i;
}
parser fooparser(inout bit<8> i, out bit<8> out1, out bit<8> out2, out bit<8> out3, out bit<8> out4) {
    bit<8> tmp = i;
    bit<8> i = i + 1;
    state start {
        out1 = tmp;
        out2 = i;
        {
            bit<8> i = i + 2;
            out3 = i;
        }
        out4 = i;
        transition accept;
    }
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<8> i;
    bit<8> out1;
    bit<8> out2;
    bit<8> out3;
    bit<8> out4;
    state start {
        packet.extract(hdr.eth);
        i = hdr.eth.srcAddr[15:8];
        fooparser.apply(i, out1, out2, out3, out4);
        log_msg("i={} out1={} out2={} out3={} out4={}", { i, out1, out2, out3, out4 });
        transition accept;
    }
}

control fooctrl(inout bit<8> i, out bit<8> out1, out bit<8> out2, out bit<8> out3, out bit<8> out4) {
    bit<8> tmp = i;
    bit<8> i = i + 1;
    apply {
        out1 = tmp;
        out2 = i;
        {
            bit<8> i = i + 2;
            out3 = i;
        }
        out4 = i;
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<8> i;
    bit<8> out1;
    bit<8> out2;
    bit<8> out3;
    bit<8> out4;
    apply {
        i = hdr.eth.srcAddr[7:0];
        fooctrl.apply(i, out1, out2, out3, out4);
        log_msg("i={} out1={} out2={} out3={} out4={}", { i, out1, out2, out3, out4 });
        out3 = foofunc(i, out1, out2);
        log_msg("i={} out1={} out2={} out3={}", { i, out1, out2, out3 });
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
