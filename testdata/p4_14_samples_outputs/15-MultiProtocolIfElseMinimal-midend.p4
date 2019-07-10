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

struct metadata {
    bit<1> _ing_metadata_drop0;
    bit<8> _ing_metadata_egress_port1;
    bit<4> _ing_metadata_packet_type2;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".ipv4") 
    ipv4_t     ipv4;
    @name(".vlan_tag") 
    vlan_tag_t vlan_tag;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".nop") action nop() {
    }
    @name(".nop") action nop_2() {
    }
    @name(".set_egress_port") action set_egress_port(bit<8> egress_port) {
        meta._ing_metadata_egress_port1 = egress_port;
    }
    @name(".set_egress_port") action set_egress_port_2(bit<8> egress_port) {
        meta._ing_metadata_egress_port1 = egress_port;
    }
    @name(".ipv4_match") table ipv4_match_0 {
        actions = {
            nop();
            set_egress_port();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.ipv4.srcAddr: exact @name("ipv4.srcAddr") ;
        }
        default_action = NoAction_0();
    }
    @name(".l2_match") table l2_match_0 {
        actions = {
            nop_2();
            set_egress_port_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr") ;
        }
        default_action = NoAction_3();
    }
    apply {
        if (hdr.ethernet.etherType == 16w0x800) {
            ipv4_match_0.apply();
        } else {
            l2_match_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<vlan_tag_t>(hdr.vlan_tag);
        packet.emit<ipv4_t>(hdr.ipv4);
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

