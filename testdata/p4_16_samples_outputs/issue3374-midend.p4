#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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

parser parserImpl(packet_in pkt, out headers_t hdrs, inout main_metadata_t meta, inout standard_metadata_t stdmeta) {
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

control verifyChecksum(inout headers_t hdr, inout main_metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdrs, inout main_metadata_t meta, inout standard_metadata_t stdmeta) {
    bit<2> hsiVar;
    bit<12> hsVar;
    bit<2> hsiVar_0;
    bit<16> hsVar_0;
    @name("ingressImpl.execute") action execute() {
        hsiVar_0 = meta.depth + 2w3;
        if (hsiVar_0 == 2w0) {
            meta.ethType = hdrs.vlan_tag[2w0].ether_type;
        } else if (hsiVar_0 == 2w1) {
            meta.ethType = hdrs.vlan_tag[2w1].ether_type;
        } else if (hsiVar_0 >= 2w1) {
            meta.ethType = hsVar_0;
        }
        hsiVar_0 = meta.depth + 2w3;
        if (hsiVar_0 == 2w0) {
            hdrs.vlan_tag[2w0].ether_type = 16w2;
        } else if (hsiVar_0 == 2w1) {
            hdrs.vlan_tag[2w1].ether_type = 16w2;
        }
        hsiVar = meta.depth;
        if (hsiVar == 2w0) {
            hdrs.vlan_tag[2w0].vid = (bit<12>)hdrs.vlan_tag[2w0].cfi;
        } else if (hsiVar == 2w1) {
            hdrs.vlan_tag[2w1].vid = (bit<12>)hdrs.vlan_tag[2w1].cfi;
        }
        hsiVar = meta.depth;
        if (hsiVar == 2w0) {
            hsiVar_0 = meta.depth + 2w3;
            if (hsiVar_0 == 2w0) {
                hdrs.vlan_tag[2w0].vid = hdrs.vlan_tag[2w0].vid;
            } else if (hsiVar_0 == 2w1) {
                hdrs.vlan_tag[2w0].vid = hdrs.vlan_tag[2w1].vid;
            } else if (hsiVar_0 >= 2w1) {
                hdrs.vlan_tag[2w0].vid = hsVar;
            }
        } else if (hsiVar == 2w1) {
            hsiVar_0 = meta.depth + 2w3;
            if (hsiVar_0 == 2w0) {
                hdrs.vlan_tag[2w1].vid = hdrs.vlan_tag[2w0].vid;
            } else if (hsiVar_0 == 2w1) {
                hdrs.vlan_tag[2w1].vid = hdrs.vlan_tag[2w1].vid;
            } else if (hsiVar_0 >= 2w1) {
                hdrs.vlan_tag[2w1].vid = hsVar;
            }
        }
    }
    @name("ingressImpl.execute_1") action execute_1() {
        mark_to_drop(stdmeta);
    }
    bit<12> key_0;
    @name("ingressImpl.stub") table stub_0 {
        key = {
            key_0: exact @name("hdrs.vlan_tag[meta.depth].vid");
        }
        actions = {
            execute();
        }
        const default_action = execute();
        size = 1000000;
    }
    @name("ingressImpl.stub1") table stub1_0 {
        key = {
            hdrs.ethernet.etherType: exact @name("hdrs.ethernet.etherType");
        }
        actions = {
            execute_1();
        }
        const default_action = execute_1();
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
    @hidden action issue3374l126() {
        switch_0_key = hdrs.vlan_tag[2w0].vid;
    }
    @hidden action issue3374l126_0() {
        switch_0_key = hdrs.vlan_tag[2w1].vid;
    }
    @hidden action issue3374l126_1() {
        switch_0_key = hsVar;
    }
    @hidden action issue3374l126_2() {
        hsiVar = meta.depth;
    }
    @hidden action issue3374l104() {
        key_0 = hdrs.vlan_tag[2w0].vid;
    }
    @hidden action issue3374l104_0() {
        key_0 = hdrs.vlan_tag[2w1].vid;
    }
    @hidden action issue3374l104_1() {
        key_0 = hsVar;
    }
    @hidden action issue3374l104_2() {
        hsiVar = meta.depth;
    }
    @hidden action issue3374l130() {
        hsiVar = meta.depth;
    }
    @hidden table tbl_issue3374l126 {
        actions = {
            issue3374l126_2();
        }
        const default_action = issue3374l126_2();
    }
    @hidden table tbl_issue3374l126_0 {
        actions = {
            issue3374l126();
        }
        const default_action = issue3374l126();
    }
    @hidden table tbl_issue3374l126_1 {
        actions = {
            issue3374l126_0();
        }
        const default_action = issue3374l126_0();
    }
    @hidden table tbl_issue3374l126_2 {
        actions = {
            issue3374l126_1();
        }
        const default_action = issue3374l126_1();
    }
    @hidden table tbl_issue3374l104 {
        actions = {
            issue3374l104_2();
        }
        const default_action = issue3374l104_2();
    }
    @hidden table tbl_issue3374l104_0 {
        actions = {
            issue3374l104();
        }
        const default_action = issue3374l104();
    }
    @hidden table tbl_issue3374l104_1 {
        actions = {
            issue3374l104_0();
        }
        const default_action = issue3374l104_0();
    }
    @hidden table tbl_issue3374l104_2 {
        actions = {
            issue3374l104_1();
        }
        const default_action = issue3374l104_1();
    }
    @hidden table tbl_issue3374l130 {
        actions = {
            issue3374l130();
        }
        const default_action = issue3374l130();
    }
    apply {
        tbl_issue3374l126.apply();
        if (hsiVar == 2w0) {
            tbl_issue3374l126_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_issue3374l126_1.apply();
        } else if (hsiVar >= 2w1) {
            tbl_issue3374l126_2.apply();
        }
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_issue3374l104.apply();
                if (hsiVar == 2w0) {
                    tbl_issue3374l104_0.apply();
                } else if (hsiVar == 2w1) {
                    tbl_issue3374l104_1.apply();
                } else if (hsiVar >= 2w1) {
                    tbl_issue3374l104_2.apply();
                }
                stub_0.apply();
            }
            switch_0_case_0: {
                tbl_issue3374l130.apply();
                if (hsiVar == 2w0 && hdrs.vlan_tag[2w0].ether_type == hdrs.ethernet.etherType) {
                    stub1_0.apply();
                } else if (hsiVar == 2w1 && hdrs.vlan_tag[2w1].ether_type == hdrs.ethernet.etherType) {
                    stub1_0.apply();
                }
            }
            switch_0_case_1: {
            }
        }
    }
}

control egressImpl(inout headers_t hdr, inout main_metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout main_metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, main_metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
