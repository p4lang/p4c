#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1> drop;
    bit<8> egress_port;
    bit<4> packet_type;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct __metadataImpl {
    @name("ing_metadata") 
    ingress_metadata_t  ing_metadata;
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("vlan_tag") 
    vlan_tag_t vlan_tag;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".parse_vlan_tag") state parse_vlan_tag {
        packet.extract<vlan_tag_t>(hdr.vlan_tag);
        transition select(hdr.vlan_tag.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan_tag;
            16w0x9100: parse_vlan_tag;
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop_0() {
    }
    @name(".set_egress_port") action set_egress_port_0(bit<8> egress_port) {
        meta.ing_metadata.egress_port = egress_port;
    }
    @name(".ipv4_match") table ipv4_match_0 {
        actions = {
            nop_0();
            set_egress_port_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ipv4.srcAddr: exact @name("hdr.ipv4.srcAddr") ;
        }
        default_action = NoAction();
    }
    @name(".l2_match") table l2_match_0 {
        actions = {
            nop_0();
            set_egress_port_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        default_action = NoAction();
    }
    apply {
        if (hdr.ethernet.etherType == 16w0x800) 
            ipv4_match_0.apply();
        else 
            l2_match_0.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
        packet.emit<ipv4_t>(hdr.ipv4);
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
