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
    bit<2> hsiVar_0;
    @name("MainControlImpl.execute") action execute_1() {
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
    @hidden action pnaexamplevarIndex1l102() {
        key_0 = hdrs.vlan_tag[2w0].vid;
    }
    @hidden action pnaexamplevarIndex1l102_0() {
        key_0 = hdrs.vlan_tag[2w1].vid;
    }
    @hidden action pnaexamplevarIndex1l102_1() {
        key_0 = hsVar;
    }
    @hidden action pnaexamplevarIndex1l102_2() {
        hsiVar = meta.depth;
    }
    @hidden table tbl_pnaexamplevarIndex1l102 {
        actions = {
            pnaexamplevarIndex1l102_2();
        }
        const default_action = pnaexamplevarIndex1l102_2();
    }
    @hidden table tbl_pnaexamplevarIndex1l102_0 {
        actions = {
            pnaexamplevarIndex1l102();
        }
        const default_action = pnaexamplevarIndex1l102();
    }
    @hidden table tbl_pnaexamplevarIndex1l102_1 {
        actions = {
            pnaexamplevarIndex1l102_0();
        }
        const default_action = pnaexamplevarIndex1l102_0();
    }
    @hidden table tbl_pnaexamplevarIndex1l102_2 {
        actions = {
            pnaexamplevarIndex1l102_1();
        }
        const default_action = pnaexamplevarIndex1l102_1();
    }
    apply {
        tbl_pnaexamplevarIndex1l102.apply();
        if (hsiVar == 2w0) {
            tbl_pnaexamplevarIndex1l102_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_pnaexamplevarIndex1l102_1.apply();
        } else if (hsiVar >= 2w1) {
            tbl_pnaexamplevarIndex1l102_2.apply();
        }
        stub_0.apply();
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexamplevarIndex1l124() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[0]);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[1]);
    }
    @hidden table tbl_pnaexamplevarIndex1l124 {
        actions = {
            pnaexamplevarIndex1l124();
        }
        const default_action = pnaexamplevarIndex1l124();
    }
    apply {
        tbl_pnaexamplevarIndex1l124.apply();
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
