#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header my_tag_t {
    bit<32> bd;
    bit<16> ethertype;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vlan_id;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".my_tag") 
    my_tag_t   my_tag;
    @name(".vlan_tag") 
    vlan_tag_t vlan_tag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_my_tag_inner") state parse_my_tag_inner {
        packet.extract(hdr.my_tag);
        transition select(hdr.my_tag.ethertype) {
            default: accept;
        }
    }
    @name(".parse_my_tag_outer") state parse_my_tag_outer {
        packet.extract(hdr.my_tag);
        transition select(hdr.my_tag.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag_inner;
            default: accept;
        }
    }
    @name(".parse_vlan_tag_inner") state parse_vlan_tag_inner {
        packet.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ethertype) {
            default: accept;
        }
    }
    @name(".parse_vlan_tag_outer") state parse_vlan_tag_outer {
        packet.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ethertype) {
            16w0x9000: parse_my_tag_inner;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag_outer;
            16w0x9000: parse_my_tag_outer;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t2") table t2 {
        actions = {
            nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
    }
    apply {
        t2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t1") table t1 {
        actions = {
            nop;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag);
        packet.emit(hdr.my_tag);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

