#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<3>  lkp_pkt_type;
    bit<48> lkp_mac_sa;
    bit<48> lkp_mac_da;
    bit<16> lkp_mac_type;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
    @name(".ingress_metadata") 
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t    ethernet;
    @name(".vlan_tag_") 
    vlan_tag_t[2] vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            default: accept;
        }
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract(hdr.vlan_tag_.next);
        transition select(hdr.vlan_tag_.last.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x9200: parse_vlan;
            16w0x9300: parse_vlan;
            default: accept;
        }
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_valid_outer_unicast_packet_untagged") action set_valid_outer_unicast_packet_untagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w1;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_unicast_packet_single_tagged") action set_valid_outer_unicast_packet_single_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w1;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_unicast_packet_double_tagged") action set_valid_outer_unicast_packet_double_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w1;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_unicast_packet_qinq_tagged") action set_valid_outer_unicast_packet_qinq_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w1;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_untagged") action set_valid_outer_multicast_packet_untagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w2;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_multicast_packet_single_tagged") action set_valid_outer_multicast_packet_single_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w2;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_multicast_packet_double_tagged") action set_valid_outer_multicast_packet_double_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w2;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_multicast_packet_qinq_tagged") action set_valid_outer_multicast_packet_qinq_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w2;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_untagged") action set_valid_outer_broadcast_packet_untagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w4;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".set_valid_outer_broadcast_packet_single_tagged") action set_valid_outer_broadcast_packet_single_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w4;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[0].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_double_tagged") action set_valid_outer_broadcast_packet_double_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w4;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.vlan_tag_[1].etherType;
    }
    @name(".set_valid_outer_broadcast_packet_qinq_tagged") action set_valid_outer_broadcast_packet_qinq_tagged() {
        meta.ingress_metadata.lkp_pkt_type = 3w4;
        meta.ingress_metadata.lkp_mac_sa = hdr.ethernet.srcAddr;
        meta.ingress_metadata.lkp_mac_da = hdr.ethernet.dstAddr;
        meta.ingress_metadata.lkp_mac_type = hdr.ethernet.etherType;
    }
    @name(".validate_outer_ethernet") table validate_outer_ethernet {
        actions = {
            set_valid_outer_unicast_packet_untagged;
            set_valid_outer_unicast_packet_single_tagged;
            set_valid_outer_unicast_packet_double_tagged;
            set_valid_outer_unicast_packet_qinq_tagged;
            set_valid_outer_multicast_packet_untagged;
            set_valid_outer_multicast_packet_single_tagged;
            set_valid_outer_multicast_packet_double_tagged;
            set_valid_outer_multicast_packet_qinq_tagged;
            set_valid_outer_broadcast_packet_untagged;
            set_valid_outer_broadcast_packet_single_tagged;
            set_valid_outer_broadcast_packet_double_tagged;
            set_valid_outer_broadcast_packet_qinq_tagged;
        }
        key = {
            hdr.ethernet.dstAddr      : ternary;
            hdr.vlan_tag_[0].isValid(): exact;
            hdr.vlan_tag_[1].isValid(): exact;
        }
        size = 64;
    }
    apply {
        validate_outer_ethernet.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag_);
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

