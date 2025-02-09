/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include <core.p4>
#include <tofino1_specs.p4>
#include <tofino1_base.p4>
#include <tofino1_arch.p4>

const int register_size = 16384;
const int num_slots = register_size / 2;
const int max_num_workers = 32;
const int max_num_workers_log2 = 5;
const int forwarding_table_size = 1024;
const int max_num_queue_pairs_per_worker = 512;
const int max_num_queue_pairs_per_worker_log2 = 9;
const int max_num_queue_pairs = max_num_queue_pairs_per_worker * max_num_workers;
const int max_num_queue_pairs_log2 = max_num_queue_pairs_per_worker_log2 + max_num_workers_log2;
const bit<16> null_level1_exclusion_id = 0xffff;
typedef bit<3> mirror_type_t;
const mirror_type_t MIRROR_TYPE_I2E = 1;
const mirror_type_t MIRROR_TYPE_E2E = 2;
typedef bit<48> mac_addr_t;
typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x800;
const ether_type_t ETHERTYPE_ARP = 16w0x806;
const ether_type_t ETHERTYPE_ROCEv1 = 16w0x8915;
typedef bit<32> ipv4_addr_t;
enum bit<8> ip_protocol_t {
    ICMP = 1,
    UDP = 17
}

enum bit<16> arp_opcode_t {
    REQUEST = 1,
    REPLY = 2
}

enum bit<8> icmp_type_t {
    ECHO_REPLY = 0,
    ECHO_REQUEST = 8
}

typedef bit<16> udp_port_t;
const udp_port_t UDP_PORT_ROCEV2 = 4791;
const udp_port_t UDP_PORT_SWITCHML_BASE = 0xbee0;
const udp_port_t UDP_PORT_SWITCHML_MASK = 0xfff0;
typedef bit<128> ib_gid_t;
typedef bit<24> sequence_number_t;
typedef bit<24> queue_pair_t;
typedef bit<32> rkey_t;
typedef bit<64> addr_t;
enum bit<8> ib_opcode_t {
    UC_SEND_FIRST = 8w0b100000,
    UC_SEND_MIDDLE = 8w0b100001,
    UC_SEND_LAST = 8w0b100010,
    UC_SEND_LAST_IMMEDIATE = 8w0b100011,
    UC_SEND_ONLY = 8w0b100100,
    UC_SEND_ONLY_IMMEDIATE = 8w0b100101,
    UC_RDMA_WRITE_FIRST = 8w0b100110,
    UC_RDMA_WRITE_MIDDLE = 8w0b100111,
    UC_RDMA_WRITE_LAST = 8w0b101000,
    UC_RDMA_WRITE_LAST_IMMEDIATE = 8w0b101001,
    UC_RDMA_WRITE_ONLY = 8w0b101010,
    UC_RDMA_WRITE_ONLY_IMMEDIATE = 8w0b101011
}

typedef bit<(max_num_queue_pairs_log2)> queue_pair_index_t;
enum bit<2> worker_type_t {
    FORWARD_ONLY = 0,
    SWITCHML_UDP = 1,
    ROCEv2 = 2
}

typedef bit<16> worker_id_t;
typedef bit<32> worker_bitmap_t;
struct worker_bitmap_pair_t {
    worker_bitmap_t first;
    worker_bitmap_t second;
}

typedef bit<8> num_workers_t;
struct num_workers_pair_t {
    num_workers_t first;
    num_workers_t second;
}

typedef bit<15> pool_index_t;
typedef bit<14> pool_index_by2_t;
typedef bit<16> worker_pool_index_t;
typedef bit<32> value_t;
struct value_pair_t {
    value_t first;
    value_t second;
}

typedef int<8> exponent_t;
struct exponent_pair_t {
    exponent_t first;
    exponent_t second;
}

typedef bit<16> msg_id_t;
enum bit<3> packet_size_t {
    IBV_MTU_128 = 0,
    IBV_MTU_256 = 1,
    IBV_MTU_512 = 2,
    IBV_MTU_1024 = 3
}

typedef bit<16> drop_probability_t;
typedef bit<32> counter_t;
typedef bit<4> packet_type_underlying_t;
enum bit<4> packet_type_t {
    MIRROR = 0x0,
    BROADCAST = 0x1,
    RETRANSMIT = 0x2,
    IGNORE = 0x3,
    CONSUME0 = 0x4,
    CONSUME1 = 0x5,
    CONSUME2 = 0x6,
    CONSUME3 = 0x7,
    HARVEST0 = 0x8,
    HARVEST1 = 0x9,
    HARVEST2 = 0xa,
    HARVEST3 = 0xb,
    HARVEST4 = 0xc,
    HARVEST5 = 0xd,
    HARVEST6 = 0xe,
    HARVEST7 = 0xf
}

struct port_metadata_t {
    drop_probability_t ingress_drop_probability;
    drop_probability_t egress_drop_probability;
}

@pa_no_overlay("ingress" , "ig_md.switchml_md.simulate_egress_drop") @flexible header switchml_md_h {
    MulticastGroupId_t mgid;
    queue_pair_index_t recirc_port_selector;
    packet_size_t      packet_size;
    worker_type_t      worker_type;
    worker_id_t        worker_id;
    bit<16>            src_port;
    bit<16>            dst_port;
    packet_type_t      packet_type;
    bit<16>            ether_type_msb;
    pool_index_t       pool_index;
    num_workers_t      first_last_flag;
    worker_bitmap_t    map_result;
    worker_bitmap_t    worker_bitmap_before;
    bit<32>            tsi;
    bit<8>             job_number;
    PortId_t           ingress_port;
    bool               simulate_egress_drop;
    num_workers_t      num_workers;
    exponent_t         e0;
    exponent_t         e1;
    msg_id_t           msg_id;
    bool               first_packet;
    bool               last_packet;
}

@flexible header switchml_rdma_md_h {
    bit<64> rdma_addr;
}

@flexible struct ingress_metadata_t {
    switchml_md_h      switchml_md;
    switchml_rdma_md_h switchml_rdma_md;
    worker_bitmap_t    worker_bitmap;
    bool               checksum_err_ipv4;
    bool               update_ipv4_checksum;
    mac_addr_t         switch_mac;
    ipv4_addr_t        switch_ip;
    port_metadata_t    port_metadata;
}

struct egress_metadata_t {
    switchml_md_h      switchml_md;
    switchml_rdma_md_h switchml_rdma_md;
    bool               checksum_err_ipv4;
    bool               update_ipv4_checksum;
}

header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    ether_type;
}

header ipv4_h {
    bit<4>        version;
    bit<4>        ihl;
    bit<8>        diffserv;
    bit<16>       total_len;
    bit<16>       identification;
    bit<3>        flags;
    bit<13>       frag_offset;
    bit<8>        ttl;
    ip_protocol_t protocol;
    bit<16>       hdr_checksum;
    ipv4_addr_t   src_addr;
    ipv4_addr_t   dst_addr;
}

header icmp_h {
    icmp_type_t msg_type;
    bit<8>      msg_code;
    bit<16>     checksum;
}

header arp_h {
    bit<16>      hw_type;
    ether_type_t proto_type;
    bit<8>       hw_addr_len;
    bit<8>       proto_addr_len;
    arp_opcode_t opcode;
}

header arp_ipv4_h {
    mac_addr_t  src_hw_addr;
    ipv4_addr_t src_proto_addr;
    mac_addr_t  dst_hw_addr;
    ipv4_addr_t dst_proto_addr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header switchml_h {
    bit<4>        msg_type;
    bit<1>        unused;
    packet_size_t size;
    bit<8>        job_number;
    bit<32>       tsi;
    bit<16>       pool_index;
}

header ib_bth_h {
    ib_opcode_t       opcode;
    bit<1>            se;
    bit<1>            migration_req;
    bit<2>            pad_count;
    bit<4>            transport_version;
    bit<16>           partition_key;
    bit<1>            f_res1;
    bit<1>            b_res1;
    bit<6>            reserved;
    queue_pair_t      dst_qp;
    bit<1>            ack_req;
    bit<7>            reserved2;
    sequence_number_t psn;
}

@pa_container_size("ingress" , "hdr.ib_bth.psn" , 32) header ib_reth_h {
    bit<64> addr;
    bit<32> r_key;
    bit<32> len;
}

header ib_immediate_h {
    bit<32> immediate;
}

header ib_icrc_h {
    bit<32> icrc;
}

header exponents_h {
    exponent_t e0;
    exponent_t e1;
}

header data_h {
    value_t d00;
    value_t d01;
    value_t d02;
    value_t d03;
    value_t d04;
    value_t d05;
    value_t d06;
    value_t d07;
    value_t d08;
    value_t d09;
    value_t d10;
    value_t d11;
    value_t d12;
    value_t d13;
    value_t d14;
    value_t d15;
    value_t d16;
    value_t d17;
    value_t d18;
    value_t d19;
    value_t d20;
    value_t d21;
    value_t d22;
    value_t d23;
    value_t d24;
    value_t d25;
    value_t d26;
    value_t d27;
    value_t d28;
    value_t d29;
    value_t d30;
    value_t d31;
}

struct header_t {
    ethernet_h     ethernet;
    arp_h          arp;
    arp_ipv4_h     arp_ipv4;
    ipv4_h         ipv4;
    icmp_h         icmp;
    udp_h          udp;
    switchml_h     switchml;
    exponents_h    exponents;
    ib_bth_h       ib_bth;
    ib_reth_h      ib_reth;
    ib_immediate_h ib_immediate;
    data_h         d0;
    data_h         d1;
    ib_icrc_h      ib_icrc;
}

parser IngressParser(packet_in pkt, out header_t hdr, out ingress_metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() ipv4_checksum;
    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1: parse_resubmit;
            default: parse_port_metadata;
        }
    }
    state parse_resubmit {
        pkt.advance(64);
        transition parse_ethernet;
    }
    state parse_port_metadata {
        ig_md.port_metadata = port_metadata_unpack<port_metadata_t>(pkt);
        transition select(ig_intr_md.ingress_port) {
            64: parse_recirculate;
            68: parse_recirculate;
            320: parse_ethernet;
            0x80 &&& 0x180: parse_recirculate;
            0x100 &&& 0x180: parse_recirculate;
            0x180 &&& 0x180: parse_recirculate;
            default: parse_ethernet;
        }
    }
    state parse_recirculate {
        pkt.extract(ig_md.switchml_md);
        pkt.extract(ig_md.switchml_rdma_md);
        transition select(ig_md.switchml_md.packet_type) {
            packet_type_t.CONSUME0: parse_consume;
            packet_type_t.CONSUME1: parse_consume;
            packet_type_t.CONSUME2: parse_consume;
            packet_type_t.CONSUME3: parse_consume;
            default: parse_harvest;
        }
    }
    state parse_consume {
        pkt.extract(hdr.d0);
        pkt.extract(hdr.d1);
        transition accept;
    }
    state parse_harvest {
        hdr.d0.setValid();
        hdr.d1.setValid();
        transition accept;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_ARP: parse_arp;
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept_regular;
        }
    }
    state parse_arp {
        pkt.extract(hdr.arp);
        transition select(hdr.arp.hw_type, hdr.arp.proto_type) {
            (0x1, ETHERTYPE_IPV4): parse_arp_ipv4;
            default: accept_regular;
        }
    }
    state parse_arp_ipv4 {
        pkt.extract(hdr.arp_ipv4);
        transition accept_regular;
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        ig_md.checksum_err_ipv4 = ipv4_checksum.verify();
        ig_md.update_ipv4_checksum = false;
        transition select(hdr.ipv4.ihl, hdr.ipv4.frag_offset, hdr.ipv4.protocol) {
            (5, 0, ip_protocol_t.ICMP): parse_icmp;
            (5, 0, ip_protocol_t.UDP): parse_udp;
            default: accept_regular;
        }
    }
    state parse_icmp {
        pkt.extract(hdr.icmp);
        transition accept_regular;
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            UDP_PORT_ROCEV2: parse_ib_bth;
            UDP_PORT_SWITCHML_BASE &&& UDP_PORT_SWITCHML_MASK: parse_switchml;
            default: accept_regular;
        }
    }
    state parse_ib_bth {
        pkt.extract(hdr.ib_bth);
        transition select(hdr.ib_bth.opcode) {
            ib_opcode_t.UC_SEND_FIRST: parse_ib_payload;
            ib_opcode_t.UC_SEND_MIDDLE: parse_ib_payload;
            ib_opcode_t.UC_SEND_LAST: parse_ib_payload;
            ib_opcode_t.UC_SEND_LAST_IMMEDIATE: parse_ib_immediate;
            ib_opcode_t.UC_SEND_ONLY: parse_ib_payload;
            ib_opcode_t.UC_SEND_ONLY_IMMEDIATE: parse_ib_immediate;
            ib_opcode_t.UC_RDMA_WRITE_FIRST: parse_ib_reth;
            ib_opcode_t.UC_RDMA_WRITE_MIDDLE: parse_ib_payload;
            ib_opcode_t.UC_RDMA_WRITE_LAST: parse_ib_payload;
            ib_opcode_t.UC_RDMA_WRITE_LAST_IMMEDIATE: parse_ib_immediate;
            ib_opcode_t.UC_RDMA_WRITE_ONLY: parse_ib_reth;
            ib_opcode_t.UC_RDMA_WRITE_ONLY_IMMEDIATE: parse_ib_reth_immediate;
            default: accept_regular;
        }
    }
    state parse_ib_immediate {
        pkt.extract(hdr.ib_immediate);
        transition parse_ib_payload;
    }
    state parse_ib_reth {
        pkt.extract(hdr.ib_reth);
        transition parse_ib_payload;
    }
    state parse_ib_reth_immediate {
        pkt.extract(hdr.ib_reth);
        pkt.extract(hdr.ib_immediate);
        transition parse_ib_payload;
    }
    state parse_ib_payload {
        pkt.extract(hdr.d0);
        pkt.extract(hdr.d1);
        ig_md.switchml_md.setValid();
        ig_md.switchml_md.ether_type_msb = 16w0xffff;
        ig_md.switchml_md.packet_type = packet_type_t.CONSUME0;
        ig_md.switchml_rdma_md.setValid();
        transition accept;
    }
    state parse_switchml {
        pkt.extract(hdr.switchml);
        pkt.extract(hdr.exponents);
        transition parse_values;
    }
    state parse_values {
        pkt.extract(hdr.d0);
        pkt.extract(hdr.d1);
        ig_md.switchml_md.setValid();
        ig_md.switchml_md.packet_type = packet_type_t.CONSUME0;
        ig_md.switchml_rdma_md.setValid();
        transition accept;
    }
    state accept_regular {
        ig_md.switchml_md.setValid();
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
        ig_md.switchml_rdma_md.setValid();
        transition accept;
    }
}

control IngressDeparser(packet_out pkt, inout header_t hdr, in ingress_metadata_t ig_md, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    Checksum() ipv4_checksum;
    apply {
        if (ig_md.update_ipv4_checksum) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        }
        pkt.emit(ig_md.switchml_md);
        pkt.emit(ig_md.switchml_rdma_md);
        pkt.emit(hdr);
    }
}

parser EgressParser(packet_in pkt, out header_t hdr, out egress_metadata_t eg_md, out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        pkt.extract(eg_intr_md);
        transition select(eg_intr_md.pkt_length) {
            0: parse_switchml_md;
            default: parse_switchml_md;
        }
    }
    state parse_switchml_md {
        pkt.extract(eg_md.switchml_md);
        transition parse_switchml_rdma_md;
    }
    state parse_switchml_rdma_md {
        pkt.extract(eg_md.switchml_rdma_md);
        transition accept;
    }
}

control EgressDeparser(packet_out pkt, inout header_t hdr, in egress_metadata_t eg_md, in egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md) {
    Checksum() ipv4_checksum;
    apply {
        if (eg_md.update_ipv4_checksum) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        }
        pkt.emit(hdr);
    }
}

control ARPandICMPResponder(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    action send_back() {
        ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    }
    action send_arp_reply(mac_addr_t switch_mac, ipv4_addr_t switch_ip) {
        hdr.ethernet.dst_addr = hdr.arp_ipv4.src_hw_addr;
        hdr.ethernet.src_addr = switch_mac;
        hdr.arp.opcode = arp_opcode_t.REPLY;
        hdr.arp_ipv4.dst_hw_addr = hdr.arp_ipv4.src_hw_addr;
        hdr.arp_ipv4.dst_proto_addr = hdr.arp_ipv4.src_proto_addr;
        hdr.arp_ipv4.src_hw_addr = switch_mac;
        hdr.arp_ipv4.src_proto_addr = switch_ip;
        send_back();
    }
    action send_icmp_echo_reply(mac_addr_t switch_mac, ipv4_addr_t switch_ip) {
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = switch_mac;
        hdr.ipv4.dst_addr = hdr.ipv4.src_addr;
        hdr.ipv4.src_addr = switch_ip;
        hdr.icmp.msg_type = icmp_type_t.ECHO_REPLY;
        hdr.icmp.checksum = 0;
        send_back();
    }
    table arp_icmp {
        key = {
            hdr.arp_ipv4.isValid()     : exact;
            hdr.icmp.isValid()         : exact;
            hdr.arp.opcode             : ternary;
            hdr.arp_ipv4.dst_proto_addr: ternary;
            hdr.icmp.msg_type          : ternary;
            hdr.ipv4.dst_addr          : ternary;
        }
        actions = {
            send_arp_reply;
            send_icmp_echo_reply;
        }
        size = 2;
    }
    apply {
        arp_icmp.apply();
    }
}

control Forwarder(in header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    action set_egress_port(bit<9> egress_port) {
        ig_tm_md.ucast_egress_port = egress_port;
        ig_tm_md.bypass_egress = 1w1;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        ig_md.switchml_md.setInvalid();
        ig_md.switchml_rdma_md.setInvalid();
    }
    action flood(MulticastGroupId_t flood_mgid) {
        ig_tm_md.mcast_grp_a = flood_mgid;
        ig_tm_md.level1_exclusion_id = 7w0b1000000 ++ ig_intr_md.ingress_port;
        ig_tm_md.bypass_egress = 1w1;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        ig_md.switchml_md.setInvalid();
        ig_md.switchml_rdma_md.setInvalid();
    }
    table forward {
        key = {
            hdr.ethernet.dst_addr: exact;
        }
        actions = {
            set_egress_port;
            flood;
        }
        size = forwarding_table_size;
    }
    apply {
        forward.apply();
    }
}

control IngressDropSimulator(inout port_metadata_t port_metadata) {
    Random<drop_probability_t>() rng;
    apply {
        if (port_metadata.ingress_drop_probability != 0) {
            port_metadata.ingress_drop_probability = rng.get() |+| port_metadata.ingress_drop_probability;
        }
    }
}

control EgressDropSimulator(inout port_metadata_t port_metadata, in queue_pair_index_t qp_index, out bool simulate_egress_drop) {
    Random<drop_probability_t>() rng;
    Counter<counter_t, queue_pair_index_t>(max_num_queue_pairs, CounterType_t.PACKETS) simulated_drop_packet_counter;
    apply {
        if (port_metadata.egress_drop_probability != 0) {
            port_metadata.egress_drop_probability = rng.get() |+| port_metadata.egress_drop_probability;
        }
        simulate_egress_drop = false;
        if (port_metadata.egress_drop_probability == 0xffff) {
            simulate_egress_drop = true;
        }
        if (port_metadata.ingress_drop_probability == 0xffff || port_metadata.egress_drop_probability == 0xffff) {
            simulated_drop_packet_counter.count(qp_index);
        }
    }
}

control UDPReceiver(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    DirectCounter<counter_t>(CounterType_t.PACKETS_AND_BYTES) receive_counter;
    action drop() {
        ig_dprsr_md.drop_ctl[0:0] = 1;
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
        receive_counter.count();
    }
    action forward() {
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
        receive_counter.count();
    }
    action set_bitmap(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap) {
        receive_counter.count();
        ig_md.worker_bitmap = worker_bitmap;
        ig_md.switchml_md.num_workers = num_workers;
        ig_md.switchml_md.mgid = mgid;
        ig_md.switchml_md.packet_size = hdr.switchml.size;
        ig_md.switchml_md.worker_type = worker_type;
        ig_md.switchml_md.worker_id = worker_id;
        ig_md.switchml_md.dst_port = hdr.udp.src_port;
        ig_md.switchml_md.src_port = hdr.udp.dst_port;
        ig_md.switchml_md.tsi = hdr.switchml.tsi;
        ig_md.switchml_md.job_number = hdr.switchml.job_number;
        ig_md.switchml_md.pool_index = hdr.switchml.pool_index[13:0] ++ hdr.switchml.pool_index[15:15];
        ig_md.switchml_md.first_packet = true;
        ig_md.switchml_md.last_packet = true;
        ig_md.switchml_md.e0 = hdr.exponents.e0;
        ig_md.switchml_md.e1 = hdr.exponents.e1;
        hdr.ethernet.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.udp.setInvalid();
        hdr.switchml.setInvalid();
        hdr.exponents.setInvalid();
    }
    table receive_udp {
        key = {
            ig_intr_md.ingress_port: ternary;
            hdr.ethernet.src_addr  : ternary;
            hdr.ethernet.dst_addr  : ternary;
            hdr.ipv4.src_addr      : ternary;
            hdr.ipv4.dst_addr      : ternary;
            hdr.udp.dst_port       : ternary;
            ig_prsr_md.parser_err  : ternary;
        }
        actions = {
            drop;
            set_bitmap;
            @defaultonly forward;
        }
        const default_action = forward;
        size = max_num_workers + 16;
        counters = receive_counter;
    }
    apply {
        receive_udp.apply();
    }
}

control UDPSender(inout egress_metadata_t eg_md, in egress_intrinsic_metadata_t eg_intr_md, inout header_t hdr) {
    DirectCounter<counter_t>(CounterType_t.PACKETS_AND_BYTES) send_counter;
    action set_switch_mac_and_ip(mac_addr_t switch_mac, ipv4_addr_t switch_ip) {
        hdr.ethernet.src_addr = switch_mac;
        hdr.ipv4.src_addr = switch_ip;
        hdr.udp.src_port = eg_md.switchml_md.src_port;
        hdr.ethernet.ether_type = ETHERTYPE_IPV4;
        hdr.ipv4.version = 4;
        hdr.ipv4.ihl = 5;
        hdr.ipv4.diffserv = 0x0;
        hdr.ipv4.total_len = hdr.ipv4.minSizeInBytes() + (hdr.udp.minSizeInBytes() + hdr.switchml.minSizeInBytes() + hdr.exponents.minSizeInBytes());
        ;
        hdr.ipv4.identification = 0x0;
        hdr.ipv4.flags = 0b0;
        hdr.ipv4.frag_offset = 0;
        hdr.ipv4.ttl = 64;
        hdr.ipv4.protocol = ip_protocol_t.UDP;
        hdr.ipv4.hdr_checksum = 0;
        hdr.ipv4.src_addr = switch_ip;
        eg_md.update_ipv4_checksum = true;
        hdr.udp.length = hdr.udp.minSizeInBytes() + hdr.switchml.minSizeInBytes() + hdr.exponents.minSizeInBytes();
        hdr.switchml.setValid();
        hdr.switchml.msg_type = 1;
        hdr.switchml.unused = 0;
        hdr.switchml.size = eg_md.switchml_md.packet_size;
        hdr.switchml.job_number = eg_md.switchml_md.job_number;
        hdr.switchml.tsi = eg_md.switchml_md.tsi;
        hdr.switchml.pool_index[13:0] = eg_md.switchml_md.pool_index[14:1];
    }
    table switch_mac_and_ip {
        actions = {
            @defaultonly set_switch_mac_and_ip;
        }
        size = 1;
    }
    action set_dst_addr(mac_addr_t eth_dst_addr, ipv4_addr_t ip_dst_addr) {
        hdr.ethernet.dst_addr = eth_dst_addr;
        hdr.ipv4.dst_addr = ip_dst_addr;
        hdr.udp.dst_port = eg_md.switchml_md.dst_port;
        hdr.udp.checksum = 0;
        eg_md.update_ipv4_checksum = true;
        hdr.switchml.pool_index[15:15] = eg_md.switchml_md.pool_index[0:0];
        hdr.exponents.setValid();
        hdr.exponents.e0 = eg_md.switchml_md.e0;
        hdr.exponents.e1 = eg_md.switchml_md.e1;
        send_counter.count();
    }
    table dst_addr {
        key = {
            eg_md.switchml_md.worker_id: exact;
        }
        actions = {
            set_dst_addr;
        }
        size = max_num_workers;
        counters = send_counter;
    }
    apply {
        hdr.ethernet.setValid();
        hdr.ipv4.setValid();
        hdr.udp.setValid();
        hdr.switchml.setValid();
        hdr.switchml.pool_index = 16w0;
        switch_mac_and_ip.apply();
        dst_addr.apply();
        if (eg_md.switchml_md.packet_size == packet_size_t.IBV_MTU_256) {
            hdr.ipv4.total_len = hdr.ipv4.total_len + 256;
            hdr.udp.length = hdr.udp.length + 256;
        } else if (eg_md.switchml_md.packet_size == packet_size_t.IBV_MTU_1024) {
            hdr.ipv4.total_len = hdr.ipv4.total_len + 1024;
            hdr.udp.length = hdr.udp.length + 1024;
        }
    }
}

typedef bit<32> return_t;
struct receiver_data_t {
    bit<32> next_sequence_number;
    bit<32> pool_index;
}

control RDMAReceiver(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    DirectCounter<counter_t>(CounterType_t.PACKETS_AND_BYTES) rdma_receive_counter;
    Register<receiver_data_t, queue_pair_index_t>(max_num_queue_pairs) receiver_data_register;
    Counter<counter_t, queue_pair_index_t>(max_num_queue_pairs, CounterType_t.PACKETS) rdma_packet_counter;
    Counter<counter_t, queue_pair_index_t>(max_num_queue_pairs, CounterType_t.PACKETS) rdma_message_counter;
    Counter<counter_t, queue_pair_index_t>(max_num_queue_pairs, CounterType_t.PACKETS) rdma_sequence_violation_counter;
    bool message_possibly_received;
    bool sequence_violation;
    RegisterAction<receiver_data_t, queue_pair_index_t, return_t>(receiver_data_register) receiver_reset_action = {
        void apply(inout receiver_data_t value, out return_t read_value) {
            if ((bit<32>)hdr.ib_bth.psn == 0xffffff) {
                value.next_sequence_number = 0;
            } else {
                value.next_sequence_number = (bit<32>)hdr.ib_bth.psn + 1;
            }
            value.pool_index = (bit<32>)hdr.ib_reth.r_key;
            read_value = value.pool_index;
        }
    };
    RegisterAction<receiver_data_t, queue_pair_index_t, return_t>(receiver_data_register) receiver_increment_action = {
        void apply(inout receiver_data_t value, out return_t read_value) {
            if ((bit<32>)hdr.ib_bth.psn == value.next_sequence_number) {
                value.pool_index = value.pool_index |+| 2;
                if (value.next_sequence_number == 0xffffff) {
                    value.next_sequence_number = 0;
                } else {
                    value.next_sequence_number = value.next_sequence_number + 1;
                }
            } else {
                value.pool_index = 0x80000000;
                value.next_sequence_number = value.next_sequence_number;
            }
            read_value = value.pool_index;
        }
    };
    action set_bitmap(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        ig_md.worker_bitmap = worker_bitmap;
        ig_md.switchml_md.num_workers = num_workers;
        ig_md.switchml_md.mgid = mgid;
        ig_md.switchml_md.worker_type = worker_type;
        ig_md.switchml_md.worker_id = worker_id;
        ig_md.switchml_md.dst_port = hdr.udp.src_port;
        ig_md.switchml_rdma_md.rdma_addr = hdr.ib_reth.addr;
        ig_md.switchml_md.tsi = hdr.ib_reth.len;
        ig_md.switchml_md.packet_size = packet_size;
        ig_md.switchml_md.recirc_port_selector = (queue_pair_index_t)(hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0]);
        hdr.ethernet.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.udp.setInvalid();
        hdr.ib_bth.setInvalid();
        hdr.ib_reth.setInvalid();
        rdma_receive_counter.count();
    }
    action assign_pool_index(bit<32> result) {
        ig_md.switchml_md.pool_index = result[14:0];
    }
    action process_immediate() {
        ig_md.switchml_md.msg_id = hdr.ib_immediate.immediate[31:16];
        ig_md.switchml_md.e0 = (exponent_t)hdr.ib_immediate.immediate[15:8];
        ig_md.switchml_md.e1 = (exponent_t)hdr.ib_immediate.immediate[7:0];
        hdr.ib_immediate.setInvalid();
    }
    action only_packet(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        set_bitmap(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
        ig_md.switchml_md.first_packet = true;
        ig_md.switchml_md.last_packet = true;
        return_t result = receiver_reset_action.execute(hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0]);
        assign_pool_index(result);
        message_possibly_received = true;
        sequence_violation = (bool)result[31:31];
    }
    action only_packet_with_immediate(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        process_immediate();
        only_packet(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
    }
    action first_packet(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        set_bitmap(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
        ig_md.switchml_md.first_packet = true;
        ig_md.switchml_md.last_packet = false;
        return_t result = receiver_reset_action.execute(hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0]);
        assign_pool_index(result);
        message_possibly_received = false;
        sequence_violation = (bool)result[31:31];
    }
    action middle_packet(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        set_bitmap(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
        ig_md.switchml_md.first_packet = false;
        ig_md.switchml_md.last_packet = false;
        return_t result = receiver_increment_action.execute(hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0]);
        ig_dprsr_md.drop_ctl[0:0] = result[31:31];
        assign_pool_index(result);
        message_possibly_received = false;
        sequence_violation = (bool)result[31:31];
    }
    action last_packet(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        set_bitmap(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
        ig_md.switchml_md.first_packet = false;
        ig_md.switchml_md.last_packet = true;
        return_t result = receiver_increment_action.execute(hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0]);
        ig_dprsr_md.drop_ctl[0:0] = result[31:31];
        assign_pool_index(result);
        message_possibly_received = true;
        sequence_violation = (bool)result[31:31];
    }
    action last_packet_with_immediate(MulticastGroupId_t mgid, worker_type_t worker_type, worker_id_t worker_id, num_workers_t num_workers, worker_bitmap_t worker_bitmap, packet_size_t packet_size) {
        process_immediate();
        last_packet(mgid, worker_type, worker_id, num_workers, worker_bitmap, packet_size);
    }
    action forward() {
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
        rdma_receive_counter.count();
    }
    table receive_roce {
        key = {
            hdr.ipv4.src_addr       : exact;
            hdr.ipv4.dst_addr       : exact;
            hdr.ib_bth.partition_key: exact;
            hdr.ib_bth.opcode       : exact;
            hdr.ib_bth.dst_qp       : ternary;
        }
        actions = {
            only_packet;
            only_packet_with_immediate;
            first_packet;
            middle_packet;
            last_packet;
            last_packet_with_immediate;
            @defaultonly forward;
        }
        const default_action = forward;
        size = max_num_workers * 6;
        counters = rdma_receive_counter;
    }
    apply {
        if (receive_roce.apply().hit) {
            rdma_packet_counter.count(ig_md.switchml_md.recirc_port_selector);
            if (sequence_violation) {
                rdma_sequence_violation_counter.count(ig_md.switchml_md.recirc_port_selector);
            } else if (message_possibly_received) {
                rdma_message_counter.count(ig_md.switchml_md.recirc_port_selector);
            }
        }
    }
}

control RDMASender(inout header_t hdr, inout egress_metadata_t eg_md, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md) {
    rkey_t rdma_rkey;
    mac_addr_t rdma_switch_mac;
    ipv4_addr_t rdma_switch_ip;
    DirectCounter<counter_t>(CounterType_t.PACKETS_AND_BYTES) rdma_send_counter;
    action set_switch_mac_and_ip(mac_addr_t switch_mac, ipv4_addr_t switch_ip, bit<31> message_length, pool_index_t first_last_mask) {
        rdma_switch_mac = switch_mac;
        rdma_switch_ip = switch_ip;
    }
    table switch_mac_and_ip {
        actions = {
            @defaultonly set_switch_mac_and_ip;
        }
        size = 1;
    }
    action fill_in_roce_fields(mac_addr_t dest_mac, ipv4_addr_t dest_ip) {
        hdr.switchml.setInvalid();
        hdr.exponents.setInvalid();
        hdr.ethernet.setValid();
        hdr.ethernet.dst_addr = dest_mac;
        hdr.ethernet.src_addr = rdma_switch_mac;
        hdr.ethernet.ether_type = ETHERTYPE_IPV4;
        hdr.ipv4.setValid();
        hdr.ipv4.version = 4;
        hdr.ipv4.ihl = 5;
        hdr.ipv4.diffserv = 0x2;
        hdr.ipv4.identification = 0x1;
        hdr.ipv4.flags = 0b10;
        hdr.ipv4.frag_offset = 0;
        hdr.ipv4.ttl = 64;
        hdr.ipv4.protocol = ip_protocol_t.UDP;
        hdr.ipv4.hdr_checksum = 0;
        hdr.ipv4.src_addr = rdma_switch_ip;
        hdr.ipv4.dst_addr = dest_ip;
        hdr.ipv4.total_len = hdr.ib_icrc.minSizeInBytes() + hdr.ib_bth.minSizeInBytes() + hdr.udp.minSizeInBytes() + hdr.ipv4.minSizeInBytes();
        eg_md.update_ipv4_checksum = true;
        hdr.udp.setValid();
        hdr.udp.src_port = 1w1 ++ eg_md.switchml_md.worker_id[14:0];
        hdr.udp.dst_port = UDP_PORT_ROCEV2;
        hdr.udp.checksum = 0;
        hdr.udp.length = hdr.ib_icrc.minSizeInBytes() + hdr.ib_bth.minSizeInBytes() + hdr.udp.minSizeInBytes();
        hdr.ib_bth.setValid();
        hdr.ib_bth.opcode = ib_opcode_t.UC_RDMA_WRITE_ONLY;
        hdr.ib_bth.se = 0;
        hdr.ib_bth.migration_req = 1;
        hdr.ib_bth.pad_count = 0;
        hdr.ib_bth.transport_version = 0;
        hdr.ib_bth.partition_key = 0xffff;
        hdr.ib_bth.f_res1 = 0;
        hdr.ib_bth.b_res1 = 0;
        hdr.ib_bth.reserved = 0;
        hdr.ib_bth.dst_qp = 0;
        hdr.ib_bth.ack_req = 0;
        hdr.ib_bth.reserved2 = 0;
        rdma_send_counter.count();
    }
    action fill_in_roce_write_fields(mac_addr_t dest_mac, ipv4_addr_t dest_ip, rkey_t rkey) {
        fill_in_roce_fields(dest_mac, dest_ip);
        rdma_rkey = rkey;
    }
    table create_roce_packet {
        key = {
            eg_md.switchml_md.worker_id: exact;
        }
        actions = {
            fill_in_roce_fields;
            fill_in_roce_write_fields;
        }
        size = max_num_workers;
        counters = rdma_send_counter;
    }
    DirectRegister<bit<32>>() psn_register;
    DirectRegisterAction<bit<32>, bit<32>>(psn_register) psn_action = {
        void apply(inout bit<32> value, out bit<32> read_value) {
            bit<32> masked_sequence_number = value & 0xffffff;
            read_value = masked_sequence_number;
            bit<32> incremented_value = value + 1;
            value = incremented_value;
        }
    };
    action add_qpn_and_psn(queue_pair_t qpn) {
        hdr.ib_bth.dst_qp = qpn;
        hdr.ib_bth.psn = psn_action.execute()[23:0];
    }
    table fill_in_qpn_and_psn {
        key = {
            eg_md.switchml_md.worker_id : exact;
            eg_md.switchml_md.pool_index: ternary;
        }
        actions = {
            add_qpn_and_psn;
        }
        size = max_num_queue_pairs;
        registers = psn_register;
    }
    action set_opcode_common(ib_opcode_t opcode) {
        hdr.ib_bth.opcode = opcode;
    }
    action set_immediate() {
        hdr.ib_immediate.setValid();
        hdr.ib_immediate.immediate = eg_md.switchml_md.msg_id ++ +(bit<8>)eg_md.switchml_md.e0 ++ +(bit<8>)eg_md.switchml_md.e1;
    }
    action set_rdma() {
        hdr.ib_reth.setValid();
        hdr.ib_reth.r_key = rdma_rkey;
        hdr.ib_reth.len = eg_md.switchml_md.tsi;
        hdr.ib_reth.addr = eg_md.switchml_rdma_md.rdma_addr;
    }
    action set_first() {
        set_opcode_common(ib_opcode_t.UC_RDMA_WRITE_FIRST);
        set_rdma();
        hdr.udp.length = hdr.udp.length + (bit<16>)hdr.ib_reth.minSizeInBytes();
        hdr.ipv4.total_len = hdr.ipv4.total_len + (bit<16>)hdr.ib_reth.minSizeInBytes();
    }
    action set_middle() {
        set_opcode_common(ib_opcode_t.UC_RDMA_WRITE_MIDDLE);
    }
    action set_last_immediate() {
        set_opcode_common(ib_opcode_t.UC_RDMA_WRITE_LAST_IMMEDIATE);
        set_immediate();
        hdr.udp.length = hdr.udp.length + (bit<16>)hdr.ib_immediate.minSizeInBytes();
        hdr.ipv4.total_len = hdr.ipv4.total_len + (bit<16>)hdr.ib_immediate.minSizeInBytes();
    }
    action set_only_immediate() {
        set_opcode_common(ib_opcode_t.UC_RDMA_WRITE_ONLY_IMMEDIATE);
        set_rdma();
        set_immediate();
        hdr.udp.length = hdr.udp.length + (bit<16>)(hdr.ib_immediate.minSizeInBytes() + hdr.ib_reth.minSizeInBytes());
        hdr.ipv4.total_len = hdr.ipv4.total_len + (bit<16>)(hdr.ib_immediate.minSizeInBytes() + hdr.ib_reth.minSizeInBytes());
    }
    table set_opcodes {
        key = {
            eg_md.switchml_md.first_packet: exact;
            eg_md.switchml_md.last_packet : exact;
        }
        actions = {
            set_first;
            set_middle;
            set_last_immediate;
            set_only_immediate;
        }
        size = 4;
        const entries = {
                        (true, false) : set_first();
                        (false, false) : set_middle();
                        (false, true) : set_last_immediate();
                        (true, true) : set_only_immediate();
        }
    }
    apply {
        switch_mac_and_ip.apply();
        create_roce_packet.apply();
        if (eg_md.switchml_md.packet_size == packet_size_t.IBV_MTU_256) {
            hdr.ipv4.total_len = hdr.ipv4.total_len + 256;
            hdr.udp.length = hdr.udp.length + 256;
        } else if (eg_md.switchml_md.packet_size == packet_size_t.IBV_MTU_1024) {
            hdr.ipv4.total_len = hdr.ipv4.total_len + 1024;
            hdr.udp.length = hdr.udp.length + 1024;
        }
        fill_in_qpn_and_psn.apply();
        set_opcodes.apply();
    }
}

control ReconstructWorkerBitmap(inout ingress_metadata_t ig_md) {
    action reconstruct_worker_bitmap_from_worker_id(worker_bitmap_t bitmap) {
        ig_md.worker_bitmap = bitmap;
    }
    table reconstruct_worker_bitmap {
        key = {
            ig_md.switchml_md.worker_id: ternary;
        }
        actions = {
            reconstruct_worker_bitmap_from_worker_id;
        }
        const entries = {
                        0 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 0);
                        1 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 1);
                        2 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 2);
                        3 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 3);
                        4 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 4);
                        5 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 5);
                        6 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 6);
                        7 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 7);
                        8 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 8);
                        9 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 9);
                        10 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 10);
                        11 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 11);
                        12 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 12);
                        13 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 13);
                        14 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 14);
                        15 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 15);
                        16 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 16);
                        17 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 17);
                        18 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 18);
                        19 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 19);
                        20 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 20);
                        21 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 21);
                        22 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 22);
                        23 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 23);
                        24 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 24);
                        25 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 25);
                        26 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 26);
                        27 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 27);
                        28 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 28);
                        29 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 29);
                        30 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 30);
                        31 &&& 0x1f : reconstruct_worker_bitmap_from_worker_id(1 << 31);
        }
    }
    apply {
        reconstruct_worker_bitmap.apply();
    }
}

control UpdateAndCheckWorkerBitmap(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    Register<worker_bitmap_pair_t, pool_index_by2_t>(num_slots) worker_bitmap;
    RegisterAction<worker_bitmap_pair_t, pool_index_by2_t, worker_bitmap_t>(worker_bitmap) worker_bitmap_update_set0 = {
        void apply(inout worker_bitmap_pair_t value, out worker_bitmap_t return_value) {
            return_value = value.first;
            value.first = value.first | ig_md.worker_bitmap;
            value.second = value.second & ~ig_md.worker_bitmap;
        }
    };
    RegisterAction<worker_bitmap_pair_t, pool_index_by2_t, worker_bitmap_t>(worker_bitmap) worker_bitmap_update_set1 = {
        void apply(inout worker_bitmap_pair_t value, out worker_bitmap_t return_value) {
            return_value = value.second;
            value.first = value.first & ~ig_md.worker_bitmap;
            value.second = value.second | ig_md.worker_bitmap;
        }
    };
    action drop() {
        ig_dprsr_md.drop_ctl[0:0] = 1;
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
    }
    action simulate_drop() {
        drop();
    }
    action check_worker_bitmap_action() {
        ig_md.switchml_md.map_result = ig_md.switchml_md.worker_bitmap_before & ig_md.worker_bitmap;
    }
    action update_worker_bitmap_set0_action() {
        ig_md.switchml_md.worker_bitmap_before = worker_bitmap_update_set0.execute(ig_md.switchml_md.pool_index[14:1]);
        check_worker_bitmap_action();
    }
    action update_worker_bitmap_set1_action() {
        ig_md.switchml_md.worker_bitmap_before = worker_bitmap_update_set1.execute(ig_md.switchml_md.pool_index[14:1]);
        check_worker_bitmap_action();
    }
    table update_and_check_worker_bitmap {
        key = {
            ig_md.switchml_md.pool_index                : ternary;
            ig_md.switchml_md.packet_type               : ternary;
            ig_md.port_metadata.ingress_drop_probability: ternary;
        }
        actions = {
            update_worker_bitmap_set0_action;
            update_worker_bitmap_set1_action;
            drop;
            simulate_drop;
            NoAction;
        }
        const entries = {
                        (default, packet_type_t.CONSUME0, 0xffff) : simulate_drop();
                        (15w0 &&& 15w1, packet_type_t.CONSUME0, default) : update_worker_bitmap_set0_action();
                        (15w1 &&& 15w1, packet_type_t.CONSUME0, default) : update_worker_bitmap_set1_action();
                        (15w0 &&& 15w1, packet_type_t.CONSUME1, default) : update_worker_bitmap_set0_action();
                        (15w1 &&& 15w1, packet_type_t.CONSUME1, default) : update_worker_bitmap_set1_action();
                        (15w0 &&& 15w1, packet_type_t.CONSUME2, default) : update_worker_bitmap_set0_action();
                        (15w1 &&& 15w1, packet_type_t.CONSUME2, default) : update_worker_bitmap_set1_action();
                        (15w0 &&& 15w1, packet_type_t.CONSUME3, default) : update_worker_bitmap_set0_action();
                        (15w1 &&& 15w1, packet_type_t.CONSUME3, default) : update_worker_bitmap_set1_action();
        }
        const default_action = NoAction;
    }
    apply {
        update_and_check_worker_bitmap.apply();
    }
}

control WorkersCounter(in header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    Register<num_workers_pair_t, pool_index_t>(register_size) workers_count;
    RegisterAction<num_workers_pair_t, pool_index_t, num_workers_t>(workers_count) workers_count_action = {
        void apply(inout num_workers_pair_t value, out num_workers_t read_value) {
            read_value = value.first;
            if (value.first == 0) {
                value.first = ig_md.switchml_md.num_workers - 1;
            } else {
                value.first = value.first - 1;
            }
        }
    };
    RegisterAction<num_workers_pair_t, pool_index_t, num_workers_t>(workers_count) read_workers_count_action = {
        void apply(inout num_workers_pair_t value, out num_workers_t read_value) {
            read_value = value.first;
        }
    };
    action count_workers_action() {
        ig_md.switchml_md.first_last_flag = workers_count_action.execute(ig_md.switchml_md.pool_index);
    }
    action single_worker_count_action() {
        workers_count_action.execute(ig_md.switchml_md.pool_index);
        ig_md.switchml_md.first_last_flag = 1;
    }
    action single_worker_read_action() {
        ig_md.switchml_md.first_last_flag = 0;
    }
    action read_count_workers_action() {
        ig_md.switchml_md.first_last_flag = read_workers_count_action.execute(ig_md.switchml_md.pool_index);
    }
    table count_workers {
        key = {
            ig_md.switchml_md.num_workers: ternary;
            ig_md.switchml_md.map_result : ternary;
            ig_md.switchml_md.packet_type: ternary;
        }
        actions = {
            single_worker_count_action;
            single_worker_read_action;
            count_workers_action;
            read_count_workers_action;
            @defaultonly NoAction;
        }
        const entries = {
                        (1, 0, packet_type_t.CONSUME0) : single_worker_count_action();
                        (1, 0, packet_type_t.CONSUME1) : single_worker_count_action();
                        (1, 0, packet_type_t.CONSUME2) : single_worker_count_action();
                        (1, 0, packet_type_t.CONSUME3) : single_worker_count_action();
                        (1, default, packet_type_t.CONSUME0) : single_worker_read_action();
                        (1, default, packet_type_t.CONSUME1) : single_worker_read_action();
                        (1, default, packet_type_t.CONSUME2) : single_worker_read_action();
                        (1, default, packet_type_t.CONSUME3) : single_worker_read_action();
                        (default, 0, packet_type_t.CONSUME0) : count_workers_action();
                        (default, 0, packet_type_t.CONSUME1) : count_workers_action();
                        (default, 0, packet_type_t.CONSUME2) : count_workers_action();
                        (default, 0, packet_type_t.CONSUME3) : count_workers_action();
                        (default, default, packet_type_t.CONSUME0) : read_count_workers_action();
                        (default, default, packet_type_t.CONSUME1) : read_count_workers_action();
                        (default, default, packet_type_t.CONSUME2) : read_count_workers_action();
                        (default, default, packet_type_t.CONSUME3) : read_count_workers_action();
        }
        const default_action = NoAction;
    }
    apply {
        count_workers.apply();
    }
}

control Exponents(in exponent_t exponent0, in exponent_t exponent1, out exponent_t max_exponent0, out exponent_t max_exponent1, inout ingress_metadata_t ig_md) {
    Register<exponent_pair_t, pool_index_t>(register_size) exponents;
    RegisterAction<exponent_pair_t, pool_index_t, exponent_t>(exponents) write_read0_register_action = {
        void apply(inout exponent_pair_t value, out exponent_t read_value) {
            value.first = exponent0;
            value.second = exponent1;
            read_value = value.first;
        }
    };
    action write_read0_action() {
        max_exponent0 = write_read0_register_action.execute(ig_md.switchml_md.pool_index);
    }
    RegisterAction<exponent_pair_t, pool_index_t, exponent_t>(exponents) max_read0_register_action = {
        void apply(inout exponent_pair_t value, out exponent_t read_value) {
            value.first = max(value.first, exponent0);
            value.second = max(value.second, exponent1);
            read_value = value.first;
        }
    };
    action max_read0_action() {
        max_exponent0 = max_read0_register_action.execute(ig_md.switchml_md.pool_index);
    }
    RegisterAction<exponent_pair_t, pool_index_t, exponent_t>(exponents) read0_register_action = {
        void apply(inout exponent_pair_t value, out exponent_t read_value) {
            read_value = value.first;
        }
    };
    action read0_action() {
        max_exponent0 = read0_register_action.execute(ig_md.switchml_md.pool_index);
    }
    RegisterAction<exponent_pair_t, pool_index_t, exponent_t>(exponents) read1_register_action = {
        void apply(inout exponent_pair_t value, out exponent_t read_value) {
            read_value = value.second;
        }
    };
    action read1_action() {
        max_exponent1 = read1_register_action.execute(ig_md.switchml_md.pool_index);
    }
    table exponent_max {
        key = {
            ig_md.switchml_md.worker_bitmap_before: ternary;
            ig_md.switchml_md.map_result          : ternary;
            ig_md.switchml_md.packet_type         : ternary;
        }
        actions = {
            write_read0_action;
            max_read0_action;
            read0_action;
            read1_action;
            @defaultonly NoAction;
        }
        size = 4;
        const entries = {
                        (32w0, default, packet_type_t.CONSUME0) : write_read0_action();
                        (default, 32w0, packet_type_t.CONSUME0) : max_read0_action();
                        (default, default, packet_type_t.CONSUME0) : read0_action();
                        (default, default, packet_type_t.HARVEST7) : read1_action();
        }
        const default_action = NoAction;
    }
    apply {
        exponent_max.apply();
    }
}

control Processor(in value_t value0, in value_t value1, out value_t value0_out, out value_t value1_out, in switchml_md_h switchml_md) {
    Register<value_pair_t, pool_index_t>(register_size) values;
    RegisterAction<value_pair_t, pool_index_t, value_t>(values) write_read1_register_action = {
        void apply(inout value_pair_t value, out value_t read_value) {
            value.first = value0;
            value.second = value1;
            read_value = value.second;
        }
    };
    action write_read1_action() {
        value1_out = write_read1_register_action.execute(switchml_md.pool_index);
    }
    RegisterAction<value_pair_t, pool_index_t, value_t>(values) sum_read1_register_action = {
        void apply(inout value_pair_t value, out value_t read_value) {
            value.first = value.first + value0;
            value.second = value.second + value1;
            read_value = value.second;
        }
    };
    action sum_read1_action() {
        value1_out = sum_read1_register_action.execute(switchml_md.pool_index);
    }
    RegisterAction<value_pair_t, pool_index_t, value_t>(values) read0_register_action = {
        void apply(inout value_pair_t value, out value_t read_value) {
            read_value = value.first;
        }
    };
    action read0_action() {
        value0_out = read0_register_action.execute(switchml_md.pool_index);
    }
    RegisterAction<value_pair_t, pool_index_t, value_t>(values) read1_register_action = {
        void apply(inout value_pair_t value, out value_t read_value) {
            read_value = value.second;
        }
    };
    action read1_action() {
        value1_out = read1_register_action.execute(switchml_md.pool_index);
    }
    table sum {
        key = {
            switchml_md.worker_bitmap_before: ternary;
            switchml_md.map_result          : ternary;
            switchml_md.packet_type         : ternary;
        }
        actions = {
            write_read1_action;
            sum_read1_action;
            read0_action;
            read1_action;
            NoAction;
        }
        size = 20;
        const entries = {
                        (32w0, default, packet_type_t.CONSUME0) : write_read1_action();
                        (32w0, default, packet_type_t.CONSUME1) : write_read1_action();
                        (32w0, default, packet_type_t.CONSUME2) : write_read1_action();
                        (32w0, default, packet_type_t.CONSUME3) : write_read1_action();
                        (default, 32w0, packet_type_t.CONSUME0) : sum_read1_action();
                        (default, 32w0, packet_type_t.CONSUME1) : sum_read1_action();
                        (default, 32w0, packet_type_t.CONSUME2) : sum_read1_action();
                        (default, 32w0, packet_type_t.CONSUME3) : sum_read1_action();
                        (default, default, packet_type_t.CONSUME0) : read1_action();
                        (default, default, packet_type_t.CONSUME1) : read1_action();
                        (default, default, packet_type_t.CONSUME2) : read1_action();
                        (default, default, packet_type_t.CONSUME3) : read1_action();
                        (default, default, packet_type_t.HARVEST0) : read1_action();
                        (default, default, packet_type_t.HARVEST1) : read0_action();
                        (default, default, packet_type_t.HARVEST2) : read1_action();
                        (default, default, packet_type_t.HARVEST3) : read0_action();
                        (default, default, packet_type_t.HARVEST4) : read1_action();
                        (default, default, packet_type_t.HARVEST5) : read0_action();
                        (default, default, packet_type_t.HARVEST6) : read1_action();
                        (default, default, packet_type_t.HARVEST7) : read0_action();
        }
        const default_action = NoAction;
    }
    apply {
        sum.apply();
    }
}

control NextStepSelector(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    bool count_consume;
    bool count_broadcast;
    bool count_retransmit;
    bool count_recirculate;
    bool count_drop;
    Counter<counter_t, pool_index_t>(register_size, CounterType_t.PACKETS) broadcast_counter;
    Counter<counter_t, pool_index_t>(register_size, CounterType_t.PACKETS) retransmit_counter;
    Counter<counter_t, pool_index_t>(register_size, CounterType_t.PACKETS) recirculate_counter;
    Counter<counter_t, pool_index_t>(register_size, CounterType_t.PACKETS) drop_counter;
    action recirculate_for_consume(packet_type_t packet_type, PortId_t recirc_port) {
        hdr.d0.setInvalid();
        hdr.d1.setInvalid();
        ig_tm_md.ucast_egress_port = recirc_port;
        ig_tm_md.bypass_egress = 1w1;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        ig_md.switchml_md.packet_type = packet_type;
        count_consume = true;
        count_recirculate = true;
    }
    action recirculate_for_harvest(packet_type_t packet_type, PortId_t recirc_port) {
        ig_tm_md.ucast_egress_port = recirc_port;
        ig_tm_md.bypass_egress = 1w1;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        ig_md.switchml_md.packet_type = packet_type;
    }
    action recirculate_for_CONSUME1(PortId_t recirc_port) {
        recirculate_for_consume(packet_type_t.CONSUME1, recirc_port);
    }
    action recirculate_for_CONSUME2_same_port_next_pipe() {
        recirculate_for_consume(packet_type_t.CONSUME2, 2w2 ++ ig_intr_md.ingress_port[6:0]);
    }
    action recirculate_for_CONSUME3_same_port_next_pipe() {
        recirculate_for_consume(packet_type_t.CONSUME3, 2w3 ++ ig_intr_md.ingress_port[6:0]);
    }
    action recirculate_for_HARVEST1(PortId_t recirc_port) {
        hdr.d0.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST1, recirc_port);
    }
    action recirculate_for_HARVEST2(PortId_t recirc_port) {
        hdr.d1.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST2, recirc_port);
    }
    action recirculate_for_HARVEST3(PortId_t recirc_port) {
        hdr.d0.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST3, recirc_port);
    }
    action recirculate_for_HARVEST4(PortId_t recirc_port) {
        hdr.d1.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST4, recirc_port);
    }
    action recirculate_for_HARVEST5(PortId_t recirc_port) {
        hdr.d0.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST5, recirc_port);
    }
    action recirculate_for_HARVEST6(PortId_t recirc_port) {
        hdr.d1.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST6, recirc_port);
    }
    action recirculate_for_HARVEST7(PortId_t recirc_port) {
        hdr.d0.setInvalid();
        recirculate_for_harvest(packet_type_t.HARVEST7, recirc_port);
    }
    action finish_consume() {
        ig_dprsr_md.drop_ctl[0:0] = 1;
        count_consume = true;
        count_drop = true;
    }
    action broadcast() {
        hdr.d1.setInvalid();
        ig_tm_md.mcast_grp_a = ig_md.switchml_md.mgid;
        ig_tm_md.level1_exclusion_id = null_level1_exclusion_id;
        ig_md.switchml_md.packet_type = packet_type_t.BROADCAST;
        ig_tm_md.bypass_egress = 1w0;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        count_broadcast = true;
    }
    action retransmit() {
        hdr.d1.setInvalid();
        ig_tm_md.ucast_egress_port = ig_md.switchml_md.ingress_port;
        ig_md.switchml_md.packet_type = packet_type_t.RETRANSMIT;
        ig_tm_md.bypass_egress = 1w0;
        ig_dprsr_md.drop_ctl[0:0] = 0;
        count_retransmit = true;
    }
    action drop() {
        ig_dprsr_md.drop_ctl[0:0] = 1;
        ig_md.switchml_md.packet_type = packet_type_t.IGNORE;
        count_drop = true;
    }
    table next_step {
        key = {
            ig_md.switchml_md.packet_size    : ternary;
            ig_md.switchml_md.worker_id      : ternary;
            ig_md.switchml_md.packet_type    : ternary;
            ig_md.switchml_md.first_last_flag: ternary;
            ig_md.switchml_md.map_result     : ternary;
        }
        actions = {
            recirculate_for_CONSUME1;
            recirculate_for_CONSUME2_same_port_next_pipe;
            recirculate_for_CONSUME3_same_port_next_pipe;
            recirculate_for_HARVEST1;
            recirculate_for_HARVEST2;
            recirculate_for_HARVEST3;
            recirculate_for_HARVEST4;
            recirculate_for_HARVEST5;
            recirculate_for_HARVEST6;
            recirculate_for_HARVEST7;
            finish_consume;
            broadcast;
            retransmit;
            drop;
        }
        const default_action = drop();
        size = 128;
    }
    apply {
        count_consume = false;
        count_broadcast = false;
        count_retransmit = false;
        count_recirculate = false;
        count_drop = false;
        next_step.apply();
        if (count_consume || count_drop) {
            drop_counter.count(ig_md.switchml_md.pool_index);
        }
        if (count_recirculate) {
            recirculate_counter.count(ig_md.switchml_md.pool_index);
        }
        if (count_broadcast) {
            broadcast_counter.count(ig_md.switchml_md.pool_index);
        }
        if (count_retransmit) {
            retransmit_counter.count(ig_md.switchml_md.pool_index);
        }
    }
}

control Ingress(inout header_t hdr, inout ingress_metadata_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    ARPandICMPResponder() arp_icmp_responder;
    Forwarder() forwarder;
    IngressDropSimulator() ingress_drop_sim;
    EgressDropSimulator() egress_drop_sim;
    RDMAReceiver() rdma_receiver;
    UDPReceiver() udp_receiver;
    WorkersCounter() workers_counter;
    ReconstructWorkerBitmap() reconstruct_worker_bitmap;
    UpdateAndCheckWorkerBitmap() update_and_check_worker_bitmap;
    NextStepSelector() next_step_selector;
    Exponents() exponents;
    Processor() value00;
    Processor() value01;
    Processor() value02;
    Processor() value03;
    Processor() value04;
    Processor() value05;
    Processor() value06;
    Processor() value07;
    Processor() value08;
    Processor() value09;
    Processor() value10;
    Processor() value11;
    Processor() value12;
    Processor() value13;
    Processor() value14;
    Processor() value15;
    Processor() value16;
    Processor() value17;
    Processor() value18;
    Processor() value19;
    Processor() value20;
    Processor() value21;
    Processor() value22;
    Processor() value23;
    Processor() value24;
    Processor() value25;
    Processor() value26;
    Processor() value27;
    Processor() value28;
    Processor() value29;
    Processor() value30;
    Processor() value31;
    apply {
        if (ig_md.switchml_md.packet_type == packet_type_t.CONSUME0) {
            if (hdr.ib_bth.isValid()) {
                rdma_receiver.apply(hdr, ig_md, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
            } else {
                udp_receiver.apply(hdr, ig_md, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
            }
            ingress_drop_sim.apply(ig_md.port_metadata);
            egress_drop_sim.apply(ig_md.port_metadata, hdr.ib_bth.dst_qp[16 + max_num_workers_log2 - 1:16] ++ hdr.ib_bth.dst_qp[max_num_queue_pairs_per_worker_log2 - 1:0], ig_md.switchml_md.simulate_egress_drop);
            ig_md.switchml_md.ingress_port = ig_intr_md.ingress_port;
        } else if (ig_md.switchml_md.packet_type == packet_type_t.CONSUME1 || ig_md.switchml_md.packet_type == packet_type_t.CONSUME2 || ig_md.switchml_md.packet_type == packet_type_t.CONSUME3) {
            reconstruct_worker_bitmap.apply(ig_md);
        }
        if (ig_dprsr_md.drop_ctl[0:0] == 1w0) {
            if (ig_md.switchml_md.packet_type == packet_type_t.CONSUME0 || ig_md.switchml_md.packet_type == packet_type_t.CONSUME1 || ig_md.switchml_md.packet_type == packet_type_t.CONSUME2 || ig_md.switchml_md.packet_type == packet_type_t.CONSUME3) {
                update_and_check_worker_bitmap.apply(hdr, ig_md, ig_intr_md, ig_dprsr_md, ig_tm_md);
                workers_counter.apply(hdr, ig_md, ig_dprsr_md);
            }
            if ((packet_type_underlying_t)ig_md.switchml_md.packet_type >= (packet_type_underlying_t)packet_type_t.CONSUME0) {
                if (ig_md.switchml_md.last_packet) {
                    exponents.apply(ig_md.switchml_md.e0, ig_md.switchml_md.e1, ig_md.switchml_md.e0, ig_md.switchml_md.e1, ig_md);
                }
                value00.apply(hdr.d0.d00, hdr.d1.d00, hdr.d0.d00, hdr.d1.d00, ig_md.switchml_md);
                value01.apply(hdr.d0.d01, hdr.d1.d01, hdr.d0.d01, hdr.d1.d01, ig_md.switchml_md);
                value02.apply(hdr.d0.d02, hdr.d1.d02, hdr.d0.d02, hdr.d1.d02, ig_md.switchml_md);
                value03.apply(hdr.d0.d03, hdr.d1.d03, hdr.d0.d03, hdr.d1.d03, ig_md.switchml_md);
                value04.apply(hdr.d0.d04, hdr.d1.d04, hdr.d0.d04, hdr.d1.d04, ig_md.switchml_md);
                value05.apply(hdr.d0.d05, hdr.d1.d05, hdr.d0.d05, hdr.d1.d05, ig_md.switchml_md);
                value06.apply(hdr.d0.d06, hdr.d1.d06, hdr.d0.d06, hdr.d1.d06, ig_md.switchml_md);
                value07.apply(hdr.d0.d07, hdr.d1.d07, hdr.d0.d07, hdr.d1.d07, ig_md.switchml_md);
                value08.apply(hdr.d0.d08, hdr.d1.d08, hdr.d0.d08, hdr.d1.d08, ig_md.switchml_md);
                value09.apply(hdr.d0.d09, hdr.d1.d09, hdr.d0.d09, hdr.d1.d09, ig_md.switchml_md);
                value10.apply(hdr.d0.d10, hdr.d1.d10, hdr.d0.d10, hdr.d1.d10, ig_md.switchml_md);
                value11.apply(hdr.d0.d11, hdr.d1.d11, hdr.d0.d11, hdr.d1.d11, ig_md.switchml_md);
                value12.apply(hdr.d0.d12, hdr.d1.d12, hdr.d0.d12, hdr.d1.d12, ig_md.switchml_md);
                value13.apply(hdr.d0.d13, hdr.d1.d13, hdr.d0.d13, hdr.d1.d13, ig_md.switchml_md);
                value14.apply(hdr.d0.d14, hdr.d1.d14, hdr.d0.d14, hdr.d1.d14, ig_md.switchml_md);
                value15.apply(hdr.d0.d15, hdr.d1.d15, hdr.d0.d15, hdr.d1.d15, ig_md.switchml_md);
                value16.apply(hdr.d0.d16, hdr.d1.d16, hdr.d0.d16, hdr.d1.d16, ig_md.switchml_md);
                value17.apply(hdr.d0.d17, hdr.d1.d17, hdr.d0.d17, hdr.d1.d17, ig_md.switchml_md);
                value18.apply(hdr.d0.d18, hdr.d1.d18, hdr.d0.d18, hdr.d1.d18, ig_md.switchml_md);
                value19.apply(hdr.d0.d19, hdr.d1.d19, hdr.d0.d19, hdr.d1.d19, ig_md.switchml_md);
                value20.apply(hdr.d0.d20, hdr.d1.d20, hdr.d0.d20, hdr.d1.d20, ig_md.switchml_md);
                value21.apply(hdr.d0.d21, hdr.d1.d21, hdr.d0.d21, hdr.d1.d21, ig_md.switchml_md);
                value22.apply(hdr.d0.d22, hdr.d1.d22, hdr.d0.d22, hdr.d1.d22, ig_md.switchml_md);
                value23.apply(hdr.d0.d23, hdr.d1.d23, hdr.d0.d23, hdr.d1.d23, ig_md.switchml_md);
                value24.apply(hdr.d0.d24, hdr.d1.d24, hdr.d0.d24, hdr.d1.d24, ig_md.switchml_md);
                value25.apply(hdr.d0.d25, hdr.d1.d25, hdr.d0.d25, hdr.d1.d25, ig_md.switchml_md);
                value26.apply(hdr.d0.d26, hdr.d1.d26, hdr.d0.d26, hdr.d1.d26, ig_md.switchml_md);
                value27.apply(hdr.d0.d27, hdr.d1.d27, hdr.d0.d27, hdr.d1.d27, ig_md.switchml_md);
                value28.apply(hdr.d0.d28, hdr.d1.d28, hdr.d0.d28, hdr.d1.d28, ig_md.switchml_md);
                value29.apply(hdr.d0.d29, hdr.d1.d29, hdr.d0.d29, hdr.d1.d29, ig_md.switchml_md);
                value30.apply(hdr.d0.d30, hdr.d1.d30, hdr.d0.d30, hdr.d1.d30, ig_md.switchml_md);
                value31.apply(hdr.d0.d31, hdr.d1.d31, hdr.d0.d31, hdr.d1.d31, ig_md.switchml_md);
                next_step_selector.apply(hdr, ig_md, ig_intr_md, ig_dprsr_md, ig_tm_md);
            } else {
                arp_icmp_responder.apply(hdr, ig_md, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
                forwarder.apply(hdr, ig_md, ig_intr_md, ig_dprsr_md, ig_tm_md);
            }
        }
    }
}

control Egress(inout header_t hdr, inout egress_metadata_t eg_md, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md, inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    RDMASender() rdma_sender;
    UDPSender() udp_sender;
    apply {
        if (eg_md.switchml_md.packet_type == packet_type_t.BROADCAST || eg_md.switchml_md.packet_type == packet_type_t.RETRANSMIT) {
            if (eg_md.switchml_md.simulate_egress_drop) {
                eg_md.switchml_md.packet_type = packet_type_t.IGNORE;
                eg_intr_dprs_md.drop_ctl[0:0] = 1;
            }
            if (eg_md.switchml_md.packet_type == packet_type_t.BROADCAST) {
                eg_md.switchml_md.worker_id = eg_intr_md.egress_rid;
            }
            if (eg_md.switchml_md.worker_type == worker_type_t.ROCEv2) {
                rdma_sender.apply(hdr, eg_md, eg_intr_md, eg_intr_md_from_prsr, eg_intr_dprs_md);
            } else {
                udp_sender.apply(eg_md, eg_intr_md, hdr);
            }
        }
    }
}

Pipeline(IngressParser(), Ingress(), IngressDeparser(), EgressParser(), Egress(), EgressDeparser()) pipe;

Switch(pipe) main;

