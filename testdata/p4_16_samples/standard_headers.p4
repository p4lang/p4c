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

#ifndef _STANDARD_HEADERS_P4_
#define _STANDARD_HEADERS_P4_

header ethernet_h
{
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    bit<32>      srcAddr;
    bit<32>      dstAddr;
}

header snap_header_h {
    bit<8> dsap;
    bit<8> ssap;
    bit<8> control_;
    bit<24> oui;
    bit<16> type_;
}

header roce_header_h {
    bit<320> ib_grh;
    bit<96> ib_bth;
}

header roce_v2_header_h {
    bit<96> ib_bth;
}

header fcoe_header_h {
    bit<4> version;
    bit<4> type_;
    bit<8> sof;
    bit<32> rsvd1;
    bit<32> ts_upper;
    bit<32> ts_lower;
    bit<32> size_;
    bit<8> eof;
    bit<24> rsvd2;
}

header vlan_tag_h {
    bit<3> pcp;
    bit<1> cfi;
    bit<12> vid;
    bit<16> etherType;
}

header vlan_tag_3b_h {
    bit<3> pcp;
    bit<1> cfi;
    bit<4> vid;
    bit<16> etherType;
}

header vlan_tag_5b_h {
    bit<3> pcp;
    bit<1> cfi;
    bit<20> vid;
    bit<16> etherType;
}

header ieee802_1ah_h {
    bit<3> pcp;
    bit<1> dei;
    bit<1> uca;
    bit<3> reserved;
    bit<24> i_sid;
}

header mpls_h {
    bit<20> label;
    bit<3> tc;
    bit<1> bos;
    bit<8> ttl;
}

header ipv6_h {
    bit<4> version;
    bit<8> trafficClass;
    bit<20> flowLabel;
    bit<16> payloadLen;
    bit<8> nextHdr;
    bit<8> hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header icmp_h {
    bit<8> type_;
    bit<8> code;
    bit<16> hdrChecksum;
}

header icmpv6_h {
    bit<8> type_;
    bit<8> code;
    bit<16> hdrChecksum;
}

header tcp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4> dataOffset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header sctp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> verifTag;
    bit<32> checksum;
}

header gre_h {
    bit<1> C;
    bit<1> R;
    bit<1> K;
    bit<1> S;
    bit<1> s;
    bit<3> recurse;
    bit<5> flags;
    bit<3> ver;
    bit<16> proto;
}

header nvgre_h {
    bit<24> tni;
    bit<8> reserved;
}

/* 8 bytes */
header erspan_header_v1_h {
    bit<4> version;
    bit<12> vlan;
    bit<6> priority;
    bit<10> span_id;
    bit<8> direction;
    bit<8> truncated;
}

/* 8 bytes */
header erspan_header_v2_h {
    bit<4> version;
    bit<12> vlan;
    bit<6> priority;
    bit<10> span_id;
    bit<32> unknown7;
}

header ipsec_esp_h {
    bit<32> spi;
    bit<32> seqNo;
}

header ipsec_ah_h {
    bit<8> nextHdr;
    bit<8> length_;
    bit<16> zero;
    bit<32> spi;
    bit<32> seqNo;
}

header arp_rarp_h {
    bit<16> hwType;
    bit<16> protoType;
    bit<8> hwAddrLen;
    bit<8> protoAddrLen;
    bit<16> opcode;
}

header arp_rarp_ipv4_h {
    bit<48> srcHwAddr;
    bit<32> srcProtoAddr;
    bit<48> dstHwAddr;
    bit<32> dstProtoAddr;
}

header eompls_h {
    bit<4> zero;
    bit<12> reserved;
    bit<16> seqNo;
}

header vxlan_h {
    bit<8> flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8> reserved2;
}

header nsh_h {
    bit<1> oam;
    bit<1> context;
    bit<6> flags;
    bit<8> reserved;
    bit<16> protoType;
    bit<24> spath;
    bit<8> sindex;
}

header nsh_context_h {
    bit<32> network_platform;
    bit<32> network_shared;
    bit<32> service_platform;
    bit<32> service_shared;
}

/* GENEVE HEADERS
   3 possible options with known type, known length */

header genv_h {
    bit<2> ver;
    bit<6> optLen;
    bit<1> oam;
    bit<1> critical;
    bit<6> reserved;
    bit<16> protoType;
    bit<24> vni;
    bit<8> reserved2;
}

#define GENV_OPTION_A_TYPE 0x000001
/* TODO: Would it be convenient to have some kind of sizeof macro ? */
#define GENV_OPTION_A_LENGTH 2 /* in bytes */

header genv_opt_A_h {
    bit<16> optClass;
    bit<8> optType;
    bit<3> reserved;
    bit<5> optLen;
    bit<32> dt;
}

#define GENV_OPTION_B_TYPE 0x000002
#define GENV_OPTION_B_LENGTH 3 /* in bytes */

header genv_opt_B_h {
    bit<16> optClass;
    bit<8> optType;
    bit<3> reserved;
    bit<5> optLen;
    bit<64> dt;
}

#define GENV_OPTION_C_TYPE 0x000003
#define GENV_OPTION_C_LENGTH 2 /* in bytes */

header genv_opt_C_h {
    bit<16> optClass;
    bit<8> optType;
    bit<3> reserved;
    bit<5> optLen;
    bit<32> dt;
}
#endif
