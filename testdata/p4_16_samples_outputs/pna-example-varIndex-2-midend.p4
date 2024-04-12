#include <core.p4>
#include <pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header vlan_tag_h {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t    ethernet;
    vlan_tag_h[2] vlan_tag;
}

struct main_metadata_t {
    bit<2>  depth;
    bit<16> ethType;
}

control PreControlImpl(in headers_t hdrs, inout main_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

parser MainParserImpl(packet_in pkt, out headers_t hdrs, inout main_metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        meta.depth = 2w1;
        pkt.extract<ethernet_t>(hdrs.ethernet);
        transition select(hdrs.ethernet.etherType) {
            16w0x8100: parse_vlan_tag;
            default: accept;
        }
    }
    state parse_vlan_tag {
        pkt.extract<vlan_tag_h>(hdrs.vlan_tag.next);
        meta.depth = meta.depth + 2w3;
        transition select(hdrs.vlan_tag.last.ether_type) {
            16w0x8100: parse_vlan_tag;
            default: accept;
        }
    }
}

control MainControlImpl(inout headers_t hdrs, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    bit<2> hsiVar;
    bit<12> hsVar;
    @name("MainControlImpl.execute") action execute_1() {
        hsiVar = meta.depth;
        if (hsiVar == 2w0) {
            hdrs.vlan_tag[2w0].vid = hdrs.vlan_tag[0].vid;
        } else if (hsiVar == 2w1) {
            hdrs.vlan_tag[2w1].vid = hdrs.vlan_tag[0].vid;
        }
    }
    @name("MainControlImpl.execute_1") action execute_3() {
        drop_packet();
    }
    bit<12> key_0;
    @name("MainControlImpl.stub") table stub_0 {
        key = {
            key_0: exact @name("hdrs.vlan_tag[meta.depth].vid");
        }
        actions = {
            execute_1();
        }
        const default_action = execute_1();
        size = 1000000;
    }
    @name("MainControlImpl.stub1") table stub1_0 {
        key = {
            hdrs.ethernet.etherType: exact @name("hdrs.ethernet.etherType");
        }
        actions = {
            execute_3();
        }
        const default_action = execute_3();
        size = 1000000;
    }
    bit<12> switch_0_key;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
            switch_0_case_1();
        }
        const default_action = switch_0_case_1();
        const entries = {
                        const 12w1 : switch_0_case();
                        const 12w2 : switch_0_case_0();
        }
    }
    @hidden action pnaexamplevarIndex2l125() {
        switch_0_key = hdrs.vlan_tag[2w0].vid;
    }
    @hidden action pnaexamplevarIndex2l125_0() {
        switch_0_key = hdrs.vlan_tag[2w1].vid;
    }
    @hidden action pnaexamplevarIndex2l125_1() {
        switch_0_key = hsVar;
    }
    @hidden action pnaexamplevarIndex2l125_2() {
        hsiVar = meta.depth;
    }
    @hidden action pnaexamplevarIndex2l102() {
        key_0 = hdrs.vlan_tag[2w0].vid;
    }
    @hidden action pnaexamplevarIndex2l102_0() {
        key_0 = hdrs.vlan_tag[2w1].vid;
    }
    @hidden action pnaexamplevarIndex2l102_1() {
        key_0 = hsVar;
    }
    @hidden action pnaexamplevarIndex2l102_2() {
        hsiVar = meta.depth;
    }
    @hidden table tbl_pnaexamplevarIndex2l125 {
        actions = {
            pnaexamplevarIndex2l125_2();
        }
        const default_action = pnaexamplevarIndex2l125_2();
    }
    @hidden table tbl_pnaexamplevarIndex2l125_0 {
        actions = {
            pnaexamplevarIndex2l125();
        }
        const default_action = pnaexamplevarIndex2l125();
    }
    @hidden table tbl_pnaexamplevarIndex2l125_1 {
        actions = {
            pnaexamplevarIndex2l125_0();
        }
        const default_action = pnaexamplevarIndex2l125_0();
    }
    @hidden table tbl_pnaexamplevarIndex2l125_2 {
        actions = {
            pnaexamplevarIndex2l125_1();
        }
        const default_action = pnaexamplevarIndex2l125_1();
    }
    @hidden table tbl_pnaexamplevarIndex2l102 {
        actions = {
            pnaexamplevarIndex2l102_2();
        }
        const default_action = pnaexamplevarIndex2l102_2();
    }
    @hidden table tbl_pnaexamplevarIndex2l102_0 {
        actions = {
            pnaexamplevarIndex2l102();
        }
        const default_action = pnaexamplevarIndex2l102();
    }
    @hidden table tbl_pnaexamplevarIndex2l102_1 {
        actions = {
            pnaexamplevarIndex2l102_0();
        }
        const default_action = pnaexamplevarIndex2l102_0();
    }
    @hidden table tbl_pnaexamplevarIndex2l102_2 {
        actions = {
            pnaexamplevarIndex2l102_1();
        }
        const default_action = pnaexamplevarIndex2l102_1();
    }
    apply {
        tbl_pnaexamplevarIndex2l125.apply();
        if (hsiVar == 2w0) {
            tbl_pnaexamplevarIndex2l125_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_pnaexamplevarIndex2l125_1.apply();
        } else if (hsiVar >= 2w1) {
            tbl_pnaexamplevarIndex2l125_2.apply();
        }
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_pnaexamplevarIndex2l102.apply();
                if (hsiVar == 2w0) {
                    tbl_pnaexamplevarIndex2l102_0.apply();
                } else if (hsiVar == 2w1) {
                    tbl_pnaexamplevarIndex2l102_1.apply();
                } else if (hsiVar >= 2w1) {
                    tbl_pnaexamplevarIndex2l102_2.apply();
                }
                stub_0.apply();
            }
            switch_0_case_0: {
                stub1_0.apply();
            }
            switch_0_case_1: {
            }
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexamplevarIndex2l139() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[0]);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[1]);
    }
    @hidden table tbl_pnaexamplevarIndex2l139 {
        actions = {
            pnaexamplevarIndex2l139();
        }
        const default_action = pnaexamplevarIndex2l139();
    }
    apply {
        tbl_pnaexamplevarIndex2l139.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
