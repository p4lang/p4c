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
    @hidden action issue1127bmv2l53() {
        hdr_0_h1.out1 = 8w4;
    }
    @hidden action act() {
        hdr_0_h1 = hdr.h1;
    }
    @hidden action issue1127bmv2l53_0() {
        hdr_0_h1.out1 = 8w4;
    }
    @hidden action act_0() {
        hdr.h1 = hdr_0_h1;
    }
    @hidden action act_1() {
        hdr.h1 = hdr_0_h1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_issue1127bmv2l53 {
        actions = {
            issue1127bmv2l53();
        }
        const default_action = issue1127bmv2l53();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue1127bmv2l53_0 {
        actions = {
            issue1127bmv2l53_0();
        }
        const default_action = issue1127bmv2l53_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        tbl_act.apply();
        if (hdr.h1.op1 == 8w0x0) {
            ;
        } else if (hdr.h1.op1[7:4] == 4w1) {
            tbl_issue1127bmv2l53.apply();
        }
        tbl_act_0.apply();
        if (hdr.h1.op2 == 8w0x0) {
            ;
        } else if (hdr.h1.op2[7:4] == 4w1) {
            tbl_issue1127bmv2l53_0.apply();
        }
        tbl_act_1.apply();
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

