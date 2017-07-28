#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
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
    ethernet_t    ethernet;
    @name(".vlan_tag") 
    vlan_tag_t[5] vlan_tag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag;
            default: accept;
        }
    }
    @name(".parse_vlan_tag") state parse_vlan_tag {
        packet.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ethertype) {
            16w0x8100 &&& 16w0xefff: parse_vlan_tag;
            default: accept;
        }
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
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

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
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

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag);
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

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
