#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> mac_addr_t;
header aggregator_t {
    bit<8> base0;
    bit<8> base1;
    bit<8> base2;
    bit<8> base3;
    bit<8> val;
}

header vec_e_t {
    bit<8> e;
}

header ml_hdr_t {
    int<8> idx;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

struct headers {
    ethernet_t      ethernet;
    ml_hdr_t        ml;
    vec_e_t[3]      vector;
    aggregator_t[3] pool;
}

struct metadata_t {
    int<8> counter;
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<ml_hdr_t>(hdr.ml);
        packet.extract<vec_e_t>(hdr.vector[0]);
        packet.extract<vec_e_t>(hdr.vector[1]);
        packet.extract<vec_e_t>(hdr.vector[2]);
        packet.extract<aggregator_t>(hdr.pool[0]);
        packet.extract<aggregator_t>(hdr.pool[1]);
        packet.extract<aggregator_t>(hdr.pool[2]);
        meta.counter = 8s0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    int<8> hsiVar0_0;
    aggregator_t hsVar1;
    int<8> hsiVar2;
    aggregator_t hsVar3;
    int<8> hsiVar4;
    aggregator_t hsVar5;
    int<8> hsiVar6;
    aggregator_t hsVar7;
    @hidden action issue2726bmv2l87() {
        hdr.pool[0].val = hdr.pool[1].val + 8w1;
    }
    @hidden action issue2726bmv2l87_0() {
        hdr.pool[1].val = hdr.pool[1].val + 8w1;
    }
    @hidden action issue2726bmv2l87_1() {
        hdr.pool[2].val = hdr.pool[1].val + 8w1;
    }
    @hidden action issue2726bmv2l87_2() {
        hdr.pool[2].val = hdr.pool[1].val + 8w1;
    }
    @hidden action issue2726bmv2l87_3() {
        hdr.pool[8s2] = hsVar1;
    }
    @hidden action issue2726bmv2l81() {
        meta.counter = meta.counter + 8s1;
        hdr.vector[0].e = hdr.pool[1].val + 8w1;
        hsiVar0_0 = (int<8>)hdr.ml.idx;
    }
    @hidden action issue2726bmv2l88() {
        hdr.pool[0].base2 = hdr.vector[0].e;
    }
    @hidden action issue2726bmv2l88_0() {
        hdr.pool[1].base2 = hdr.vector[0].e;
    }
    @hidden action issue2726bmv2l88_1() {
        hdr.pool[2].base2 = hdr.vector[0].e;
    }
    @hidden action issue2726bmv2l88_2() {
        hdr.pool[2].base2 = hdr.vector[0].e;
    }
    @hidden action issue2726bmv2l88_3() {
        hdr.pool[8s2] = hsVar3;
    }
    @hidden action issue2726bmv2l88_4() {
        hsiVar2 = (int<8>)hdr.ml.idx;
    }
    @hidden action issue2726bmv2l90() {
        hdr.vector[1].e = hdr.pool[0].base0;
    }
    @hidden action issue2726bmv2l90_0() {
        hdr.vector[1].e = hdr.pool[1].base0;
    }
    @hidden action issue2726bmv2l90_1() {
        hdr.vector[1].e = hdr.pool[2].base0;
    }
    @hidden action issue2726bmv2l90_2() {
        hdr.vector[1].e = hdr.pool[2].base0;
    }
    @hidden action issue2726bmv2l90_3() {
        hdr.pool[8s2] = hsVar5;
    }
    @hidden action issue2726bmv2l90_4() {
        hsiVar4 = (int<8>)hdr.ml.idx;
    }
    @hidden action issue2726bmv2l92() {
        hdr.pool[0].base0 = hdr.pool[0].base1 + 8w1;
    }
    @hidden action issue2726bmv2l92_0() {
        hdr.pool[1].base0 = hdr.pool[1].base1 + 8w1;
    }
    @hidden action issue2726bmv2l92_1() {
        hdr.pool[2].base0 = hdr.pool[2].base1 + 8w1;
    }
    @hidden action issue2726bmv2l92_2() {
        hdr.pool[2].base0 = hdr.pool[2].base1 + 8w1;
    }
    @hidden action issue2726bmv2l92_3() {
        hdr.pool[8s2] = hsVar7;
    }
    @hidden action issue2726bmv2l92_4() {
        hsiVar6 = (int<8>)hdr.ml.idx;
    }
    @hidden action issue2726bmv2l93() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @hidden table tbl_issue2726bmv2l81 {
        actions = {
            issue2726bmv2l81();
        }
        const default_action = issue2726bmv2l81();
    }
    @hidden table tbl_issue2726bmv2l87 {
        actions = {
            issue2726bmv2l87();
        }
        const default_action = issue2726bmv2l87();
    }
    @hidden table tbl_issue2726bmv2l87_0 {
        actions = {
            issue2726bmv2l87_0();
        }
        const default_action = issue2726bmv2l87_0();
    }
    @hidden table tbl_issue2726bmv2l87_1 {
        actions = {
            issue2726bmv2l87_1();
        }
        const default_action = issue2726bmv2l87_1();
    }
    @hidden table tbl_issue2726bmv2l87_2 {
        actions = {
            issue2726bmv2l87_3();
        }
        const default_action = issue2726bmv2l87_3();
    }
    @hidden table tbl_issue2726bmv2l87_3 {
        actions = {
            issue2726bmv2l87_2();
        }
        const default_action = issue2726bmv2l87_2();
    }
    @hidden table tbl_issue2726bmv2l88 {
        actions = {
            issue2726bmv2l88_4();
        }
        const default_action = issue2726bmv2l88_4();
    }
    @hidden table tbl_issue2726bmv2l88_0 {
        actions = {
            issue2726bmv2l88();
        }
        const default_action = issue2726bmv2l88();
    }
    @hidden table tbl_issue2726bmv2l88_1 {
        actions = {
            issue2726bmv2l88_0();
        }
        const default_action = issue2726bmv2l88_0();
    }
    @hidden table tbl_issue2726bmv2l88_2 {
        actions = {
            issue2726bmv2l88_1();
        }
        const default_action = issue2726bmv2l88_1();
    }
    @hidden table tbl_issue2726bmv2l88_3 {
        actions = {
            issue2726bmv2l88_3();
        }
        const default_action = issue2726bmv2l88_3();
    }
    @hidden table tbl_issue2726bmv2l88_4 {
        actions = {
            issue2726bmv2l88_2();
        }
        const default_action = issue2726bmv2l88_2();
    }
    @hidden table tbl_issue2726bmv2l90 {
        actions = {
            issue2726bmv2l90_4();
        }
        const default_action = issue2726bmv2l90_4();
    }
    @hidden table tbl_issue2726bmv2l90_0 {
        actions = {
            issue2726bmv2l90();
        }
        const default_action = issue2726bmv2l90();
    }
    @hidden table tbl_issue2726bmv2l90_1 {
        actions = {
            issue2726bmv2l90_0();
        }
        const default_action = issue2726bmv2l90_0();
    }
    @hidden table tbl_issue2726bmv2l90_2 {
        actions = {
            issue2726bmv2l90_1();
        }
        const default_action = issue2726bmv2l90_1();
    }
    @hidden table tbl_issue2726bmv2l90_3 {
        actions = {
            issue2726bmv2l90_3();
        }
        const default_action = issue2726bmv2l90_3();
    }
    @hidden table tbl_issue2726bmv2l90_4 {
        actions = {
            issue2726bmv2l90_2();
        }
        const default_action = issue2726bmv2l90_2();
    }
    @hidden table tbl_issue2726bmv2l92 {
        actions = {
            issue2726bmv2l92_4();
        }
        const default_action = issue2726bmv2l92_4();
    }
    @hidden table tbl_issue2726bmv2l92_0 {
        actions = {
            issue2726bmv2l92();
        }
        const default_action = issue2726bmv2l92();
    }
    @hidden table tbl_issue2726bmv2l92_1 {
        actions = {
            issue2726bmv2l92_0();
        }
        const default_action = issue2726bmv2l92_0();
    }
    @hidden table tbl_issue2726bmv2l92_2 {
        actions = {
            issue2726bmv2l92_1();
        }
        const default_action = issue2726bmv2l92_1();
    }
    @hidden table tbl_issue2726bmv2l92_3 {
        actions = {
            issue2726bmv2l92_3();
        }
        const default_action = issue2726bmv2l92_3();
    }
    @hidden table tbl_issue2726bmv2l92_4 {
        actions = {
            issue2726bmv2l92_2();
        }
        const default_action = issue2726bmv2l92_2();
    }
    @hidden table tbl_issue2726bmv2l93 {
        actions = {
            issue2726bmv2l93();
        }
        const default_action = issue2726bmv2l93();
    }
    apply {
        tbl_issue2726bmv2l81.apply();
        if (hsiVar0_0 == 8s0) {
            tbl_issue2726bmv2l87.apply();
        } else if (hsiVar0_0 == 8s1) {
            tbl_issue2726bmv2l87_0.apply();
        } else if (hsiVar0_0 == 8s2) {
            tbl_issue2726bmv2l87_1.apply();
        } else {
            tbl_issue2726bmv2l87_2.apply();
            if (hsiVar0_0 >= 8s3) {
                tbl_issue2726bmv2l87_3.apply();
            }
        }
        tbl_issue2726bmv2l88.apply();
        if (hsiVar2 == 8s0) {
            tbl_issue2726bmv2l88_0.apply();
        } else if (hsiVar2 == 8s1) {
            tbl_issue2726bmv2l88_1.apply();
        } else if (hsiVar2 == 8s2) {
            tbl_issue2726bmv2l88_2.apply();
        } else {
            tbl_issue2726bmv2l88_3.apply();
            if (hsiVar2 >= 8s3) {
                tbl_issue2726bmv2l88_4.apply();
            }
        }
        tbl_issue2726bmv2l90.apply();
        if (hsiVar4 == 8s0) {
            tbl_issue2726bmv2l90_0.apply();
        } else if (hsiVar4 == 8s1) {
            tbl_issue2726bmv2l90_1.apply();
        } else if (hsiVar4 == 8s2) {
            tbl_issue2726bmv2l90_2.apply();
        } else {
            tbl_issue2726bmv2l90_3.apply();
            if (hsiVar4 >= 8s3) {
                tbl_issue2726bmv2l90_4.apply();
            }
        }
        tbl_issue2726bmv2l92.apply();
        if (hsiVar6 == 8s0) {
            tbl_issue2726bmv2l92_0.apply();
        } else if (hsiVar6 == 8s1) {
            tbl_issue2726bmv2l92_1.apply();
        } else if (hsiVar6 == 8s2) {
            tbl_issue2726bmv2l92_2.apply();
        } else {
            tbl_issue2726bmv2l92_3.apply();
            if (hsiVar6 >= 8s3) {
                tbl_issue2726bmv2l92_4.apply();
            }
        }
        tbl_issue2726bmv2l93.apply();
    }
}

control egress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ml_hdr_t>(hdr.ml);
        packet.emit<vec_e_t>(hdr.vector[0]);
        packet.emit<vec_e_t>(hdr.vector[1]);
        packet.emit<vec_e_t>(hdr.vector[2]);
        packet.emit<aggregator_t>(hdr.pool[0]);
        packet.emit<aggregator_t>(hdr.pool[1]);
        packet.emit<aggregator_t>(hdr.pool[2]);
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), ingress(), egress(), MyComputeChecksum(), MyDeparser()) main;

