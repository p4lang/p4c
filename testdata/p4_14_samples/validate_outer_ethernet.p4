/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
        etherType : 16;
    }
}

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_VLAN 0x8100, 0x9100, 0x9200, 0x9300

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        default: ingress;
    }
}

#define VLAN_DEPTH 2
header vlan_tag_t vlan_tag_[VLAN_DEPTH];

parser parse_vlan {
    extract(vlan_tag_[next]);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        default: ingress;
    }
}

/* Packet types */
#define L2_UNICAST                             1
#define L2_MULTICAST                           2
#define L2_BROADCAST                           4

header_type ingress_metadata_t {
    fields {
        lkp_pkt_type : 3;
        lkp_mac_sa : 48;
        lkp_mac_da : 48;
        lkp_mac_type : 16;
    }
}

metadata ingress_metadata_t ingress_metadata;

action set_valid_outer_unicast_packet_untagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_UNICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

action set_valid_outer_unicast_packet_single_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_UNICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[0].etherType);
}

action set_valid_outer_unicast_packet_double_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_UNICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[1].etherType);
}

action set_valid_outer_unicast_packet_qinq_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_UNICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

action set_valid_outer_multicast_packet_untagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_MULTICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

action set_valid_outer_multicast_packet_single_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_MULTICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[0].etherType);
}

action set_valid_outer_multicast_packet_double_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_MULTICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[1].etherType);
}

action set_valid_outer_multicast_packet_qinq_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_MULTICAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

action set_valid_outer_broadcast_packet_untagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_BROADCAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

action set_valid_outer_broadcast_packet_single_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_BROADCAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[0].etherType);
}

action set_valid_outer_broadcast_packet_double_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_BROADCAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, vlan_tag_[1].etherType);
}

action set_valid_outer_broadcast_packet_qinq_tagged() {
    modify_field(ingress_metadata.lkp_pkt_type, L2_BROADCAST);
    modify_field(ingress_metadata.lkp_mac_sa, ethernet.srcAddr);
    modify_field(ingress_metadata.lkp_mac_da, ethernet.dstAddr);
    modify_field(ingress_metadata.lkp_mac_type, ethernet.etherType);
}

table validate_outer_ethernet {
    reads {
        ethernet.dstAddr : ternary;
        vlan_tag_[0] : valid;
        vlan_tag_[1] : valid;
    }
    actions {
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
    size : 64;
}

control ingress {
    apply(validate_outer_ethernet);
}
