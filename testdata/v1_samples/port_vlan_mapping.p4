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

header_type snap_header_t {
    fields {
        dsap : 8;
        ssap : 8;
        control_ : 8;
        oui : 24;
        type_ : 16;
    }
}

header_type roce_header_t {
    fields {
        ib_grh : 320;
        ib_bth : 96;
    }
}

header_type roce_v2_header_t {
    fields {
        ib_bth : 96;
    }
}

header_type fcoe_header_t {
    fields {
        version : 4;
        type_ : 4;
        sof : 8;
        rsvd1 : 32;
        ts_upper : 32;
        ts_lower : 32;
        size_ : 32;
        eof : 8;
        rsvd2 : 24;
    }
}

header_type cpu_header_t {
    fields {
        qid : 3;
        pad : 1;
        reason_code : 12;
        rxhash : 16;
        bridge_domain : 16;
        ingress_lif : 16;
        egress_lif : 16;
        lu_bypass_ingress : 1;
        lu_bypass_egress : 1;
        pad1 : 14;
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

header_type vlan_tag_3b_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 4;
        etherType : 16;
    }
}
header_type vlan_tag_5b_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 20;
        etherType : 16;
    }
}

header_type ieee802_1ah_t {
    fields {
        pcp : 3;
        dei : 1;
        uca : 1;
        reserved : 3;
        i_sid : 24;
    }
}

header_type mpls_t {
    fields {
        label : 20;
        tc : 3;
        bos : 1;
        ttl : 8;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type ipv6_t {
    fields {
        version : 4;
        trafficClass : 8;
        flowLabel : 20;
        payloadLen : 16;
        nextHdr : 8;
        hopLimit : 8;
        srcAddr : 128;
        dstAddr : 128;
    }
}

header_type icmp_t {
    fields {
        type_ : 8;
        code : 8;
        hdrChecksum : 16;
    }
}

header_type icmpv6_t {
    fields {
        type_ : 8;
        code : 8;
        hdrChecksum : 16;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header_type sctp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        verifTag : 32;
        checksum : 32;
    }
}

header_type gre_t {
    fields {
        C : 1;
        R : 1;
        K : 1;
        S : 1;
        s : 1;
        recurse : 3;
        flags : 5;
        ver : 3;
        proto : 16;
    }
}

header_type nvgre_t {
    fields {
        tni : 24;
        reserved : 8;
    }
}

/* 8 bytes */
header_type erspan_header_v1_t {
    fields {
        version : 4;
        vlan : 12;
        priority : 6;
        span_id : 10;
        direction : 8;
        truncated: 8;
    }
}

/* 8 bytes */
header_type erspan_header_v2_t {
    fields {
        version : 4;
        vlan : 12;
        priority : 6;
        span_id : 10;
        unknown7 : 32;
    }
}

header_type ipsec_esp_t {
    fields {
        spi : 32;
        seqNo : 32;
    }
}

header_type ipsec_ah_t {
    fields {
        nextHdr : 8;
        length_ : 8;
        zero : 16;
        spi : 32;
        seqNo : 32;
    }
}

header_type arp_rarp_t {
    fields {
        hwType : 16;
        protoType : 16;
        hwAddrLen : 8;
        protoAddrLen : 8;
        opcode : 16;
    }
}

header_type arp_rarp_ipv4_t {
    fields {
        srcHwAddr : 48;
        srcProtoAddr : 32;
        dstHwAddr : 48;
        dstProtoAddr : 32;
    }
}

header_type eompls_t {
    fields {
        zero : 4;
        reserved : 12;
        seqNo : 16;
    }
}

header_type vxlan_t {
    fields {
        flags : 8;
        reserved : 24;
        vni : 24;
        reserved2 : 8;
    }
}

header_type nsh_t {
    fields {
        oam : 1;
        context : 1;
        flags : 6;
        reserved : 8;
        protoType: 16;
        spath : 24;
        sindex : 8;
    }
}

header_type nsh_context_t {
    fields {
        network_platform : 32;
        network_shared : 32;
        service_platform : 32;
        service_shared : 32;
    }
}

/* GENEVE HEADERS 
   3 possible options with known type, known length */ 
 
header_type genv_t { 
    fields { 
        ver : 2; 
        optLen : 6; 
        oam : 1; 
        critical : 1; 
        reserved : 6; 
        protoType : 16; 
        vni : 24; 
        reserved2 : 8; 
    } 
} 
 
#define GENV_OPTION_A_TYPE 0x000001 
/* TODO: Would it be convenient to have some kind of sizeof macro ? */ 
#define GENV_OPTION_A_LENGTH 2 /* in bytes */ 
 
header_type genv_opt_A_t { 
    fields { 
        optClass : 16; 
        optType : 8; 
        reserved : 3; 
        optLen : 5; 
        data : 32; 
    } 
} 
 
#define GENV_OPTION_B_TYPE 0x000002 
#define GENV_OPTION_B_LENGTH 3 /* in bytes */ 
 
header_type genv_opt_B_t { 
    fields { 
        optClass : 16; 
        optType : 8; 
        reserved : 3; 
        optLen : 5; 
        data : 64; 
    } 
} 
 
#define GENV_OPTION_C_TYPE 0x000003 
#define GENV_OPTION_C_LENGTH 2 /* in bytes */ 
 
header_type genv_opt_C_t { 
    fields { 
        optClass : 16; 
        optType : 8; 
        reserved : 3; 
        optLen : 5; 
        data : 32; 
    } 
} 
parser start {
    return parse_input_port;
}

header_type input_port_hdr_t {
    fields {
        port : 16;
    }
}

header input_port_hdr_t input_port_hdr;

parser parse_input_port {
    extract (input_port_hdr);
    return parse_ethernet;
}

#define ETHERTYPE_CPU 0x9000, 0x010c
#define ETHERTYPE_VLAN 0x8100, 0x9100, 0x9200, 0x9300
#define ETHERTYPE_MPLS 0x8847
#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86dd
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_RARP 0x8035
#define ETHERTYPE_NSH 0x894f
#define ETHERTYPE_ETHERNET 0x6558
#define ETHERTYPE_ROCE 0x8915
#define ETHERTYPE_FCOE 0x8906
/* missing: vlan_3b, vlan_5b, ieee802_1q, ieee802_1ad */

#define IPV4_MULTICAST_MAC 0x01005E
#define IPV6_MULTICAST_MAC 0x3333

/* Tunnel types */
#define TUNNEL_TYPE_NONE               0
#define TUNNEL_TYPE_VXLAN              1
#define TUNNEL_TYPE_GRE                2
#define TUNNEL_TYPE_GENEVE             3 
#define TUNNEL_TYPE_NVGRE              4
#define TUNNEL_TYPE_MPLS               5

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0 mask 0xf800: parse_snap_header; /* < 1536 */
        ETHERTYPE_CPU : parse_cpu_header;
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_MPLS : parse_mpls;
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
        ETHERTYPE_ARP : parse_arp_rarp;
        ETHERTYPE_RARP : parse_arp_rarp;
        ETHERTYPE_NSH : parse_nsh;
        ETHERTYPE_ROCE: parse_roce;
        ETHERTYPE_FCOE: parse_fcoe;
        default: ingress;
    }
}

header snap_header_t snap_header;

parser parse_snap_header {
    extract(snap_header);
    return ingress;
}

header roce_header_t roce;

parser parse_roce {
    extract(roce);
    return ingress;
}

header fcoe_header_t fcoe;

parser parse_fcoe {
    extract(fcoe);
    return ingress;
}

header cpu_header_t cpu_header;

parser parse_cpu_header {
    extract(cpu_header);
    return select(latest.etherType) {
        0 mask 0xf800: parse_snap_header; /* < 1536 */
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_MPLS : parse_mpls;
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
        ETHERTYPE_ARP : parse_arp_rarp;
        ETHERTYPE_RARP : parse_arp_rarp;
        ETHERTYPE_NSH : parse_nsh;
        ETHERTYPE_ROCE: parse_roce;
        ETHERTYPE_FCOE: parse_fcoe;
        default: ingress;
    }
}

#define VLAN_DEPTH 2
header vlan_tag_t vlan_tag_[VLAN_DEPTH];
header vlan_tag_3b_t vlan_tag_3b[VLAN_DEPTH];
header vlan_tag_5b_t vlan_tag_5b[VLAN_DEPTH];

parser parse_vlan {
    extract(vlan_tag_[next]);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_MPLS : parse_mpls;
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_IPV6 : parse_ipv6;
        ETHERTYPE_ARP : parse_arp_rarp;
        ETHERTYPE_RARP : parse_arp_rarp;
        default: ingress;
    }
}

#define MPLS_DEPTH 3
/* all the tags but the last one */
header mpls_t mpls[MPLS_DEPTH];

/* last mpls tag in the stack */
header mpls_t mpls_bos;

/* TODO: this will be optimized when pushed to the chip ? */

parser parse_mpls {
    return select(current(23, 1)) {
        0 : parse_mpls_not_bos;
        1 : parse_mpls_bos;
        default: ingress;
    }
}

parser parse_mpls_not_bos {
    extract(mpls[next]);
    return parse_mpls;
}

parser parse_mpls_bos {
    extract(mpls_bos);
    return select(current(0, 4)) {
        0x4 : parse_inner_ipv4;
        0x6 : parse_inner_ipv6;
        default : parse_eompls;
    }    
}

#define IP_PROTOCOLS_ICMP 1
#define IP_PROTOCOLS_TCP 6
#define IP_PROTOCOLS_UDP 17
#define IP_PROTOCOLS_GRE 47
#define IP_PROTOCOLS_IPSEC_ESP 50
#define IP_PROTOCOLS_IPSEC_AH 51
#define IP_PROTOCOLS_ICMPV6 58
#define IP_PROTOCOLS_SCTP 132

header ipv4_t ipv4;

field_list ipv4_checksum_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.flags;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum  {
    verify ipv4_checksum if(ipv4.ihl == 5);
    update ipv4_checksum if(ipv4.ihl == 5);
}

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_ICMP : parse_icmp;
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        IP_PROTOCOLS_GRE : parse_gre;
//        IP_PROTOCOLS_SCTP : parse_sctp;
        default: ingress;
    }
}

header ipv6_t ipv6;

parser parse_ipv6 {
    extract(ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_ICMPV6 : parse_icmpv6;
        IP_PROTOCOLS_TCP : parse_tcp;
        IP_PROTOCOLS_UDP : parse_udp;
        IP_PROTOCOLS_GRE : parse_gre;
//        IP_PROTOCOLS_SCTP : parse_sctp;
        default: ingress;
    }
}

header icmp_t icmp;

parser parse_icmp {
    extract(icmp);
    return ingress;
}

header icmpv6_t icmpv6;

parser parse_icmpv6 {
    extract(icmpv6);
    return ingress;
}

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    return ingress;
}

#define UDP_PORT_VXLAN 4789
#define UDP_PORT_GENV 6081
// Check IANA UDP port number
 #define UDP_PORT_ROCE_V2 1021

header udp_t udp;

header roce_v2_header_t roce_v2;

parser parse_roce_v2 {
    extract(roce_v2);
    return ingress;
}

parser parse_udp {
    extract(udp);
    return select(latest.dstPort) {
        UDP_PORT_VXLAN : parse_vxlan;
        UDP_PORT_GENV: parse_geneve;
        UDP_PORT_ROCE_V2: parse_roce_v2;
        default: ingress;
    }
}

header sctp_t sctp;

parser parse_sctp {
    extract(sctp);
    return ingress;
}


#define GRE_PROTOCOLS_NVGRE 0x6558
#define GRE_PROTOCOLS_GRE 0x6559
#define GRE_PROTOCOLS_ERSPAN_V1 0x88BE
#define GRE_PROTOCOLS_ERSPAN_V2 0x22EB

#define GRE_DEPTH 2

//header gre_t gre[GRE_DEPTH];
header gre_t gre;

#if 0
header_type gre_opt_t {
    fields {
        key : 32;
    }
}

header gre_opt_t gre_opt;

parser parse_gre_key {
    extract(gre_opt);
    return ingress;
}
parser parse_gre_key2 {
    extract(gre_opt);
    extract(gre_opt);
    return ingress;
}
parser parse_gre_key22 {
    extract(gre_opt);
    extract(gre_opt);
    return ingress;
}
parser parse_gre_opt1 {
    extract(gre_opt);
    return ingress;
}
parser parse_gre_opt2 {
    extract(gre_opt);
    extract(gre_opt);
    return ingress;
}
parser parse_gre_opt3 {
    extract(gre_opt);
    extract(gre_opt);
    extract(gre_opt);
    return ingress;
}

parser parse_gre_opts {
    return select(latest.C, latest.S, latest.K) {
        1 mask 0x0000 :  parse_gre_key;
        2 mask 0x0000 :  parse_gre_opt1;
        3 mask 0x0000 :  parse_gre_key2;
        4 mask 0x0000 :  parse_gre_opt1;
        5 mask 0x0000 :  parse_gre_key22;
        6 mask 0x0000 :  parse_gre_opt2;
        7 mask 0x0000 :  parse_gre_opt3;
        default: ingress;
    }
}
#endif

parser parse_gre {
    extract(gre);
//    parse_gre_opts;
    return select(latest.K, latest.proto) {
        GRE_PROTOCOLS_NVGRE : parse_nvgre;
//        GRE_PROTOCOLS_GRE : parse_gre;
        GRE_PROTOCOLS_ERSPAN_V1 : parse_erspan_v1;
        GRE_PROTOCOLS_ERSPAN_V2 : parse_erspan_v2;
        ETHERTYPE_NSH : parse_nsh;
        default: ingress;
    }
}

header nvgre_t nvgre;
header ethernet_t inner_ethernet;

header ipv4_t inner_ipv4;
header ipv6_t inner_ipv6;
header ipv4_t outer_ipv4;
header ipv6_t outer_ipv6;

field_list inner_ipv4_checksum_list {
        inner_ipv4.version;
        inner_ipv4.ihl;
        inner_ipv4.diffserv;
        inner_ipv4.totalLen;
        inner_ipv4.identification;
        inner_ipv4.flags;
        inner_ipv4.fragOffset;
        inner_ipv4.ttl;
        inner_ipv4.protocol;
        inner_ipv4.srcAddr;
        inner_ipv4.dstAddr;
}

field_list_calculation inner_ipv4_checksum {
    input {
        inner_ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field inner_ipv4.hdrChecksum {
    verify inner_ipv4_checksum if(valid(ipv4));
    update inner_ipv4_checksum if(valid(ipv4));
}

header udp_t outer_udp;

parser parse_nvgre {
    extract(nvgre);
    return parse_inner_ethernet;
}

header erspan_header_v1_t erspan_v1_header;

parser parse_erspan_v1 {
    extract(erspan_v1_header);
    return ingress;
}

header erspan_header_v2_t erspan_v2_header;

parser parse_erspan_v2 {
    extract(erspan_v2_header);
    return ingress;
}

#define ARP_PROTOTYPES_ARP_RARP_IPV4 0x0800

header arp_rarp_t arp_rarp;

parser parse_arp_rarp {
    extract(arp_rarp);
    return select(latest.protoType) {
        ARP_PROTOTYPES_ARP_RARP_IPV4 : parse_arp_rarp_ipv4;
        default: ingress;
    }
}

header arp_rarp_ipv4_t arp_rarp_ipv4;

parser parse_arp_rarp_ipv4 {
    extract(arp_rarp_ipv4);
    return ingress;
}

header eompls_t eompls;

parser parse_eompls {
    extract(eompls);
    extract(inner_ethernet);
    return ingress;
}

header vxlan_t vxlan;

parser parse_vxlan {
    extract(vxlan);
    return parse_inner_ethernet;
}

header genv_t genv;

header genv_opt_A_t genv_opt_A;
header genv_opt_B_t genv_opt_B;
header genv_opt_C_t genv_opt_C;

parser parse_geneve {
    extract(genv);
    /*
    counter_init(counter_1, latest.optLen);
    return select(latest.optLen) {
        0 : parse_genv_inner;
        default : parse_genv_opts;
    }
    */
    return parse_genv_inner;
}

#if 0

parser parse_genv_opts {
    /* switching on combined class and type */
    return select(current(0, 24)) {
        GENV_OPTION_A_TYPE: parse_genv_opt_A;
        GENV_OPTION_B_TYPE: parse_genv_opt_B;
        GENV_OPTION_C_TYPE: parse_genv_opt_C;
    }
}

parser parse_genv_opt_A {
    extract(genv_opt_A);
    counter_decrement(counter_1, GENV_OPTION_A_LENGTH);
    return select(counter_1) {
        0 : parse_genv_inner;
        default : parse_genv_opts;
    }
}

parser parse_genv_opt_B {
    extract(genv_opt_B);
    counter_decrement(counter_1, GENV_OPTION_B_LENGTH);
    return select(counter_1) {
        0 : parse_genv_inner;
        default : parse_genv_opts;
    }
}

parser parse_genv_opt_C {
    extract(genv_opt_C);
    counter_decrement(counter_1, GENV_OPTION_C_LENGTH);
    return select(counter_1) {
        0 : parse_genv_inner;
        default : parse_genv_opts;
    }
}
#endif

parser parse_genv_inner {
    return select(genv.protoType) {
        ETHERTYPE_ETHERNET : parse_inner_ethernet;
        ETHERTYPE_IPV4 : parse_inner_ipv4;
        ETHERTYPE_IPV6 : parse_inner_ipv6;
        default : ingress;
    }
}

header nsh_t nsh;
header nsh_context_t nsh_context;

parser parse_nsh {
    extract(nsh);
    extract(nsh_context);
    return select(nsh.protoType) {
        ETHERTYPE_IPV4 : parse_inner_ipv4;
        ETHERTYPE_IPV6 : parse_inner_ipv6;
        ETHERTYPE_ETHERNET : parse_inner_ethernet;
        default: ingress;
    }
}

parser parse_inner_ipv4 {
    extract(inner_ipv4);
    return select(latest.fragOffset, latest.protocol) {
        IP_PROTOCOLS_ICMP : parse_inner_icmp;
        IP_PROTOCOLS_TCP : parse_inner_tcp;
        IP_PROTOCOLS_UDP : parse_inner_udp;
//        IP_PROTOCOLS_SCTP : parse_inner_sctp;
        default: ingress;
    }
}

header icmp_t inner_icmp;

parser parse_inner_icmp {
    extract(inner_icmp);
    return ingress;
}

header tcp_t inner_tcp;

parser parse_inner_tcp {
    extract(inner_tcp);
    return ingress;
}

header udp_t inner_udp;

parser parse_inner_udp {
    extract(inner_udp);
    return ingress;    
}

header sctp_t inner_sctp;

parser parse_inner_sctp {
    extract(inner_sctp);
    return ingress;
}

parser parse_inner_ipv6 {
    extract(inner_ipv6);
    return select(latest.nextHdr) {
        IP_PROTOCOLS_ICMPV6 : parse_inner_icmpv6;
        IP_PROTOCOLS_TCP : parse_inner_tcp;
        IP_PROTOCOLS_UDP : parse_inner_udp;
//        IP_PROTOCOLS_SCTP : parse_inner_sctp;
        default: ingress;
    }
}

header icmpv6_t inner_icmpv6;

parser parse_inner_icmpv6 {
    extract(inner_icmpv6);
    return ingress;
}

parser parse_inner_ethernet {
    extract(inner_ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_inner_ipv4;
        ETHERTYPE_IPV6 : parse_inner_ipv6;
        default: ingress;
    }
}

#define VALIDATE_PACKET_TABLE_SIZE             64
#define PORTMAP_TABLE_SIZE                     288
#define STORM_CONTROL_TABLE_SIZE               256
#define STORM_CONTROL_METER_TABLE_SIZE         256
#define PORT_VLAN_TABLE_SIZE                   32768
#define OUTER_BD_TABLE_SIZE                    256
#define OUTER_ROUTER_MAC_TABLE_SIZE            256
#define DEST_TUNNEL_TABLE_SIZE                 512
#define SRC_TUNNEL_TABLE_SIZE                  16384
#define OUTER_MULTICAST_STAR_G_TABLE_SIZE      512
#define OUTER_MULTICAST_S_G_TABLE_SIZE         1024
#define VNID_MAPPING_TABLE_SIZE                16384
#define BD_TABLE_SIZE                          16384
#define OUTER_MCAST_RPF_TABLE_SIZE             512
#define MPLS_TABLE_SIZE                        4096
#define VALIDATE_MPLS_TABLE_SIZE               512

#define ROUTER_MAC_TABLE_SIZE                  512
#define DMAC_TABLE_SIZE                        65536
#define SMAC_TABLE_SIZE                        65536
#define IPSG_TABLE_SIZE                        8192
#define IPSG_PERMIT_SPECIAL_TABLE_SIZE         512
#define INGRESS_MIRROR_ACL_TABLE_SIZE          512
#define INGRESS_MAC_ACL_TABLE_SIZE             512
#define INGRESS_IP_ACL_TABLE_SIZE              1024
#define INGRESS_IPV6_ACL_TABLE_SIZE            512
#define INGRESS_QOS_ACL_TABLE_SIZE             512
#define INGRESS_IP_RACL_TABLE_SIZE             1024
#define INGRESS_IPV6_RACL_TABLE_SIZE           256
#define IP_NAT_TABLE_SIZE                      4096

#define IPV4_LPM_TABLE_SIZE                    8192
#define IPV6_LPM_TABLE_SIZE                    2048
#define IPV4_HOST_TABLE_SIZE                   65536
#define IPV6_HOST_TABLE_SIZE                   16384

#define IPV4_MULTICAST_STAR_G_TABLE_SIZE       2048
#define IPV4_MULTICAST_S_G_TABLE_SIZE          4096
#define IPV6_MULTICAST_STAR_G_TABLE_SIZE       512
#define IPV6_MULTICAST_S_G_TABLE_SIZE          512
#define MCAST_RPF_TABLE_SIZE                   32768

#define FWD_RESULT_TABLE_SIZE                  512
#define URPF_GROUP_TABLE_SIZE                  32768
#define ECMP_GROUP_TABLE_SIZE                  1024
#define ECMP_SELECT_TABLE_SIZE                 16384
#define NEXTHOP_TABLE_SIZE                     49152
#ifdef HARLYN
#define LAG_GROUP_TABLE_SIZE                   4096
#else
#define LAG_GROUP_TABLE_SIZE                   1024
#endif
#define LAG_SELECT_TABLE_SIZE                  1024
#define SYSTEM_ACL_SIZE                        1024
#define LEARN_NOTIFY_TABLE_SIZE                512

#define MAC_REWRITE_TABLE_SIZE                 512
#define EGRESS_VNID_MAPPING_TABLE_SIZE         16384
#define EGRESS_BD_MAPPING_TABLE_SIZE           16384
#define REPLICA_TYPE_TABLE_SIZE                16
#define RID_TABLE_SIZE                         65536
#define TUNNEL_DECAP_TABLE_SIZE                512
#define IP_MTU_TABLE_SIZE                      512
#define EGRESS_SYSTEM_ACL_TABLE_SIZE           512
#define EGRESS_VLAN_XLATE_TABLE_SIZE           32768
#define SPANNING_TREE_TABLE_SIZE               4096
#define CPU_REWRITE_TABLE_SIZE                 512
#define EGRESS_ACL_TABLE_SIZE                  2048
#define VLAN_DECAP_TABLE_SIZE                  256
#define TUNNEL_HEADER_TABLE_SIZE               256
#define TUNNEL_REWRITE_TABLE_SIZE              16384
#define TUNNEL_SMAC_REWRITE_TABLE_SIZE         512
#define TUNNEL_DMAC_REWRITE_TABLE_SIZE         16384

/* Boolean */
#define FALSE                                  0
#define TRUE                                   1

/* Packet types */
#define L2_UNICAST                             1
#define L2_MULTICAST                           2
#define L2_BROADCAST                           4

#define STORM_CONTROL_COLOR_GREEN              0
#define STORM_CONTROL_COLOR_RED                1

/* IP types */
#define IPTYPE_NONE                            0
#define IPTYPE_IPV4                            1
#define IPTYPE_IPV6                            2

/* Multicast modes */
#define MCAST_MODE_NONE                        0
#define MCAST_MODE_SM                          1
#define MCAST_MODE_BIDIR                       2

#define OUTER_MCAST_KEY_TYPE_BD                0
#define OUTER_MCAST_KEY_TYPE_VRF               1

/* URPF modes */
#define URPF_MODE_NONE                         0
#define URPF_MODE_LOOSE                        1 
#define URPF_MODE_STRICT                       2

/* NAT modes */
#define NAT_MODE_NONE                          0
#define NAT_MODE_INSIDE                        1
#define NAT_MODE_OUTSIDE                       2

/* Egress tunnel types */
#define EGRESS_TUNNEL_TYPE_NONE                0
#define EGRESS_TUNNEL_TYPE_IPV4_VXLAN          1
#define EGRESS_TUNNEL_TYPE_IPV6_VXLAN          2
#define EGRESS_TUNNEL_TYPE_IPV4_GENEVE         3
#define EGRESS_TUNNEL_TYPE_IPV6_GENEVE         4
#define EGRESS_TUNNEL_TYPE_IPV4_NVGRE          5
#define EGRESS_TUNNEL_TYPE_IPV6_NVGRE          6
#define EGRESS_TUNNEL_TYPE_IPV4_ERSPANV2       7
#define EGRESS_TUNNEL_TYPE_IPV6_ERSPANV2       8
#define EGRESS_TUNNEL_TYPE_IPV4_GRE            9
#define EGRESS_TUNNEL_TYPE_IPV6_GRE            10
#define EGRESS_TUNNEL_TYPE_IPV4_IP             11
#define EGRESS_TUNNEL_TYPE_IPV6_IP             12
#define EGRESS_TUNNEL_TYPE_MPLS_L2VPN          13
#define EGRESS_TUNNEL_TYPE_MPLS_L3VPN          14

#define VRF_BIT_WIDTH                          12
#define BD_BIT_WIDTH                           16
#define ECMP_BIT_WIDTH                         10
#define LAG_BIT_WIDTH                          8
#define IFINDEX_BIT_WIDTH                      16

#define STP_GROUP_NONE                         0

#define CPU_PORT_ID                            64
#define CPU_MIRROR_SESSION_ID                  250

/* Learning Receivers */
#ifndef HARLYN
#define MAC_LEARN_RECEIVER                     1024
#else
#define MAC_LEARN_RECEIVER                     0
#endif

/* Nexthop Type */
#define NEXTHOP_TYPE_SIMPLE                    0
#define NEXTHOP_TYPE_ECMP                      1

/* METADATA */
header_type ingress_metadata_t {
    fields {
        lkp_pkt_type : 3;
        lkp_mac_sa : 48;
        lkp_mac_da : 48;
        lkp_mac_type : 16;
        lkp_ip_type : 2;
        lkp_ipv4_sa : 32;
        lkp_ipv4_da : 32;
        lkp_ipv6_sa : 128;
        lkp_ipv6_da : 128;
        lkp_ip_proto : 8;
        lkp_ip_tc : 8;
        lkp_ip_ttl : 8;

        lkp_l4_sport : 16;
        lkp_l4_dport : 16;
        lkp_inner_l4_sport : 16;
        lkp_inner_l4_dport : 16;

        lkp_icmp_type : 8;
        lkp_icmp_code : 8;
        lkp_inner_icmp_type : 8;
        lkp_inner_icmp_code : 8;

        l3_length : 16;                        /* l3 length */

        ifindex : IFINDEX_BIT_WIDTH;           /* input interface index - MSB bit lag*/
        vrf : VRF_BIT_WIDTH;                   /* VRF */

#ifdef HARLYN
        tunnel_type : 8;                       /* tunnel type from parser */
#else
        tunnel_type : 4;                       /* tunnel type from parser */
#endif
        tunnel_terminate : 1;                  /* should tunnel be terminated */
        tunnel_vni : 24;                       /* tunnel id */
        src_vtep_miss : 1;                     /* src vtep lookup failed */

        outer_bd : 8;                          /* outer BD */
        outer_ipv4_mcast_key_type : 1;         /* 0 bd, 1 vrf */
        outer_ipv4_mcast_key : 8;              /* bd or vrf value */
        outer_ipv6_mcast_key_type : 1;         /* 0 bd, 1 vrf */
        outer_ipv6_mcast_key : 8;              /* bd or vrf value */
        outer_mcast_route_hit : 1;             /* hit in the outer multicast table */
        outer_mcast_mode : 2;                  /* multicast mode from route */
        outer_dscp : 8;                        /* outer dscp */
        outer_ttl : 8;                         /* outer ttl */
        outer_routed : 1;                      /* is outer packet routed */

        l2_multicast : 1;                      /* packet is l2 multicast */
        ip_multicast : 1;                      /* packet is ip multicast */
        src_is_link_local : 1;                 /* source is link local address */
        bd : BD_BIT_WIDTH;                     /* inner BD */
        ipsg_enabled : 1;                      /* is ip source guard feature enabled */
        ipv4_unicast_enabled : 1;              /* is ipv4 unicast routing enabled on BD */
        ipv6_unicast_enabled : 1;              /* is ipv6 unicast routing enabled on BD */
        ipv4_multicast_mode : 2;               /* ipv4 multicast mode BD */
        ipv6_multicast_mode : 2;               /* ipv6 multicast mode BD */
        igmp_snooping_enabled : 1;             /* is IGMP snooping enabled on BD */
        mld_snooping_enabled : 1;              /* is MLD snooping enabled on BD */
        mpls_enabled : 1;                      /* is mpls enabled on BD */
        mpls_label: 20;                        /* Mpls label */
        mpls_exp: 3;                           /* Mpls Traffic Class */
        mpls_ttl: 8;                           /* Mpls Ttl */
        ipv4_urpf_mode : 2;                    /* 0: none, 1: strict, 3: loose */
        ipv6_urpf_mode : 2;                    /* 0: none, 1: strict, 3: loose */
        urpf_mode : 2;                         /* urpf mode for current lookup */
        nat_mode : 2;                          /* 0: none, 1: inside, 2: outside */
        rmac_group : 10;                       /* Rmac group, for rmac indirection */
        rmac_hit : 1;                          /* dst mac is the router's mac */
        mcast_route_hit : 1;                   /* hit in the multicast route table */
        mcast_bridge_hit : 1;                  /* hit in the multicast bridge table */
        bd_mrpf_group : BD_BIT_WIDTH;          /* rpf group from bd lookup */
        mcast_rpf_group : BD_BIT_WIDTH;        /* rpf group from mcast lookup */
        mcast_mode : 2;                        /* multicast mode from route */
        uuc_mc_index : 16;                     /* unknown unicast multicast index */
        umc_mc_index : 16;                     /* unknown multicast multicast index */
        bcast_mc_index : 16;                   /* broadcast multicast index */
        multicast_route_mc_index : 16;         /* multicast index from mfib */
        multicast_bridge_mc_index : 16;        /* multicast index from igmp/mld snoop */
        storm_control_color : 1;               /* 0 : pass, 1 : fail */
        urpf_hit : 1;                          /* hit in urpf table */
        urpf_check_fail :1;                    /* urpf check failed */
        urpf_bd_group : BD_BIT_WIDTH;          /* urpf bd group */
        routed : 1;                            /* is packet routed */

        if_label : 16;                         /* if label for acls */
        bd_label : 16;                         /* bd label for acls */

        l2_src_miss : 1;                       /* l2 source miss */
        l2_src_move : IFINDEX_BIT_WIDTH;       /* l2 source interface mis-match */
        ipsg_check_fail : 1;                   /* ipsg check failed */
        acl_deny : 1;                          /* ifacl/vacl deny action */
        racl_deny : 1;                         /* racl deny action */
        l2_redirect : 1;                       /* l2 redirect action */
        acl_redirect :   1;                    /* ifacl/vacl redirect action */
        racl_redirect : 1;                     /* racl redirect action */
        fib_hit : 1;                           /* fib hit */
        nat_hit : 1;                           /* fwd and rewrite info from nat */

        mirror_session_id : 10;                /* mirror session id */

        l2_nexthop : 16;                       /* next hop from l2 */
        acl_nexthop : 16;                      /* next hop from ifacl/vacl */
        racl_nexthop : 16;                     /* next hop from racl */
        fib_nexthop : 16;                      /* next hop from fib */
        nat_nexthop : 16;                      /* next hop from nat */
        l2_nexthop_type : 1;                   /* ecmp or nexthop */
        acl_nexthop_type : 1;                  /* ecmp or nexthop */
        racl_nexthop_type : 1;                 /* ecmp or nexthop */
        fib_nexthop_type : 1;                  /* ecmp or nexthop */
        nexthop_index : 16;                    /* final next hop index */
        nexthop_type : 1;                      /* final next hop index type */
        nat_rewrite_index : 16;                /* NAT rewrite index */

        marked_cos : 3;                        /* marked vlan cos value */
        marked_dscp : 8;                       /* marked dscp value */
        marked_exp : 3;                        /* marked exp value */
        ttl : 8;                               /* update ttl */

#ifndef HARLYN
        egress_ifindex : IFINDEX_BIT_WIDTH;    /* egress interface index */
#else
        egress_ifindex : 32;                   /* egress interface index */
#endif
        egress_bd : BD_BIT_WIDTH;              /* egress BD */
        same_bd_check : BD_BIT_WIDTH;          /* ingress bd xor egress bd */

        ingress_bypass : 1;                    /* skip the entire ingress pipeline */
        ipv4_dstaddr_24b : 24;                 /* first 24b of ipv4 dst addr */
        drop_0 : 1;                            /* dummy */
        drop_reason : 8;                       /* drop reason for negative mirroring */
        stp_group: 10;                         /* spanning tree group id */
        stp_state : 3;                         /* spanning tree port state */
        control_frame: 1;                      /* control frame */
        header_count: 4;                       /* Number of headers */
    }
}

metadata ingress_metadata_t ingress_metadata;

table port_vlan_mapping {
    reads {
        ingress_metadata.ifindex : exact;
        vlan_tag_[0] : valid;
        vlan_tag_[0].vid : exact;
        vlan_tag_[1] : valid;
        vlan_tag_[1].vid : exact;
    }

    action_profile: outer_bd_action_profile;
    size : PORT_VLAN_TABLE_SIZE;
}

action set_outer_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode, stp_group) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(ingress_metadata.outer_ipv4_mcast_key_type, OUTER_MCAST_KEY_TYPE_BD);
    modify_field(ingress_metadata.outer_ipv4_mcast_key, bd);
    modify_field(ingress_metadata.outer_ipv6_mcast_key_type, OUTER_MCAST_KEY_TYPE_BD);
    modify_field(ingress_metadata.outer_ipv6_mcast_key, bd);

    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ingress_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(ingress_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ingress_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ingress_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.stp_group, stp_group);
}

action set_outer_bd_ipv4_mcast_switch_ipv6_mcast_route_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode, stp_group) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(ingress_metadata.outer_ipv4_mcast_key_type, OUTER_MCAST_KEY_TYPE_BD);
    modify_field(ingress_metadata.outer_ipv4_mcast_key, bd);
    modify_field(ingress_metadata.outer_ipv6_mcast_key_type, OUTER_MCAST_KEY_TYPE_VRF);
    modify_field(ingress_metadata.outer_ipv6_mcast_key, vrf);

    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ingress_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(ingress_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ingress_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ingress_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.stp_group, stp_group);
}

action set_outer_bd_ipv4_mcast_route_ipv6_mcast_switch_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode, stp_group) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(ingress_metadata.outer_ipv4_mcast_key_type, OUTER_MCAST_KEY_TYPE_VRF);
    modify_field(ingress_metadata.outer_ipv4_mcast_key, vrf);
    modify_field(ingress_metadata.outer_ipv6_mcast_key_type, OUTER_MCAST_KEY_TYPE_BD);
    modify_field(ingress_metadata.outer_ipv6_mcast_key, bd);

    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ingress_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(ingress_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ingress_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ingress_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.stp_group, stp_group);
}

action set_outer_bd_ipv4_mcast_route_ipv6_mcast_route_flags(bd, vrf,
        rmac_group, mrpf_group,
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, ipv6_unicast_enabled,
        ipv4_multicast_mode, ipv6_multicast_mode,
        igmp_snooping_enabled, mld_snooping_enabled,
        ipv4_urpf_mode, ipv6_urpf_mode, stp_group) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.bd, bd);
    modify_field(ingress_metadata.outer_bd, bd);
    modify_field(ingress_metadata.outer_ipv4_mcast_key_type, OUTER_MCAST_KEY_TYPE_VRF);
    modify_field(ingress_metadata.outer_ipv4_mcast_key, vrf);
    modify_field(ingress_metadata.outer_ipv6_mcast_key_type, OUTER_MCAST_KEY_TYPE_VRF);
    modify_field(ingress_metadata.outer_ipv6_mcast_key, vrf);

    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ingress_metadata.ipv4_multicast_mode, ipv4_multicast_mode);
    modify_field(ingress_metadata.ipv6_multicast_mode, ipv6_multicast_mode);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.mld_snooping_enabled, mld_snooping_enabled);
    modify_field(ingress_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(ingress_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.bd_mrpf_group, mrpf_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.stp_group, stp_group);
}

action set_bd(outer_vlan_bd, vrf, rmac_group, 
        bd_label, uuc_mc_index, bcast_mc_index, umc_mc_index,
        ipv4_unicast_enabled, 
        igmp_snooping_enabled, stp_group) {
    modify_field(ingress_metadata.vrf, vrf);
    modify_field(ingress_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ingress_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
    modify_field(ingress_metadata.rmac_group, rmac_group);
    modify_field(ingress_metadata.uuc_mc_index, uuc_mc_index);
    modify_field(ingress_metadata.umc_mc_index, umc_mc_index);
    modify_field(ingress_metadata.bcast_mc_index, bcast_mc_index);
    modify_field(ingress_metadata.bd_label, bd_label);
    modify_field(ingress_metadata.bd, outer_vlan_bd);
    modify_field(ingress_metadata.stp_group, stp_group);
}

action_profile outer_bd_action_profile {
    actions {
        /*
         * This is the default miss action if the outer bd is not found in this
         * table.
         */
        set_bd;
        set_outer_bd_ipv4_mcast_switch_ipv6_mcast_switch_flags;
        set_outer_bd_ipv4_mcast_switch_ipv6_mcast_route_flags;
        set_outer_bd_ipv4_mcast_route_ipv6_mcast_switch_flags;
        set_outer_bd_ipv4_mcast_route_ipv6_mcast_route_flags;
    }
    size : OUTER_BD_TABLE_SIZE;
}

control ingress {

        apply(port_vlan_mapping);

}
