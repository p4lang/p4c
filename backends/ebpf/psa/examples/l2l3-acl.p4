/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

/*
 * This P4 program implements following functionality:
 * - VLAN tagging
 * - L2 forwarding
 * - L3 routing with ECMP
 * - checksum verification & re-calculation
 * - MAC learning
 * - ACL based on 5-tuple
 * - port counters
 */

#include <core.p4>
#include <psa.p4>

const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_VLAN = 0x8100;
const bit<8> PROTO_TCP = 6;
const bit<8> PROTO_UDP = 17;

typedef bit<12> vlan_id_t;
typedef bit<48> ethernet_addr_t;

struct empty_metadata_t {
}

header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
}

header vlan_tag_t {
    bit<3> pri;
    bit<1> cfi;
    vlan_id_t vlan_id;
    bit<16> eth_type;
}

header ipv4_t {
    bit<8> ver_ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header tcp_t {
    bit<16> sport;
    bit<16> dport;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_t {
    bit<16> sport;
    bit<16> dport;
    bit<16> len;
    bit<16> checksum;
}

header bridged_md_t {
    bit<32> ingress_port;
}

struct headers_t {
    bridged_md_t bridged_meta;
    ethernet_t ethernet;
    vlan_tag_t vlan_tag;
    ipv4_t ipv4;
    tcp_t  tcp;
    udp_t  udp;
}

struct mac_learn_digest_t {
    ethernet_addr_t mac_addr;
    PortId_t        port;
    vlan_id_t       vlan_id;
}

struct local_metadata_t {
    bool               send_mac_learn_msg;
    mac_learn_digest_t mac_learn_msg;
    bit<16>            l4_sport;
    bit<16>            l4_dport;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_parser_input_metadata_t standard_metadata, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    InternetChecksum() ck;
    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_VLAN : parse_vlan;
            default: accept;
        }
    }

    state parse_vlan {
        packet.extract(headers.vlan_tag);
        transition select(headers.vlan_tag.eth_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(headers.ipv4);

        ck.subtract(headers.ipv4.hdr_checksum);
        ck.subtract({/* 16-bit word */ headers.ipv4.ttl, headers.ipv4.protocol });
        headers.ipv4.hdr_checksum = ck.get();

        transition select(headers.ipv4.protocol) {
            PROTO_TCP: parse_tcp;
            PROTO_UDP: parse_udp;
            default: accept;
        }
    }

    state parse_tcp {
        packet.extract(headers.tcp);
        local_metadata.l4_sport = headers.tcp.sport;
        local_metadata.l4_dport = headers.tcp.dport;
        transition accept;
    }

    state parse_udp {
        packet.extract(headers.udp);
        local_metadata.l4_sport = headers.udp.sport;
        local_metadata.l4_dport = headers.udp.dport;
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_ingress_output_metadata_t istd) {
    Digest<mac_learn_digest_t>() mac_learn_digest;
    InternetChecksum() ck;
    apply {
        if (local_metadata.send_mac_learn_msg) {
            mac_learn_digest.pack(local_metadata.mac_learn_msg);
        }

        ck.subtract(headers.ipv4.hdr_checksum);
        ck.add({/* 16-bit word */ headers.ipv4.ttl, headers.ipv4.protocol });
        headers.ipv4.hdr_checksum = ck.get();

        packet.emit(headers.bridged_meta);
        packet.emit(headers.ethernet);
        packet.emit(headers.vlan_tag);
        packet.emit(headers.ipv4);
        packet.emit(headers.tcp);
        packet.emit(headers.udp);
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_input_metadata_t standard_metadata,
                inout psa_ingress_output_metadata_t ostd) {

    ActionSelector(PSA_HashAlgorithm_t.CRC32, 32w1024, 32w16) as;
    Counter<bit<32>, bit<32>>(100, PSA_CounterType_t.PACKETS_AND_BYTES) in_pkts;

    action push_vlan() {
        headers.vlan_tag.setValid();
        headers.vlan_tag.eth_type = headers.ethernet.ether_type;
        headers.ethernet.ether_type = ETHERTYPE_VLAN;
    }

    table tbl_ingress_vlan {

        key = {
            standard_metadata.ingress_port : exact;
            headers.vlan_tag.isValid():      exact;
        }

        actions = {
            push_vlan;
            NoAction;
        }
    }

    action mac_learn() {
        local_metadata.send_mac_learn_msg = true;
        local_metadata.mac_learn_msg.mac_addr = headers.ethernet.src_addr;
        local_metadata.mac_learn_msg.port = standard_metadata.ingress_port;
        local_metadata.mac_learn_msg.vlan_id = headers.vlan_tag.vlan_id;
    }

    table tbl_mac_learning {
        key = {
            headers.ethernet.src_addr : exact;
        }

        actions = {
            mac_learn;
            NoAction;
        }

        const default_action = mac_learn();
    }

    table tbl_routable {
        key = {
            headers.ethernet.dst_addr : exact;
            headers.vlan_tag.vlan_id : exact;
        }

        actions = { NoAction; }
    }

    action drop() {
        ingress_drop(ostd);
    }

    action set_nexthop(ethernet_addr_t smac, ethernet_addr_t dmac, vlan_id_t vlan_id) {
        headers.ipv4.ttl = headers.ipv4.ttl - 1;
        headers.ethernet.src_addr = smac;
        headers.ethernet.dst_addr = dmac;
        headers.vlan_tag.vlan_id = vlan_id;
    }

    table tbl_routing {
        key = {
            headers.ipv4.dst_addr: lpm;
            headers.ipv4.src_addr: selector;
            local_metadata.l4_sport: selector;
        }
        actions = {
            set_nexthop;
        }
        psa_implementation = as;
    }

    action forward(PortId_t output_port) {
        send_to_port(ostd, output_port);
    }

    action broadcast(MulticastGroup_t grp_id) {
        multicast(ostd, grp_id);
    }

    table tbl_switching {
        key = {
            headers.ethernet.dst_addr : exact;
            headers.vlan_tag.vlan_id  : exact;
        }

        actions = {
            forward;
            broadcast;
        }
    }

    table tbl_acl {
        key = {
            headers.ipv4.src_addr : exact;
            headers.ipv4.dst_addr : exact;
            headers.ipv4.protocol : exact;
            local_metadata.l4_sport : exact;
            local_metadata.l4_dport : exact;
        }

        actions = {
            drop;
            NoAction;
        }

        const default_action = NoAction();
    }

    apply {
        in_pkts.count((bit<32>)standard_metadata.ingress_port);

        tbl_ingress_vlan.apply();
        tbl_mac_learning.apply();
        if (tbl_routable.apply().hit) {
            switch (tbl_routing.apply().action_run) {
                set_nexthop: {
                    if (headers.ipv4.ttl == 0) {
                        drop();
                        exit;
                    }
                }
            }
        }
        tbl_switching.apply();
        tbl_acl.apply();
        if (!ostd.drop) {
            headers.bridged_meta.setValid();
            headers.bridged_meta.ingress_port = (bit<32>) standard_metadata.ingress_port;
        }
    }

}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {

    DirectCounter<bit<32>>(PSA_CounterType_t.PACKETS_AND_BYTES) out_pkts;

    action strip_vlan() {
        headers.ethernet.ether_type = headers.vlan_tag.eth_type;
        headers.vlan_tag.setInvalid();
        out_pkts.count();
    }

    action mod_vlan(vlan_id_t vlan_id) {
        headers.vlan_tag.vlan_id = vlan_id;
        out_pkts.count();
    }

    table tbl_vlan_egress {
        key = {
            istd.egress_port : exact;
        }

        actions = {
            strip_vlan;
            mod_vlan;
        }

        psa_direct_counter = out_pkts;
    }

    apply {
        // Multicast Source Pruning
        if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST && istd.egress_port == (PortId_t) headers.bridged_meta.ingress_port) {
            egress_drop(ostd);
        }
        if (!ostd.drop) {
            tbl_vlan_egress.apply();
        }
    }
}

parser egress_parser(packet_in buffer, out headers_t headers, inout local_metadata_t local_metadata, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition parse_bridged_meta;
    }

    state parse_bridged_meta {
        buffer.extract(headers.bridged_meta);
        transition parse_ethernet;
    }

    state parse_ethernet {
        buffer.extract(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            ETHERTYPE_VLAN : parse_vlan;
            default: accept;
        }
    }

    state parse_vlan {
        buffer.extract(headers.vlan_tag);
        transition accept;
    }
}

control egress_deparser(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.vlan_tag);
    }
}


IngressPipeline(packet_parser(), ingress(), packet_deparser()) ip;

EgressPipeline(egress_parser(), egress(), egress_deparser()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
