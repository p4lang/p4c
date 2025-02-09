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
#include <tna.p4>

header bpv6_primary_cbhe_h {
    bit<8>  version;
    bit<16> bundle_flags;
    bit<8>  block_length;
    bit<8>  dst_node_num;
    bit<8>  dst_serv_num;
    bit<8>  src_node_num;
    bit<8>  src_serv_num;
    bit<8>  rep_node_num;
    bit<8>  rep_serv_num;
    bit<8>  cust_node_num;
    bit<8>  cust_serv_num;
    bit<40> creation_ts;
    bit<8>  creation_ts_seq_num;
    bit<24> lifetime;
    bit<8>  dict_len;
}

header bpv6_extension_phib_h {
    bit<8>  block_type_code;
    bit<8>  block_flags;
    bit<8>  block_data_len;
    bit<32> prev_hop_scheme_name;
    bit<32> prev_hop;
}

header bpv6_extension_age_h {
    bit<8> block_type_code;
    bit<8> block_flags;
    bit<8> block_data_len;
    bit<8> bundle_age;
}

header bpv6_payload_h {
    bit<8> block_type_code;
    bit<8> block_flags;
    bit<8> payload_length;
}

const bit<8> CBOR_INDEF_LEN_ARRAY_START_CODE = 8w0x9f;
const bit<8> CBOR_INDEF_LEN_ARRAY_STOP_CODE = 8w0xff;
const bit<8> CBOR_MASK_MT = 8w0b11100000;
const bit<8> CBOR_MASK_AI = 8w0b11111;
header bpv7_start_code_h {
    bit<8> start_code;
}

header bpv7_primary_1_h {
    bit<8>  prim_initial_byte;
    bit<8>  version_num;
    bit<16> bundle_flags;
    bit<8>  crc_type;
    bit<40> dest_eid;
    bit<40> src_eid;
    bit<40> report_eid;
    bit<8>  creation_timestamp_time_initial_byte;
    bit<72> creation_timestamp_time;
    bit<8>  creation_timestamp_seq_num_initial_byte;
}

header bpv7_primary_2_1_h {
    bit<8> creation_timestamp_seq_num;
}

header bpv7_primary_2_2_h {
    bit<16> creation_timestamp_seq_num;
}

header bpv7_primary_2_4_h {
    bit<32> creation_timestamp_seq_num;
}

header bpv7_primary_2_8_h {
    bit<64> creation_timestamp_seq_num;
}

header bpv7_primary_3_h {
    bit<40> lifetime;
    bit<24> crc_field_integer;
}

header bpv7_extension_prev_node_h {
    bit<8> initial_byte;
    bit<8> block_type;
    bit<8> block_num;
    bit<8> block_flags;
    bit<8> crc_type;
    bit<8> block_data_initial_byte;
    bit<8> prev_node_array_initial_byte;
    bit<8> uri_scheme;
    bit<8> prev_node_eid_initial_byte;
    bit<8> node_num;
    bit<8> serv_num;
}

header bpv7_extension_ecos_h {
    bit<8>  initial_byte;
    bit<16> block_type;
    bit<8>  block_num;
    bit<8>  block_flags;
    bit<8>  crc_type;
    bit<8>  block_data_initial_byte;
    bit<8>  ecos_array_start;
    bit<32> ecos_data;
}

header bpv7_extension_age_h {
    bit<8> initial_byte;
    bit<8> block_type;
    bit<8> block_num;
    bit<8> block_flags;
    bit<8> crc_type;
    bit<8> block_data_initial_byte;
    bit<8> bundle_age;
}

header bpv7_payload_h {
    bit<8> initial_byte;
    bit<8> block_type;
    bit<8> block_num;
    bit<8> block_flags;
    bit<8> crc_type;
    bit<8> adu_initial_byte;
}

header bpv7_stop_code_h {
    bit<8> stop_code;
}

header adu_1_h {
    bit<8> adu;
}

header adu_2_h {
    bit<16> adu;
}

header adu_3_h {
    bit<24> adu;
}

header adu_4_h {
    bit<32> adu;
}

header adu_5_h {
    bit<40> adu;
}

header adu_6_h {
    bit<48> adu;
}

header adu_7_h {
    bit<56> adu;
}

header adu_8_h {
    bit<64> adu;
}

header adu_9_h {
    bit<72> adu;
}

header adu_10_h {
    bit<80> adu;
}

header adu_11_h {
    bit<88> adu;
}

header adu_12_h {
    bit<96> adu;
}

header adu_13_h {
    bit<104> adu;
}

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x800;
const bit<3> DIGEST_TYPE_DEBUG = 0x1;
header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16>    ether_type;
}

header ipv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> hdr_length;
    bit<16> checksum;
}

struct headers_t {
    ethernet_h                 ethernet;
    ipv4_h                     ipv4;
    udp_h                      udp;
    bpv6_primary_cbhe_h        bpv6_primary_cbhe;
    bpv6_extension_phib_h      bpv6_extension_phib;
    bpv6_extension_age_h       bpv6_extension_age;
    bpv6_payload_h             bpv6_payload;
    bpv7_start_code_h          bpv7_start_code;
    bpv7_primary_1_h           bpv7_primary_1;
    bpv7_primary_2_1_h         bpv7_primary_2_1;
    bpv7_primary_2_2_h         bpv7_primary_2_2;
    bpv7_primary_2_4_h         bpv7_primary_2_4;
    bpv7_primary_2_8_h         bpv7_primary_2_8;
    bpv7_primary_3_h           bpv7_primary_3;
    bpv7_extension_prev_node_h bpv7_extension_prev_node;
    bpv7_extension_ecos_h      bpv7_extension_ecos;
    bpv7_extension_age_h       bpv7_extension_age;
    bpv7_payload_h             bpv7_payload;
    adu_1_h                    adu_1;
    adu_2_h                    adu_2;
    adu_3_h                    adu_3;
    adu_4_h                    adu_4;
    adu_5_h                    adu_5;
    adu_6_h                    adu_6;
    adu_7_h                    adu_7;
    adu_8_h                    adu_8;
    adu_9_h                    adu_9;
    adu_10_h                   adu_10;
    adu_11_h                   adu_11;
    adu_12_h                   adu_12;
    adu_13_h                   adu_13;
    bpv7_stop_code_h           bpv7_stop_code;
}

struct debug_digest_t {
    bit<8> hdr_version_num;
    bit<8> initial_byte;
    bit<8> block_type;
    bit<8> block_num;
    bit<8> block_flags;
    bit<8> crc_type;
    bit<8> block_data_initial_byte;
    bit<8> bundle_age;
}

struct metadata_headers_t {
    bool           checksum_upd_ipv4;
    bool           checksum_upd_udp;
    bool           checksum_err_ipv4_igprs;
    bool           incomingV7;
    bool           incomingV6;
    debug_digest_t debug_metadata;
}

parser IngressParser(packet_in pkt, out headers_t hdr, out metadata_headers_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() ipv4_checksum;
    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1: parse_resubmit;
            0: parse_port_metadata;
        }
    }
    state parse_resubmit {
        transition reject;
    }
    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        meta.checksum_err_ipv4_igprs = ipv4_checksum.verify();
        transition select(hdr.ipv4.protocol) {
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition parse_version;
    }
    state parse_version {
        bit<8> start_byte = pkt.lookahead<bit<8>>();
        transition select(start_byte) {
            0x6: parse_v6;
            0x9f: parse_v7;
            default: accept;
        }
    }
    state parse_v6 {
        meta.incomingV6 = true;
        pkt.extract(hdr.bpv6_primary_cbhe);
        pkt.extract(hdr.bpv6_extension_phib);
        pkt.extract(hdr.bpv6_extension_age);
        pkt.extract(hdr.bpv6_payload);
        transition select(hdr.bpv6_payload.payload_length) {
            1: parse_v6_adu_1;
            2: parse_v6_adu_2;
            3: parse_v6_adu_3;
            4: parse_v6_adu_4;
            5: parse_v6_adu_5;
            6: parse_v6_adu_6;
            7: parse_v6_adu_7;
            8: parse_v6_adu_8;
            9: parse_v6_adu_9;
            10: parse_v6_adu_10;
            11: parse_v6_adu_11;
            12: parse_v6_adu_12;
            13: parse_v6_adu_13;
            default: accept;
        }
    }
    state parse_v6_adu_1 {
        pkt.extract(hdr.adu_1);
        transition accept;
    }
    state parse_v6_adu_2 {
        pkt.extract(hdr.adu_2);
        transition accept;
    }
    state parse_v6_adu_3 {
        pkt.extract(hdr.adu_3);
        transition accept;
    }
    state parse_v6_adu_4 {
        pkt.extract(hdr.adu_4);
        transition accept;
    }
    state parse_v6_adu_5 {
        pkt.extract(hdr.adu_5);
        transition accept;
    }
    state parse_v6_adu_6 {
        pkt.extract(hdr.adu_6);
        transition accept;
    }
    state parse_v6_adu_7 {
        pkt.extract(hdr.adu_7);
        transition accept;
    }
    state parse_v6_adu_8 {
        pkt.extract(hdr.adu_8);
        transition accept;
    }
    state parse_v6_adu_9 {
        pkt.extract(hdr.adu_9);
        transition accept;
    }
    state parse_v6_adu_10 {
        pkt.extract(hdr.adu_10);
        transition accept;
    }
    state parse_v6_adu_11 {
        pkt.extract(hdr.adu_11);
        transition accept;
    }
    state parse_v6_adu_12 {
        pkt.extract(hdr.adu_12);
        transition accept;
    }
    state parse_v6_adu_13 {
        pkt.extract(hdr.adu_13);
        transition accept;
    }
    state parse_v7 {
        meta.incomingV7 = true;
        pkt.extract(hdr.bpv7_start_code);
        transition parse_v7_prim_block;
    }
    state parse_v7_prim_block {
        pkt.extract(hdr.bpv7_primary_1);
        transition select(hdr.bpv7_primary_1.creation_timestamp_seq_num_initial_byte) {
            24 &&& CBOR_MASK_AI: parse_v7_prim_block_2_1;
            25 &&& CBOR_MASK_AI: parse_v7_prim_block_2_2;
            26 &&& CBOR_MASK_AI: parse_v7_prim_block_2_4;
            27 &&& CBOR_MASK_AI: parse_v7_prim_block_2_8;
            default: parse_v7_prim_block_3;
        }
    }
    state parse_v7_prim_block_2_1 {
        pkt.extract(hdr.bpv7_primary_2_1);
        transition parse_v7_prim_block_3;
    }
    state parse_v7_prim_block_2_2 {
        pkt.extract(hdr.bpv7_primary_2_2);
        transition parse_v7_prim_block_3;
    }
    state parse_v7_prim_block_2_4 {
        pkt.extract(hdr.bpv7_primary_2_4);
        transition parse_v7_prim_block_3;
    }
    state parse_v7_prim_block_2_8 {
        pkt.extract(hdr.bpv7_primary_2_8);
        transition parse_v7_prim_block_3;
    }
    state parse_v7_prim_block_3 {
        pkt.extract(hdr.bpv7_primary_3);
        transition parse_v7_prev_node_block;
    }
    state parse_v7_prev_node_block {
        pkt.extract(hdr.bpv7_extension_prev_node);
        transition parse_v7_ecos_block;
    }
    state parse_v7_ecos_block {
        pkt.extract(hdr.bpv7_extension_ecos);
        transition parse_v7_age_block;
    }
    state parse_v7_age_block {
        pkt.extract(hdr.bpv7_extension_age);
        transition parse_v7_payload_header;
    }
    state parse_v7_payload_header {
        pkt.extract(hdr.bpv7_payload);
        transition select(hdr.bpv7_payload.adu_initial_byte) {
            1 &&& CBOR_MASK_AI: parse_v7_adu_1;
            2 &&& CBOR_MASK_AI: parse_v7_adu_2;
            3 &&& CBOR_MASK_AI: parse_v7_adu_3;
            4 &&& CBOR_MASK_AI: parse_v7_adu_4;
            5 &&& CBOR_MASK_AI: parse_v7_adu_5;
            6 &&& CBOR_MASK_AI: parse_v7_adu_6;
            7 &&& CBOR_MASK_AI: parse_v7_adu_7;
            8 &&& CBOR_MASK_AI: parse_v7_adu_8;
            9 &&& CBOR_MASK_AI: parse_v7_adu_9;
            10 &&& CBOR_MASK_AI: parse_v7_adu_10;
            11 &&& CBOR_MASK_AI: parse_v7_adu_11;
            12 &&& CBOR_MASK_AI: parse_v7_adu_12;
            13 &&& CBOR_MASK_AI: parse_v7_adu_13;
            default: accept;
        }
    }
    state parse_v7_adu_1 {
        pkt.extract(hdr.adu_1);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_2 {
        pkt.extract(hdr.adu_2);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_3 {
        pkt.extract(hdr.adu_3);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_4 {
        pkt.extract(hdr.adu_4);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_5 {
        pkt.extract(hdr.adu_5);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_6 {
        pkt.extract(hdr.adu_6);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_7 {
        pkt.extract(hdr.adu_7);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_8 {
        pkt.extract(hdr.adu_8);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_9 {
        pkt.extract(hdr.adu_9);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_10 {
        pkt.extract(hdr.adu_10);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_11 {
        pkt.extract(hdr.adu_11);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_12 {
        pkt.extract(hdr.adu_12);
        transition parse_v7_stop_code;
    }
    state parse_v7_adu_13 {
        pkt.extract(hdr.adu_13);
        transition parse_v7_stop_code;
    }
    state parse_v7_stop_code {
        pkt.extract(hdr.bpv7_stop_code);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_headers_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    action checksum_upd_ipv4(bool update) {
        meta.checksum_upd_ipv4 = update;
    }
    action checksum_upd_udp(bool update) {
        meta.checksum_upd_udp = update;
    }
    action send(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }
    action drop() {
        ig_dprsr_md.drop_ctl = 1;
    }
    table ipv4_host {
        key = {
            hdr.ipv4.dst_addr: exact;
        }
        actions = {
            send;
            drop;
        }
        size = 65536;
    }
    apply {
        ig_dprsr_md.digest_type = DIGEST_TYPE_DEBUG;
        hdr.ipv4.ttl = 1;
        if (hdr.ipv4.isValid()) {
            hdr.ipv4.ttl = 2;
            if (hdr.udp.isValid()) {
                hdr.ipv4.ttl = 3;
                if (meta.incomingV7) {
                    if (hdr.bpv7_start_code.isValid()) {
                        hdr.ipv4.ttl = 4;
                        if (hdr.bpv7_primary_1.isValid()) {
                            hdr.ipv4.ttl = 5;
                            meta.debug_metadata.hdr_version_num = hdr.bpv7_primary_1.version_num;
                            if (hdr.bpv7_primary_2_1.isValid()) {
                                hdr.ipv4.ttl = 6;
                            } else if (hdr.bpv7_primary_2_2.isValid()) {
                                hdr.ipv4.ttl = 7;
                            } else if (hdr.bpv7_primary_2_4.isValid()) {
                                hdr.ipv4.ttl = 8;
                            } else if (hdr.bpv7_primary_2_8.isValid()) {
                                hdr.ipv4.ttl = 9;
                            }
                            if (hdr.bpv7_payload.isValid()) {
                                hdr.ipv4.ttl = 14;
                                hdr.ipv4.identification = hdr.bpv7_extension_ecos.block_data_initial_byte ++ hdr.bpv7_extension_ecos.ecos_array_start;
                                meta.debug_metadata.initial_byte = hdr.bpv7_extension_age.initial_byte;
                                meta.debug_metadata.block_type = hdr.bpv7_extension_age.block_type;
                                meta.debug_metadata.block_num = hdr.bpv7_extension_age.block_num;
                                meta.debug_metadata.block_flags = hdr.bpv7_extension_age.block_flags;
                                meta.debug_metadata.crc_type = hdr.bpv7_extension_age.crc_type;
                                meta.debug_metadata.block_data_initial_byte = hdr.bpv7_extension_age.block_data_initial_byte;
                                meta.debug_metadata.bundle_age = hdr.bpv7_extension_age.bundle_age;
                                if (hdr.adu_1.isValid()) {
                                    hdr.ipv4.ttl = 15;
                                } else if (hdr.adu_2.isValid()) {
                                    hdr.ipv4.ttl = 16;
                                } else if (hdr.adu_3.isValid()) {
                                    hdr.ipv4.ttl = 17;
                                } else if (hdr.adu_4.isValid()) {
                                    hdr.ipv4.ttl = 18;
                                } else if (hdr.adu_5.isValid()) {
                                    hdr.ipv4.ttl = 19;
                                } else if (hdr.adu_6.isValid()) {
                                    hdr.ipv4.ttl = 20;
                                } else if (hdr.adu_7.isValid()) {
                                    hdr.ipv4.ttl = 21;
                                } else if (hdr.adu_8.isValid()) {
                                    hdr.ipv4.ttl = 22;
                                } else if (hdr.adu_9.isValid()) {
                                    hdr.ipv4.ttl = 23;
                                } else if (hdr.adu_10.isValid()) {
                                    hdr.ipv4.ttl = 24;
                                } else if (hdr.adu_11.isValid()) {
                                    hdr.ipv4.ttl = 25;
                                } else if (hdr.adu_12.isValid()) {
                                    hdr.ipv4.ttl = 26;
                                } else if (hdr.adu_13.isValid()) {
                                    hdr.ipv4.ttl = 27;
                                }
                                hdr.bpv6_primary_cbhe.version = 6;
                                hdr.bpv6_primary_cbhe.bundle_flags = 0x8110;
                                hdr.bpv6_primary_cbhe.block_length = 18;
                                hdr.bpv6_primary_cbhe.dst_node_num = hdr.bpv7_primary_1.dest_eid[15:8];
                                hdr.bpv6_primary_cbhe.dst_serv_num = hdr.bpv7_primary_1.dest_eid[7:0];
                                hdr.bpv6_primary_cbhe.src_node_num = hdr.bpv7_primary_1.src_eid[15:8];
                                hdr.bpv6_primary_cbhe.src_serv_num = hdr.bpv7_primary_1.src_eid[7:0];
                                hdr.bpv6_primary_cbhe.rep_node_num = hdr.bpv7_primary_1.report_eid[15:8];
                                hdr.bpv6_primary_cbhe.rep_serv_num = hdr.bpv7_primary_1.report_eid[7:0];
                                hdr.bpv6_primary_cbhe.cust_node_num = 0;
                                hdr.bpv6_primary_cbhe.cust_serv_num = 0;
                                hdr.bpv6_primary_cbhe.creation_ts = 0x82d2e2cc6e;
                                hdr.bpv6_primary_cbhe.creation_ts_seq_num = 1;
                                hdr.bpv6_primary_cbhe.lifetime = 0x85a300;
                                hdr.bpv6_primary_cbhe.dict_len = 0;
                                hdr.bpv6_primary_cbhe.setValid();
                                hdr.bpv6_payload.block_type_code = 1;
                                hdr.bpv6_payload.block_flags = 0x9;
                                if (hdr.adu_1.isValid()) {
                                    hdr.bpv6_payload.payload_length = 1;
                                } else if (hdr.adu_2.isValid()) {
                                    hdr.bpv6_payload.payload_length = 2;
                                } else if (hdr.adu_3.isValid()) {
                                    hdr.bpv6_payload.payload_length = 3;
                                } else if (hdr.adu_4.isValid()) {
                                    hdr.bpv6_payload.payload_length = 4;
                                } else if (hdr.adu_5.isValid()) {
                                    hdr.bpv6_payload.payload_length = 5;
                                } else if (hdr.adu_6.isValid()) {
                                    hdr.bpv6_payload.payload_length = 6;
                                } else if (hdr.adu_7.isValid()) {
                                    hdr.bpv6_payload.payload_length = 7;
                                } else if (hdr.adu_8.isValid()) {
                                    hdr.bpv6_payload.payload_length = 8;
                                } else if (hdr.adu_9.isValid()) {
                                    hdr.bpv6_payload.payload_length = 9;
                                } else if (hdr.adu_10.isValid()) {
                                    hdr.bpv6_payload.payload_length = 10;
                                } else if (hdr.adu_11.isValid()) {
                                    hdr.bpv6_payload.payload_length = 11;
                                } else if (hdr.adu_12.isValid()) {
                                    hdr.bpv6_payload.payload_length = 12;
                                } else if (hdr.adu_13.isValid()) {
                                    hdr.bpv6_payload.payload_length = 13;
                                }
                                hdr.bpv6_payload.setValid();
                                bit<8> udp_len = 8 + 22 + 3 + hdr.bpv6_payload.payload_length;
                                hdr.udp.hdr_length = (bit<16>)udp_len;
                                hdr.ipv4.total_len = (bit<16>)(20 + 8 + 22 + 3 + hdr.bpv6_payload.payload_length);
                                hdr.bpv7_start_code.setInvalid();
                                hdr.bpv7_primary_1.setInvalid();
                                hdr.bpv7_primary_2_1.setInvalid();
                                hdr.bpv7_primary_2_2.setInvalid();
                                hdr.bpv7_primary_2_4.setInvalid();
                                hdr.bpv7_primary_2_8.setInvalid();
                                hdr.bpv7_primary_3.setInvalid();
                                hdr.bpv7_extension_prev_node.setInvalid();
                                hdr.bpv7_extension_ecos.setInvalid();
                                hdr.bpv7_extension_age.setInvalid();
                                hdr.bpv7_payload.setInvalid();
                                hdr.bpv7_stop_code.setInvalid();
                            }
                        }
                    }
                } else if (meta.incomingV6) {
                    hdr.ipv4.ttl = 32;
                    if (hdr.bpv6_primary_cbhe.isValid()) {
                        hdr.ipv4.ttl = 33;
                        meta.debug_metadata.hdr_version_num = hdr.bpv6_primary_cbhe.version;
                        if (hdr.bpv6_extension_phib.isValid()) {
                            hdr.ipv4.ttl = 34;
                            if (hdr.bpv6_extension_age.isValid()) {
                                hdr.ipv4.ttl = 35;
                                if (hdr.bpv6_payload.isValid()) {
                                    if (hdr.adu_1.isValid()) {
                                        hdr.ipv4.ttl = 36;
                                    } else if (hdr.adu_2.isValid()) {
                                        hdr.ipv4.ttl = 37;
                                    } else if (hdr.adu_3.isValid()) {
                                        hdr.ipv4.ttl = 38;
                                    } else if (hdr.adu_4.isValid()) {
                                        hdr.ipv4.ttl = 39;
                                    } else if (hdr.adu_5.isValid()) {
                                        hdr.ipv4.ttl = 40;
                                    } else if (hdr.adu_6.isValid()) {
                                        hdr.ipv4.ttl = 41;
                                    } else if (hdr.adu_7.isValid()) {
                                        hdr.ipv4.ttl = 42;
                                    } else if (hdr.adu_8.isValid()) {
                                        hdr.ipv4.ttl = 43;
                                    } else if (hdr.adu_9.isValid()) {
                                        hdr.ipv4.ttl = 44;
                                    } else if (hdr.adu_10.isValid()) {
                                        hdr.ipv4.ttl = 45;
                                    } else if (hdr.adu_11.isValid()) {
                                        hdr.ipv4.ttl = 46;
                                    } else if (hdr.adu_12.isValid()) {
                                        hdr.ipv4.ttl = 47;
                                    } else if (hdr.adu_13.isValid()) {
                                        hdr.ipv4.ttl = 48;
                                    }
                                    hdr.bpv7_start_code.start_code = CBOR_INDEF_LEN_ARRAY_START_CODE;
                                    hdr.bpv7_start_code.setValid();
                                    hdr.bpv7_primary_1.prim_initial_byte = 0x89;
                                    hdr.bpv7_primary_1.version_num = 0x7;
                                    hdr.bpv7_primary_1.bundle_flags = 0x1840;
                                    hdr.bpv7_primary_1.crc_type = 0x1;
                                    hdr.bpv7_primary_1.dest_eid = 24w0x820282 ++ hdr.bpv6_primary_cbhe.dst_node_num ++ hdr.bpv6_primary_cbhe.dst_serv_num;
                                    hdr.bpv7_primary_1.src_eid = 24w0x820282 ++ hdr.bpv6_primary_cbhe.src_node_num ++ hdr.bpv6_primary_cbhe.src_serv_num;
                                    hdr.bpv7_primary_1.report_eid = 24w0x820282 ++ hdr.bpv6_primary_cbhe.rep_node_num ++ hdr.bpv6_primary_cbhe.rep_serv_num;
                                    hdr.bpv7_primary_1.creation_timestamp_time_initial_byte = 0x82;
                                    hdr.bpv7_primary_1.creation_timestamp_time = 0x1b000000a56f9117c4;
                                    hdr.bpv7_primary_1.creation_timestamp_seq_num_initial_byte = 0x1;
                                    hdr.bpv7_primary_1.setValid();
                                    hdr.bpv7_primary_3.lifetime = 0x1a05265c00;
                                    hdr.bpv7_primary_3.crc_field_integer = 24w0x420d79;
                                    hdr.bpv7_primary_3.setValid();
                                    hdr.bpv7_payload.initial_byte = 0x85;
                                    hdr.bpv7_payload.block_type = 0x1;
                                    hdr.bpv7_payload.block_num = 0x1;
                                    hdr.bpv7_payload.block_flags = 0x1;
                                    hdr.bpv7_payload.crc_type = 0x0;
                                    hdr.bpv7_payload.adu_initial_byte = 4w0x4 ++ hdr.bpv6_payload.payload_length[3:0];
                                    hdr.bpv7_payload.setValid();
                                    hdr.bpv7_stop_code.stop_code = CBOR_INDEF_LEN_ARRAY_STOP_CODE;
                                    hdr.bpv7_stop_code.setValid();
                                    bit<8> udp_len = 54 + hdr.bpv6_payload.payload_length;
                                    hdr.udp.hdr_length = (bit<16>)udp_len;
                                    hdr.ipv4.total_len = (bit<16>)(20 + 54 + hdr.bpv6_payload.payload_length);
                                    hdr.bpv6_primary_cbhe.setInvalid();
                                    hdr.bpv6_extension_phib.setInvalid();
                                    hdr.bpv6_extension_age.setInvalid();
                                    hdr.bpv6_payload.setInvalid();
                                }
                            }
                        }
                    }
                }
                checksum_upd_udp(true);
            }
            checksum_upd_ipv4(true);
            ipv4_host.apply();
        }
    }
}

control IngressDeparser(packet_out pkt, inout headers_t hdr, in metadata_headers_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    Checksum() ipv4_checksum;
    Checksum() udp_checksum;
    Digest<debug_digest_t>() debug_digest;
    apply {
        if (ig_dprsr_md.digest_type == DIGEST_TYPE_DEBUG) {
            debug_digest.pack(meta.debug_metadata);
        }
        if (meta.checksum_upd_ipv4) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.src_addr, hdr.ipv4.dst_addr });
        }
        if (meta.checksum_upd_udp) {
            if (meta.incomingV7) {
                hdr.udp.checksum = udp_checksum.update(data = { hdr.ipv4.src_addr, hdr.ipv4.dst_addr, 8w0, hdr.ipv4.protocol, hdr.udp.hdr_length, hdr.udp.src_port, hdr.udp.dst_port, hdr.udp.hdr_length, hdr.bpv6_primary_cbhe.version, hdr.bpv6_primary_cbhe.bundle_flags, hdr.bpv6_primary_cbhe.block_length, hdr.bpv6_primary_cbhe.dst_node_num, hdr.bpv6_primary_cbhe.dst_serv_num, hdr.bpv6_primary_cbhe.src_node_num, hdr.bpv6_primary_cbhe.src_serv_num, hdr.bpv6_primary_cbhe.rep_node_num, hdr.bpv6_primary_cbhe.rep_serv_num, hdr.bpv6_primary_cbhe.cust_node_num, hdr.bpv6_primary_cbhe.cust_serv_num, hdr.bpv6_primary_cbhe.creation_ts, hdr.bpv6_primary_cbhe.creation_ts_seq_num, hdr.bpv6_primary_cbhe.lifetime, hdr.bpv6_primary_cbhe.dict_len, hdr.bpv6_payload.block_type_code, hdr.bpv6_payload.block_flags, hdr.bpv6_payload.payload_length, hdr.adu_1.adu, 8w0, hdr.adu_2.adu, hdr.adu_3.adu, 8w0, hdr.adu_4.adu, hdr.adu_5.adu, 8w0, hdr.adu_6.adu, hdr.adu_7.adu, 8w0, hdr.adu_8.adu, hdr.adu_9.adu, 8w0, hdr.adu_10.adu, hdr.adu_11.adu, 8w0, hdr.adu_12.adu, hdr.adu_13.adu }, zeros_as_ones = true);
            } else if (meta.incomingV6) {
                hdr.udp.checksum = udp_checksum.update(data = { hdr.ipv4.src_addr, hdr.ipv4.dst_addr, 8w0, hdr.ipv4.protocol, hdr.udp.hdr_length, hdr.udp.src_port, hdr.udp.dst_port, hdr.udp.hdr_length, hdr.bpv7_start_code.start_code, hdr.bpv7_primary_1.prim_initial_byte, hdr.bpv7_primary_1.version_num, hdr.bpv7_primary_1.bundle_flags, hdr.bpv7_primary_1.crc_type, hdr.bpv7_primary_1.dest_eid, hdr.bpv7_primary_1.src_eid, hdr.bpv7_primary_1.report_eid, hdr.bpv7_primary_1.creation_timestamp_time_initial_byte, hdr.bpv7_primary_1.creation_timestamp_time, hdr.bpv7_primary_1.creation_timestamp_seq_num_initial_byte, hdr.bpv7_primary_3.lifetime, hdr.bpv7_primary_3.crc_field_integer, hdr.bpv7_payload.initial_byte, hdr.bpv7_payload.block_type, hdr.bpv7_payload.block_num, hdr.bpv7_payload.block_flags, hdr.bpv7_payload.crc_type, hdr.bpv7_payload.adu_initial_byte, hdr.adu_1.adu, 8w0, hdr.adu_2.adu, hdr.adu_3.adu, 8w0, hdr.adu_4.adu, hdr.adu_5.adu, 8w0, hdr.adu_6.adu, hdr.adu_7.adu, 8w0, hdr.adu_8.adu, hdr.adu_9.adu, 8w0, hdr.adu_10.adu, hdr.adu_11.adu, 8w0, hdr.adu_12.adu, hdr.adu_13.adu }, zeros_as_ones = true);
            }
        }
        pkt.emit(hdr);
    }
}

struct egress_headers_t {
}

struct egress_metadata_t {
}

parser EgressParser(packet_in pkt, out egress_headers_t hdr, out egress_metadata_t meta, out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

control Egress(inout egress_headers_t hdr, inout egress_metadata_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md, inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) {
    apply {
    }
}

control EgressDeparser(packet_out pkt, inout egress_headers_t hdr, in egress_metadata_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
    apply {
        pkt.emit(hdr);
    }
}

Pipeline(IngressParser(), Ingress(), IngressDeparser(), EgressParser(), Egress(), EgressDeparser()) pipe;
Switch(pipe) main;
