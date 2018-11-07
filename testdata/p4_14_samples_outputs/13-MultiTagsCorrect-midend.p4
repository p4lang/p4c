#include <core.p4>
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
        packet.extract<my_tag_t>(hdr.my_tag);
        transition select(hdr.my_tag.ethertype) {
            default: accept;
        }
    }
    @name(".parse_my_tag_outer") state parse_my_tag_outer {
        packet.extract<my_tag_t>(hdr.my_tag);
        transition select(hdr.my_tag.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag_inner;
            default: accept;
        }
    }
    @name(".parse_vlan_tag_inner") state parse_vlan_tag_inner {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ethertype) {
            default: accept;
        }
    }
    @name(".parse_vlan_tag_outer") state parse_vlan_tag_outer {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ethertype) {
            16w0x9000: parse_my_tag_inner;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag_outer;
            16w0x9000: parse_my_tag_outer;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".nop") action nop() {
    }
    @name(".t2") table t2_0 {
        actions = {
            nop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr") ;
        }
        default_action = NoAction_0();
    }
    apply {
        t2_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_1() {
    }
    @name(".nop") action nop_2() {
    }
    @name(".t1") table t1_0 {
        actions = {
            nop_2();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("ethernet.dstAddr") ;
        }
        default_action = NoAction_1();
    }
    apply {
        t1_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
        packet.emit<my_tag_t>(hdr.my_tag);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

