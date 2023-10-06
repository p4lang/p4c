#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header h0_t {
}

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s0_t {
}

struct s1_t {
    bit<8> f1;
}

struct s2_t {
    bit<8> f1;
    bit<8> f2;
}

header hstructs_t {
    bit<8> _s1_f10;
    bit<8> _s2_f11;
    bit<8> _s2_f22;
}

struct headers_t {
    ethernet_t ethernet;
    h0_t       h0;
    h1_t       h1;
    h2_t       h2;
    hstructs_t hstructs;
}

struct metadata_t {
}

parser parserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @hidden action structurevaluedexprok1bmv2l103() {
        hdr.h1.setValid();
        hdr.h1.f1 = 8w42;
        hdr.h2.setValid();
        hdr.h2.f1 = 8w43;
        hdr.h2.f2 = 8w44;
        hdr.hstructs._s1_f10 = 8w5;
        hdr.hstructs._s2_f11 = 8w5;
        hdr.hstructs._s2_f22 = 8w10;
    }
    @hidden action structurevaluedexprok1bmv2l108() {
        hdr.h1.setValid();
        hdr.h1.f1 = 8w52;
        hdr.h2.setValid();
        hdr.h2.f1 = 8w54;
        hdr.h2.f2 = 8w53;
        hdr.hstructs._s1_f10 = 8w6;
        hdr.hstructs._s2_f11 = 8w8;
        hdr.hstructs._s2_f22 = 8w11;
    }
    @hidden action structurevaluedexprok1bmv2l95() {
        hdr.hstructs.setValid();
        hdr.h0.setValid();
    }
    @hidden action structurevaluedexprok1bmv2l116() {
        hdr.ethernet.dstAddr = (bit<48>)(bit<1>)hdr.h0.isValid();
    }
    @hidden table tbl_structurevaluedexprok1bmv2l95 {
        actions = {
            structurevaluedexprok1bmv2l95();
        }
        const default_action = structurevaluedexprok1bmv2l95();
    }
    @hidden table tbl_structurevaluedexprok1bmv2l103 {
        actions = {
            structurevaluedexprok1bmv2l103();
        }
        const default_action = structurevaluedexprok1bmv2l103();
    }
    @hidden table tbl_structurevaluedexprok1bmv2l108 {
        actions = {
            structurevaluedexprok1bmv2l108();
        }
        const default_action = structurevaluedexprok1bmv2l108();
    }
    @hidden table tbl_structurevaluedexprok1bmv2l116 {
        actions = {
            structurevaluedexprok1bmv2l116();
        }
        const default_action = structurevaluedexprok1bmv2l116();
    }
    apply {
        if (hdr.ethernet.isValid()) {
            tbl_structurevaluedexprok1bmv2l95.apply();
            if (hdr.ethernet.etherType == 16w0) {
                tbl_structurevaluedexprok1bmv2l103.apply();
            } else {
                tbl_structurevaluedexprok1bmv2l108.apply();
            }
            tbl_structurevaluedexprok1bmv2l116.apply();
        }
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<h0_t>(hdr.h0);
        pkt.emit<h1_t>(hdr.h1);
        pkt.emit<h2_t>(hdr.h2);
        pkt.emit<hstructs_t>(hdr.hstructs);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
