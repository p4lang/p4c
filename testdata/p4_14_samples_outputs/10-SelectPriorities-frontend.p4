#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header other_tag_t {
    bit<16> field1;
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
    ethernet_t  ethernet;
    @name(".other_tag") 
    other_tag_t other_tag;
    @name(".vlan_tag") 
    vlan_tag_t  vlan_tag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_other_tag") state parse_other_tag {
        packet.extract<other_tag_t>(hdr.other_tag);
        transition accept;
    }
    @name(".parse_vlan_tag") state parse_vlan_tag {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x8100 &&& 16w0xff00: parse_vlan_tag;
            16w0x8153: parse_other_tag;
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
        packet.emit<other_tag_t>(hdr.other_tag);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
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

