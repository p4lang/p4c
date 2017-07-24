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

struct __metadataImpl {
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("my_tag") 
    my_tag_t   my_tag;
    @name("vlan_tag") 
    vlan_tag_t vlan_tag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_my_tag") state parse_my_tag {
        packet.extract<my_tag_t>(hdr.my_tag);
        transition select(hdr.my_tag.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag;
            default: accept;
        }
    }
    @name(".parse_vlan_tag") state parse_vlan_tag {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ethertype) {
            16w0x9000: parse_my_tag;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag;
            16w0x9000: parse_my_tag;
            default: accept;
        }
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".nop") action nop_0() {
    }
    @name(".t2") table t2 {
        actions = {
            nop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        default_action = NoAction_0();
    }
    apply {
        t2.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_1() {
    }
    @name(".nop") action nop_1() {
    }
    @name(".t1") table t1 {
        actions = {
            nop_1();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        default_action = NoAction_1();
    }
    apply {
        t1.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
        packet.emit<my_tag_t>(hdr.my_tag);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
