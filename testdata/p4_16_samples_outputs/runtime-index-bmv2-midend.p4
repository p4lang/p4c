#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    int<8> hsiVar;
    bit<8> hsVar;
    @hidden action runtimeindexbmv2l70() {
        hdr.pool[8s0].val = hdr.pool[1].val + 8w1;
    }
    @hidden action runtimeindexbmv2l70_0() {
        hdr.pool[8s1].val = hdr.pool[1].val + 8w1;
    }
    @hidden action runtimeindexbmv2l70_1() {
        hdr.pool[8s2].val = hdr.pool[1].val + 8w1;
    }
    @hidden action runtimeindexbmv2l65() {
        meta.counter = meta.counter + 8s1;
        hdr.vector[0].e = hdr.pool[1].val + 8w1;
        hsiVar = hdr.ml.idx;
    }
    @hidden action runtimeindexbmv2l71() {
        hdr.pool[8s0].base2 = hdr.vector[0].e;
    }
    @hidden action runtimeindexbmv2l71_0() {
        hdr.pool[8s1].base2 = hdr.vector[0].e;
    }
    @hidden action runtimeindexbmv2l71_1() {
        hdr.pool[8s2].base2 = hdr.vector[0].e;
    }
    @hidden action runtimeindexbmv2l71_2() {
        hsiVar = hdr.ml.idx;
    }
    @hidden action runtimeindexbmv2l73() {
        hdr.vector[1].e = hdr.pool[8s0].base0;
    }
    @hidden action runtimeindexbmv2l73_0() {
        hdr.vector[1].e = hdr.pool[8s1].base0;
    }
    @hidden action runtimeindexbmv2l73_1() {
        hdr.vector[1].e = hdr.pool[8s2].base0;
    }
    @hidden action runtimeindexbmv2l73_2() {
        hdr.vector[1].e = hsVar;
    }
    @hidden action runtimeindexbmv2l73_3() {
        hsiVar = hdr.ml.idx;
    }
    @hidden action runtimeindexbmv2l75() {
        hdr.pool[8s0].base0 = hdr.pool[8s0].base1 + 8w1;
    }
    @hidden action runtimeindexbmv2l75_0() {
        hdr.pool[8s1].base0 = hdr.pool[8s1].base1 + 8w1;
    }
    @hidden action runtimeindexbmv2l75_1() {
        hdr.pool[8s2].base0 = hdr.pool[8s2].base1 + 8w1;
    }
    @hidden action runtimeindexbmv2l75_2() {
        hsiVar = hdr.ml.idx;
    }
    @hidden action runtimeindexbmv2l76() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @hidden table tbl_runtimeindexbmv2l65 {
        actions = {
            runtimeindexbmv2l65();
        }
        const default_action = runtimeindexbmv2l65();
    }
    @hidden table tbl_runtimeindexbmv2l70 {
        actions = {
            runtimeindexbmv2l70();
        }
        const default_action = runtimeindexbmv2l70();
    }
    @hidden table tbl_runtimeindexbmv2l70_0 {
        actions = {
            runtimeindexbmv2l70_0();
        }
        const default_action = runtimeindexbmv2l70_0();
    }
    @hidden table tbl_runtimeindexbmv2l70_1 {
        actions = {
            runtimeindexbmv2l70_1();
        }
        const default_action = runtimeindexbmv2l70_1();
    }
    @hidden table tbl_runtimeindexbmv2l71 {
        actions = {
            runtimeindexbmv2l71_2();
        }
        const default_action = runtimeindexbmv2l71_2();
    }
    @hidden table tbl_runtimeindexbmv2l71_0 {
        actions = {
            runtimeindexbmv2l71();
        }
        const default_action = runtimeindexbmv2l71();
    }
    @hidden table tbl_runtimeindexbmv2l71_1 {
        actions = {
            runtimeindexbmv2l71_0();
        }
        const default_action = runtimeindexbmv2l71_0();
    }
    @hidden table tbl_runtimeindexbmv2l71_2 {
        actions = {
            runtimeindexbmv2l71_1();
        }
        const default_action = runtimeindexbmv2l71_1();
    }
    @hidden table tbl_runtimeindexbmv2l73 {
        actions = {
            runtimeindexbmv2l73_3();
        }
        const default_action = runtimeindexbmv2l73_3();
    }
    @hidden table tbl_runtimeindexbmv2l73_0 {
        actions = {
            runtimeindexbmv2l73();
        }
        const default_action = runtimeindexbmv2l73();
    }
    @hidden table tbl_runtimeindexbmv2l73_1 {
        actions = {
            runtimeindexbmv2l73_0();
        }
        const default_action = runtimeindexbmv2l73_0();
    }
    @hidden table tbl_runtimeindexbmv2l73_2 {
        actions = {
            runtimeindexbmv2l73_1();
        }
        const default_action = runtimeindexbmv2l73_1();
    }
    @hidden table tbl_runtimeindexbmv2l73_3 {
        actions = {
            runtimeindexbmv2l73_2();
        }
        const default_action = runtimeindexbmv2l73_2();
    }
    @hidden table tbl_runtimeindexbmv2l75 {
        actions = {
            runtimeindexbmv2l75_2();
        }
        const default_action = runtimeindexbmv2l75_2();
    }
    @hidden table tbl_runtimeindexbmv2l75_0 {
        actions = {
            runtimeindexbmv2l75();
        }
        const default_action = runtimeindexbmv2l75();
    }
    @hidden table tbl_runtimeindexbmv2l75_1 {
        actions = {
            runtimeindexbmv2l75_0();
        }
        const default_action = runtimeindexbmv2l75_0();
    }
    @hidden table tbl_runtimeindexbmv2l75_2 {
        actions = {
            runtimeindexbmv2l75_1();
        }
        const default_action = runtimeindexbmv2l75_1();
    }
    @hidden table tbl_runtimeindexbmv2l76 {
        actions = {
            runtimeindexbmv2l76();
        }
        const default_action = runtimeindexbmv2l76();
    }
    apply {
        tbl_runtimeindexbmv2l65.apply();
        if (hsiVar == 8s0) {
            tbl_runtimeindexbmv2l70.apply();
        } else if (hsiVar == 8s1) {
            tbl_runtimeindexbmv2l70_0.apply();
        } else if (hsiVar == 8s2) {
            tbl_runtimeindexbmv2l70_1.apply();
        }
        tbl_runtimeindexbmv2l71.apply();
        if (hsiVar == 8s0) {
            tbl_runtimeindexbmv2l71_0.apply();
        } else if (hsiVar == 8s1) {
            tbl_runtimeindexbmv2l71_1.apply();
        } else if (hsiVar == 8s2) {
            tbl_runtimeindexbmv2l71_2.apply();
        }
        tbl_runtimeindexbmv2l73.apply();
        if (hsiVar == 8s0) {
            tbl_runtimeindexbmv2l73_0.apply();
        } else if (hsiVar == 8s1) {
            tbl_runtimeindexbmv2l73_1.apply();
        } else if (hsiVar == 8s2) {
            tbl_runtimeindexbmv2l73_2.apply();
        } else if (hsiVar >= 8s2) {
            tbl_runtimeindexbmv2l73_3.apply();
        }
        tbl_runtimeindexbmv2l75.apply();
        if (hsiVar == 8s0) {
            tbl_runtimeindexbmv2l75_0.apply();
        } else if (hsiVar == 8s1) {
            tbl_runtimeindexbmv2l75_1.apply();
        } else if (hsiVar == 8s2) {
            tbl_runtimeindexbmv2l75_2.apply();
        }
        tbl_runtimeindexbmv2l76.apply();
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
