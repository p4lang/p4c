#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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

parser parserImpl(packet_in pkt, out headers_t hdrs, inout main_metadata_t meta, inout standard_metadata_t stdmeta) {
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

control verifyChecksum(inout headers_t hdr, inout main_metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdrs, inout main_metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.execute") action execute() {
        meta.ethType = hdrs.vlan_tag[meta.depth + 2w3].ether_type;
        hdrs.vlan_tag[meta.depth + 2w3].ether_type = (ether_type_t)16w2;
        hdrs.vlan_tag[meta.depth].vid = (bit<12>)hdrs.vlan_tag[meta.depth].cfi;
        hdrs.vlan_tag[meta.depth].vid = hdrs.vlan_tag[meta.depth + 2w3].vid;
    }
    @name("ingressImpl.execute_1") action execute_1() {
        mark_to_drop(stdmeta);
    }
    @name("ingressImpl.stub") table stub_0 {
        key = {
            hdrs.vlan_tag[meta.depth].vid: exact @name("hdrs.vlan_tag[meta.depth].vid");
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
    apply {
        switch (hdrs.vlan_tag[meta.depth].vid) {
            12w1: {
                stub_0.apply();
            }
            12w2: {
                if (hdrs.vlan_tag[meta.depth].ether_type == hdrs.ethernet.etherType) {
                    stub1_0.apply();
                }
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
