#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct reg_data2_t {
    bit<8> reg_fld1;
}

struct metadata_t {
    bit<8>      reg_data1;
    reg_data2_t reg_data2;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<4> reg_idx_0;
    @name("ingress.reg1") register<bit<8>>(32w16) reg1_0;
    @name("ingress.reg2") register<reg_data2_t>(32w16) reg2_0;
    apply {
        reg_idx_0 = hdr.ethernet.dstAddr[3:0];
        reg1_0.read(meta.reg_data1, (bit<32>)reg_idx_0);
        meta.reg_data1 = meta.reg_data1 + 8w1;
        reg1_0.write((bit<32>)reg_idx_0, meta.reg_data1);
        reg2_0.read(meta.reg_data2, (bit<32>)reg_idx_0);
        meta.reg_data2.reg_fld1 = meta.reg_data2.reg_fld1 + 8w1;
        reg2_0.write((bit<32>)reg_idx_0, meta.reg_data2);
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control computeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

