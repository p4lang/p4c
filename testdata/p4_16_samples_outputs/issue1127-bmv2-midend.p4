#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h1_t {
    bit<8> op1;
    bit<8> op2;
    bit<8> out1;
}

struct headers {
    h1_t h1;
}

struct metadata {
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<h1_t>(hdr.h1);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    h1_t hdr_0_h1;
    @hidden action issue1127bmv2l44() {
        hdr_0_h1.out1 = 8w4;
    }
    @hidden action issue1127bmv2l55() {
        hdr_0_h1 = hdr.h1;
    }
    @hidden action issue1127bmv2l44_0() {
        hdr_0_h1.out1 = 8w4;
    }
    @hidden action issue1127bmv2l55_0() {
        hdr.h1 = hdr_0_h1;
    }
    @hidden action issue1127bmv2l57() {
        hdr.h1 = hdr_0_h1;
    }
    @hidden table tbl_issue1127bmv2l55 {
        actions = {
            issue1127bmv2l55();
        }
        const default_action = issue1127bmv2l55();
    }
    @hidden table tbl_issue1127bmv2l44 {
        actions = {
            issue1127bmv2l44();
        }
        const default_action = issue1127bmv2l44();
    }
    @hidden table tbl_issue1127bmv2l55_0 {
        actions = {
            issue1127bmv2l55_0();
        }
        const default_action = issue1127bmv2l55_0();
    }
    @hidden table tbl_issue1127bmv2l44_0 {
        actions = {
            issue1127bmv2l44_0();
        }
        const default_action = issue1127bmv2l44_0();
    }
    @hidden table tbl_issue1127bmv2l57 {
        actions = {
            issue1127bmv2l57();
        }
        const default_action = issue1127bmv2l57();
    }
    apply {
        tbl_issue1127bmv2l55.apply();
        if (hdr.h1.op1 == 8w0x0) {
            ;
        } else if (hdr.h1.op1[7:4] == 4w1) {
            tbl_issue1127bmv2l44.apply();
        }
        tbl_issue1127bmv2l55_0.apply();
        if (hdr.h1.op2 == 8w0x0) {
            ;
        } else if (hdr.h1.op2[7:4] == 4w1) {
            tbl_issue1127bmv2l44_0.apply();
        }
        tbl_issue1127bmv2l57.apply();
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<h1_t>(hdr.h1);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
