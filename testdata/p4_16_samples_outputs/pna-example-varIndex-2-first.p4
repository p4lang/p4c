#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
enum bit<16> ether_type_t {
    TPID = 16w0x8100,
    IPV4 = 16w0x800,
    IPV6 = 16w0x86dd
}

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header vlan_tag_h {
    bit<3>       pcp;
    bit<1>       cfi;
    bit<12>      vid;
    ether_type_t ether_type;
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
            ether_type_t.TPID: parse_vlan_tag;
            default: accept;
        }
    }
    state parse_vlan_tag {
        pkt.extract<vlan_tag_h>(hdrs.vlan_tag.next);
        meta.depth = meta.depth + 2w3;
        transition select(hdrs.vlan_tag.last.ether_type) {
            ether_type_t.TPID: parse_vlan_tag;
            default: accept;
        }
    }
}

control MainControlImpl(inout headers_t hdrs, inout main_metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action execute() {
        hdrs.vlan_tag[meta.depth].vid = hdrs.vlan_tag[0].vid;
    }
    action execute_1() {
        drop_packet();
    }
    table stub {
        key = {
            hdrs.vlan_tag[meta.depth].vid: exact @name("hdrs.vlan_tag[meta.depth].vid");
        }
        actions = {
            execute();
        }
        const default_action = execute();
        size = 1000000;
    }
    table stub1 {
        key = {
            hdrs.ethernet.etherType: exact @name("hdrs.ethernet.etherType");
        }
        actions = {
            execute_1();
        }
        const default_action = execute_1();
        size = 1000000;
    }
    apply {
        switch (hdrs.vlan_tag[meta.depth].vid) {
            12w1: {
                stub.apply();
            }
            12w2: {
                stub1.apply();
            }
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in main_metadata_t user_meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[0]);
        pkt.emit<vlan_tag_h>(hdr.vlan_tag[1]);
    }
}

PNA_NIC<headers_t, main_metadata_t, headers_t, main_metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
