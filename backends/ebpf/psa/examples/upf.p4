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
 * This P4 program implements very simple 5G User Plane Function (UPF) that includes the following functionality:
 * - checksum verification & calculation
 * - L3 routing
 * - PFCP classification based on Packet Detection Rules (PDRs)
 * - GTP tunneling based on Forwarding Action Rules (FARs)
 */

#include <core.p4>
#include <psa.p4>

const bit<16> TYPE_IPV4 = 0x800;
const bit<8> PROTO_ICMP = 1;
const bit<8> PROTO_TCP = 6;
const bit<8> PROTO_UDP = 17;

#define UDP_PORT_GTPU 2152
#define GTP_GPDU 0xff
#define GTPU_VERSION 0x01
#define GTP_PROTOCOL_TYPE_GTP 0x01

typedef bit<4> destination_t;
const destination_t ACCESS = 4w0;
const destination_t CORE = 4w1;
const destination_t SGi_LAN = 4w2;
const destination_t CP_FUNCTION = 4w3;

#define IP_VERSION_4 4
const bit<8> DEFAULT_IPV4_TTL = 64;
const bit<4> IPV4_MIN_IHL = 5;

#define ETH_HDR_SIZE 14
#define IPV4_HDR_SIZE 20
#define UDP_HDR_SIZE 8
#define GTP_HDR_SIZE 8

action nop() {
    NoAction();
}

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}


header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<6>    dscp;
    bit<2>    ecn;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
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

header icmp_t {
    bit<8> icmp_type;
    bit<8> icmp_code;
    bit<16> checksum;
    bit<16> identifier;
    bit<16> sequence_number;
    bit<64> timestamp;
}

header gtpu_t {
    bit<3>  version;    /* version */
    bit<1>  pt;         /* protocol type */
    bit<1>  spare;      /* reserved */
    bit<1>  ex_flag;    /* next extension hdr present? */
    bit<1>  seq_flag;   /* sequence no. */
    bit<1>  npdu_flag;  /* n-pdn number present ? */
    bit<8>  msgtype;    /* message type */
    bit<16> msglen;     /* message length */
    bit<32> teid;       /* tunnel endpoint id */
}

struct upf_meta_t {
    bit<64>           seid;
    bit<32>           far_id;
    destination_t     src;
    destination_t     dest;
    ip4Addr_t         outer_dst_addr;
    bit<16>           l4_sport;
    bit<16>           l4_dport;
    bit<8>            src_port_range_id;
    bit<8>            dst_port_range_id;
    bit<16>           ipv4_len;
    bit<32>           teid;
    bit<32>           gtpu_remote_ip;
    bit<32>           gtpu_local_ip;
}

struct metadata {
    upf_meta_t upf;
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t gtpu_ipv4;
    udp_t gtpu_udp;
    gtpu_t gtpu;
    ipv4_t inner_ipv4;
    udp_t inner_udp;
    ipv4_t ipv4;
    tcp_t tcp;
    udp_t udp;
    icmp_t icmp;
}

struct empty_t {}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata user_meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_t resubmit_meta,
                         in empty_t recirculate_meta) {
    InternetChecksum() ck;
    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        ck.clear();
        ck.subtract(hdr.ipv4.hdrChecksum);
        ck.subtract({/* 16-bit word */ hdr.ipv4.ttl, hdr.ipv4.protocol });
        hdr.ipv4.hdrChecksum = ck.get();
        transition select(hdr.ipv4.protocol) {
            PROTO_UDP: parse_udp;
            PROTO_TCP: parse_tcp;
            PROTO_ICMP: accept;
        }
    }

    state parse_udp {
        packet.extract(hdr.udp);
        transition select(hdr.udp.dport) {
             UDP_PORT_GTPU: parse_gtpu;
             default: accept;
        }
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }

    state parse_gtpu {
        packet.extract(hdr.gtpu);
        transition parse_inner_ipv4;
    }

    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        ck.clear();
        ck.subtract(hdr.inner_ipv4.hdrChecksum);
        ck.subtract({/* 16-bit word */ hdr.ipv4.ttl, hdr.ipv4.protocol });
        hdr.inner_ipv4.hdrChecksum = ck.get();
        transition select(hdr.inner_ipv4.protocol) {
            PROTO_UDP: parse_inner_udp;
            PROTO_TCP: parse_tcp;
            PROTO_ICMP: accept;
        }
    }

    state parse_inner_udp {
        packet.extract(hdr.inner_udp);
        transition accept;
    }
}

control upf_process_ingress_l4port(inout headers hdr, inout metadata meta) {
    action set_ingress_dst_port_range_id(bit<8> range_id) {
        meta.upf.dst_port_range_id = range_id;
    }
    action set_ingress_src_port_range_id(bit<8> range_id) {
        meta.upf.src_port_range_id = range_id;
    }
    @hidden
    action set_ingress_tcp_port_fields() {
        meta.upf.l4_sport = hdr.tcp.sport;
        meta.upf.l4_dport = hdr.tcp.dport;
    }

    @hidden
    action set_ingress_udp_port_fields() {
        meta.upf.l4_sport = hdr.udp.sport;
        meta.upf.l4_dport = hdr.udp.dport;
    }

    table ingress_l4_dst_port {
        actions = {
            nop;
            set_ingress_dst_port_range_id;
        }
        key = {
            meta.upf.l4_dport: exact; // range macthing unsupported
        }
        size = 512;
    }
    table ingress_l4_src_port {
        actions = {
            nop;
            set_ingress_src_port_range_id;
        }
        key = {
            meta.upf.l4_sport: exact; // range macthing unsupported
        }
        size = 512;
    }
    table ingress_l4port_fields {
        actions = {
            nop;
            set_ingress_tcp_port_fields;
            set_ingress_udp_port_fields;
        }
        key = {
            hdr.tcp.isValid() : exact;
            hdr.udp.isValid() : exact;
        }
        const entries = {
                (true,false) : set_ingress_tcp_port_fields();
                (false,true) : set_ingress_udp_port_fields();
                (false,false) : nop();
        }
    }
    apply {
        ingress_l4port_fields.apply();
        ingress_l4_src_port.apply();
        ingress_l4_dst_port.apply();
    }
}

control ip_forward(inout headers hdr,
                  inout metadata meta,
                  in    psa_ingress_input_metadata_t  istd,
                  inout psa_ingress_output_metadata_t ostd) {
    action drop() {
        ingress_drop(ostd);
    }

    action forward(macAddr_t srcAddr,macAddr_t dstAddr, PortId_t port) {
        send_to_port(ostd,port);
        hdr.ethernet.srcAddr = srcAddr;
        hdr.ethernet.dstAddr = dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    table ipv4_lpm {
        key= {
              meta.upf.dest : exact;
              meta.upf.outer_dst_addr: lpm;
        }
        actions = {
            forward();
            drop();
        }
        const default_action = drop();
    }

    apply {
        ipv4_lpm.apply();
    }
}



control upf_ingress(
        inout ipv4_t              gtpu_ipv4,
        inout udp_t               gtpu_udp,
        inout gtpu_t              gtpu,
        inout ipv4_t              ipv4,
        inout udp_t               udp,
        inout metadata            meta,
        in    psa_ingress_input_metadata_t  istd,
        inout psa_ingress_output_metadata_t ostd) {


    action drop() {
        ingress_drop(ostd);
    }

    @hidden
    action gtpu_decap() {
        gtpu_ipv4.setInvalid();
        gtpu_udp.setInvalid();
        gtpu.setInvalid();
        meta.upf.outer_dst_addr=ipv4.dstAddr;
    }

    action set_seid(bit<64> seid) {
        meta.upf.seid=seid;
    }

    action set_far_id(bit<32> far_id) {
        meta.upf.far_id=far_id;
    }

    action far_encap_forward(destination_t dest,
                       bit<32> teid,
                       bit<32> gtpu_remote_ip,
                       bit<32> gtpu_local_ip)  {
        meta.upf.dest=dest;
        meta.upf.teid = teid;
        meta.upf.gtpu_remote_ip = gtpu_remote_ip;
        meta.upf.gtpu_local_ip = gtpu_local_ip;
        meta.upf.outer_dst_addr=gtpu_remote_ip;
    }

    action far_forward(destination_t dest)  {
        meta.upf.dest=dest;
    }

    action set_source_interface(destination_t src) {
        meta.upf.src=src;
    }


    table source_interface_lookup_by_port {
        key = {
            istd.ingress_port: exact;
        }
        actions = {
            set_source_interface;
            @defaultonly nop();
        }
        const default_action = nop();
    }

    table session_lookup_by_ue_ip {
        key = {
            // UE addr for downlink
            ipv4.dstAddr : exact @name("ipv4_dst");
        }
        actions = {
            set_seid();
            @defaultonly nop();
        }
        const default_action = nop();
    }

    table session_lookup_by_teid {
        key = {
            gtpu.teid : exact;
        }
        actions = {
            set_seid();
            nop();
        }
        const default_action = nop();
    }

    table pdr_lookup {
        key= {
            meta.upf.seid:  exact;
            ipv4.srcAddr: ternary;
            ipv4.dstAddr: ternary;
            ipv4.protocol: ternary;
            meta.upf.src_port_range_id: ternary;
            meta.upf.dst_port_range_id: ternary;
            meta.upf.src : exact;
        }
        actions = {
            set_far_id();
            @defaultonly drop();
        }
        const default_action = drop();
    }

    table far_lookup {
        key= {
            meta.upf.far_id: exact;
        }
        actions = {
            far_forward();
            far_encap_forward();
            drop();
        }
        const default_action = drop();
    }

    @hidden
    action gtpu_encap() {
        gtpu_ipv4.setValid();
        gtpu_ipv4.version = IP_VERSION_4;
        gtpu_ipv4.ihl = IPV4_MIN_IHL;
        gtpu_ipv4.dscp = 0;
        gtpu_ipv4.ecn = 0;
        gtpu_ipv4.totalLen = ipv4.totalLen
                + (IPV4_HDR_SIZE + UDP_HDR_SIZE + GTP_HDR_SIZE);
        gtpu_ipv4.identification = 0x1513;
        gtpu_ipv4.flags = 0;
        gtpu_ipv4.fragOffset = 0;
        gtpu_ipv4.ttl = DEFAULT_IPV4_TTL;
        gtpu_ipv4.protocol = PROTO_UDP;
        gtpu_ipv4.dstAddr = meta.upf.gtpu_remote_ip;
        gtpu_ipv4.srcAddr = meta.upf.gtpu_local_ip;
        gtpu_ipv4.hdrChecksum = 0; // Updated later

        gtpu_udp.setValid();
        gtpu_udp.sport = UDP_PORT_GTPU;
        gtpu_udp.dport = UDP_PORT_GTPU;
        gtpu_udp.len = meta.upf.ipv4_len
                + (UDP_HDR_SIZE + GTP_HDR_SIZE);
        gtpu_udp.checksum = 0;

        gtpu.setValid();
        gtpu.version = GTPU_VERSION;
        gtpu.pt = GTP_PROTOCOL_TYPE_GTP;
        gtpu.spare = 0;
        gtpu.ex_flag = 0;
        gtpu.seq_flag = 0;
        gtpu.npdu_flag = 0;
        gtpu.msgtype = GTP_GPDU;
        gtpu.msglen = meta.upf.ipv4_len;
        gtpu.teid = meta.upf.teid;
    }


    apply {
        source_interface_lookup_by_port.apply();
        if (gtpu.isValid()) {
            if (session_lookup_by_teid.apply().hit) {
                gtpu_decap();
            } else {
                ingress_drop(ostd);
            }
        } else if (!session_lookup_by_ue_ip.apply().hit) {
            return;
        }
        if (pdr_lookup.apply().hit) {
            if (far_lookup.apply().hit) {
                    meta.upf.ipv4_len = ipv4.totalLen;
            }
        }
        if (meta.upf.dest == ACCESS || meta.upf.dest == CORE) {
            gtpu_encap();
        }
    }
}

control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        if (hdr.gtpu.isValid()) {
            hdr.gtpu_ipv4 = hdr.ipv4;
            hdr.ipv4 = hdr.inner_ipv4;
            hdr.gtpu_udp = hdr.udp;
            if (hdr.inner_udp.isValid()) {
                hdr.udp = hdr.inner_udp;
            } else {
                hdr.udp.setInvalid();
            }
        }
        upf_process_ingress_l4port.apply(hdr,meta);
        upf_ingress.apply(hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu,
                    hdr.ipv4, hdr.udp, meta, istd, ostd);
        ip_forward.apply(hdr,meta,istd,ostd);
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers parsed_hdr,
                        inout metadata user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_t normal_meta,
                        in empty_t clone_i2e_meta,
                        in empty_t clone_e2e_meta)
{
    state start {
        transition accept;
    }

}
control egress(inout headers hdr,
               inout metadata user_meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply { }
}


control IngressDeparserImpl(packet_out packet,
                            out empty_t clone_i2e_meta,
                            out empty_t resubmit_meta,
                            out empty_t normal_meta,
                            inout headers hdr,
                            in metadata meta,
                            in psa_ingress_output_metadata_t istd)
{
    InternetChecksum() ck;

    apply {
        if(hdr.gtpu_ipv4.isValid()) {
            ck.clear();
            ck.add({
            /* 16-bit word  0   */ hdr.gtpu_ipv4.version, hdr.gtpu_ipv4.ihl, hdr.gtpu_ipv4.dscp, hdr.gtpu_ipv4.ecn,
            /* 16-bit word  1   */ hdr.gtpu_ipv4.totalLen,
            /* 16-bit word  2   */ hdr.gtpu_ipv4.identification,
            /* 16-bit word  3   */ hdr.gtpu_ipv4.flags, hdr.gtpu_ipv4.fragOffset,
            /* 16-bit word  4   */ hdr.gtpu_ipv4.ttl, hdr.gtpu_ipv4.protocol,
            /* 16-bit word  5 skip hdr.gtpu_ipv4.hdrChecksum, */
            /* 16-bit words 6-7 */ hdr.gtpu_ipv4.srcAddr,
            /* 16-bit words 8-9 */ hdr.gtpu_ipv4.dstAddr});
            hdr.gtpu_ipv4.hdrChecksum = ck.get();
        }
        ck.clear();
        ck.subtract(hdr.ipv4.hdrChecksum);
        ck.add({/* 16-bit word */ hdr.ipv4.ttl, hdr.ipv4.protocol });
        hdr.ipv4.hdrChecksum = ck.get();
        packet.emit(hdr.ethernet);
        packet.emit(hdr.gtpu_ipv4); // only for DL
        packet.emit(hdr.gtpu_udp);  // only for DL
        packet.emit(hdr.gtpu);      // only for DL
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        packet.emit(hdr.icmp);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_t clone_e2e_meta,
                           out empty_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
