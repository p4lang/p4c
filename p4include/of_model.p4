/*
Copyright 2022 VMWare, Inc.

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

This file describes a P4 architectural model for Open vSwitch.  To use
it with Open vSwitch, use the ofp4 gateway.

For complete details on the fields and metadata described here, please
read the Open vSwitch protocol header fields manual available at
http://www.openvswitch.org/support/dist-docs/ovs-fields.7.pdf

The p4c-of backend used with this model uses annotations as follows:

  - For fields, the @name annotation is the OpenFlow name of the field.

  - For fields, the @of_slice(low:high:size) annotation says that the
    field is a slice of 'size'-bit @name from bit 'low' to bit 'high',
    inclusive.

  - On a field or a header, @of_prereq specifies an extra clause to
    add to the OpenFlow match, for supplying a prerequisite.  On a
    header, @of_prereq provides a prerequisite for all its fields.
*/

#ifndef _OF_MODEL_P4
#define _OF_MODEL_P4

#include <core.p4>

/* An OpenFlow port number.  OpenFlow 1.1 and later adopted a 32-bit port
 * number, but OVS forces ports to be in the 16-bit range, so 16 bits is still
 * sufficient. */
typedef bit<16>  PortID;

/* OpenFlow reserved port numbers.  These have little relevance in a P4
   context. */
const PortID OFPP_UNSET      = 0xfff7; /* For OXM_OF_ACTSET_OUTPUT only. */
const PortID OFPP_IN_PORT    = 0xfff8; /* Where the packet came in. */
const PortID OFPP_TABLE      = 0xfff9; /* Perform actions in flow table. */
const PortID OFPP_NORMAL     = 0xfffa; /* Process with normal L2/L3. */
const PortID OFPP_FLOOD      = 0xfffb; /* All ports except input port and
                                        * ports disabled by STP. */
const PortID OFPP_ALL        = 0xfffc; /* All ports except input port. */
const PortID OFPP_CONTROLLER = 0xfffd; /* Send to controller. */
const PortID OFPP_LOCAL      = 0xfffe; /* Local openflow "port". */
const PortID OFPP_NONE       = 0xffff; /* Not associated with any port. */

match_kind {
    optional
}

/* Tunnel metadata.  These are all-zero for packets that did not arrive in
 * a tunnel. */
struct Tunnel {
    bit<64> tun_id;             /* VXLAN VNI, GRE key, Geneve VNI, ... */
    bit<32> tun_src;            /* Outer IPv4 source address. */
    bit<32> tun_dst;            /* Outer IPv4 destination address. */
    bit<128> tun_ipv6_src;      /* Outer IPv6 source address. */
    bit<128> tun_ipv6_dst;      /* Outer IPv6 destination address. */
    bit<16> tun_gbp_id;         /* VXLAN Group Based Policy ID. */
    bit<8> tun_gbp_flags;       /* VXLAN Group Based Policy flags. */
    bit<8> tun_erspan_ver;      /* ERSPAN version number (low 4 bits). */
    bit<32> tun_erspan_idx;     /* ERSPAN index (low 20 bits). */
    bit<8> tun_erspan_dir;      /* ERSPAN direction (low bit only). */
    bit<8> tun_erspan_hwid;     /* ERSPAN ERSPAN engine ID (low 6 bits). */
    bit<8> tun_gtpu_flags;      /* GTP-U flags. */
    bit<8> tun_gtpu_msgtype;    /* GTP-U message type. */
    bit<16> tun_flags;          /* Tunnel flags (low bit only). */

    /* Access to Geneve tunneling TLV options. */
    bit<992> tun_metadata0;
    /* ... */
    bit<992> tun_metadata63;
}

/* Flags for Conntrack.ct_state. */
const bit<32> CS_NEW  = 1 << 0; // 1 for an uncommitted connection.
const bit<32> CS_EST  = 1 << 1; // 1 for an established connection.
const bit<32> CS_REL  = 1 << 2; // 1 for packet relating to established connection.
const bit<32> CS_RPL  = 1 << 3; // 1 for packet in reply direction.
const bit<32> CS_INV  = 1 << 4; // 1 for invalid packet.
const bit<32> CS_TRK  = 1 << 5; // 1 for tracked packet.
const bit<32> CS_SNAT = 1 << 6; // 1 if packet was already SNATed.
const bit<32> CS_DNAT = 1 << 7; // 1 if packet awas already DNATed.

/* Connection-tracking metadata.
 *
 * All of these fields are read-only, but connection-tracking actions update
 * them.
 */
struct Conntrack {
    bit<32> ct_state;           // CS_*.
    bit<16> ct_zone;            // Connection-tracking zone.
    bit<32> ct_mark;            // Arbitrary metadata.
    bit<128> ct_label;          // More arbitrary metadata.

    /* The following fields require a match to a valid connection tracking state
     * as a prerequisite, in addition to the IP or IPv6 ethertype
     * match. Examples of valid connection tracking state matches include
     * ct_state=+new, ct_state=+est, ct_state=+rel, and ct_state=+trk-inv. */
    bit<32> ct_nw_src;
    bit<32> ct_nw_dst;
    bit<128> ct_ipv6_src;
    bit<128> ct_ipv6_dst;
    bit<8> ct_nw_proto;
    bit<16> ct_tp_src;
    bit<16> ct_tp_dst;
}

/* Metadata fields.  These are always present for every packet. */
struct input_metadata_t {
    /* OVS metadata fields. */
    PortID in_port;             /* Ingress port. */
    bit<32> skb_priority;       /* Linux packet scheduling class. */
    bit<32> pkt_mark;           /* Linux kernel metadata. */
    bit<32> packet_type;        /* OpenFlow packet type.  0 for Ethernet. */
    Tunnel tunnel;
    Conntrack ct;
}

// Metadata passed from ingress to the architecture.
struct ingress_to_arch_t {
    /*
     * - If 'out_group' is nonzero, the packet is replicated and passed to the
     *   egress pipeline once for each member of the multicast group (as set
     *   through P4Runtime).
     */
    bit<16> out_group;  // 0 by default
    bool clone;   // false by default
}

struct output_metadata_t {
    /* P4 metadata fields used for both the ingress and the egress pipeline.
     * - If 'out_port' is zero, the packet is dropped.
     * (Port 0 is never a valid port number in OVS.)
     */
    PortID out_port;
}

header Ethernet {
    @name("dl_src")
    bit<48> src;

    @name("dl_dst")
    bit<48> dst;

    @name("dl_type")
    bit<16> type;
}

header Vlan {
    @name("vlan_tci") @of_slice(13,15,16)
    bit<3> pcp;

    @name("vlan_tci") @of_slice(12,12,16)
    bit<1> present;

    @name("vlan_tci") @of_slice(0,11,16)
    bit<12> vid;
}

@of_prereq("mpls")
header Mpls {
    @name("mpls_label")
    bit<32> label;              // Label (low 20 bits).

    @name("mpls_tc")
    bit<8> tc;                  // Traffic class (low 3 bits).

    @name("mpls_bos")
    bit<8> bos;                 // Bottom of Stack (low bit only).

    @name("mpls_ttl")
    bit<8> ttl;                 // Time to live.
}

const bit<8> FRAG_ANY = 1 << 0;   // Set for any IP fragment.
const bit<8> FRAG_LATER = 1 << 1; // Set for IP fragment with nonzero offset.

@of_prereq("ipv4")
header Ipv4 {
    @name("nw_src")
    bit<32> src;

    @name("nw_dst")
    bit<32> dst;

    @name("nw_proto")
    bit<8> proto;

    @name("nw_ttl")
    bit<8> ttl;

    @name("nw_frag")
    bit<8> frag;                // 0, or FRAG_ANY, or (FRAG_ANY | FRAG_LATER)

    @name("ip_dscp")
    bit<6> dscp;

    @name("ip_ecn")
    bit<2> ecn;
}

@of_prereq("ipv6")
header Ipv6 {
    @name("ipv6_src")
    bit<128> src;

    @name("ipv6_dst")
    bit<128> dst;

    @name("nw_proto")
    bit<8> proto;

    @name("nw_ttl")
    bit<8> ttl;

    @name("nw_frag")
    bit<8> frag;                // 0, or FRAG_ANY, or (FRAG_ANY | FRAG_LATER)

    @name("ip_dscp")
    bit<6> dscp;

    @name("ip_ecn")
    bit<2> ecn;
}

@of_prereq("arp")
header Arp {
    @name("arp_op")
    bit<16> op;

    @name("arp_spa")
    bit<32> spa;

    @name("arp_tpa")
    bit<32> tpa;

    @name("arp_sha")
    bit<48> sha;

    @name("arp_tha")
    bit<48> tha;
}

// Network Service Header (https://www.rfc-editor.org/rfc/rfc8300.html).
@of_prereq("nsh")
header Nsh {
    @name("nsh_flags")
    bit<8> flags;

    @name("nsh_ttl")
    bit<8> ttl;

    @name("nsh_mdtype")
    bit<8> mdtype;

    @name("nsh_np")
    bit<8> np;

    @name("nsh_spi")
    bit<32> spi;                // Low 24 bits only.

    @name("nsh_si")
    bit<8> si;

    @name("nsh_c1")
    bit<32> c1;

    @name("nsh_c2")
    bit<32> c2;

    @name("nsh_c3")
    bit<32> c3;

    @name("nsh_c4")
    bit<32> c4;
}

@of_prereq("tcp")		// XXX this is an IPv4 specific prerequisite, prehaps we need TcpV4 and TcpV6 headers instead.
struct Tcp {
    @name("tp_src")
    bit<16> src;

    @name("tp_dst")
    bit<16> dst;

    @name("tcp_flags")
    bit<16> flags;              // Low 12 bits only.
}

@of_prereq("udp")		// XXX this is an IPv4 specific prerequisite, prehaps we need UdpV4 and UdpV6 headers instead.
struct Udp {
    @name("tp_src")
    bit<16> src;

    @name("tp_dst")
    bit<16> dst;
}

@of_prereq("sctp")		// XXX this is an IPv4 specific prerequisite, prehaps we need SctpV4 and SctpV6 headers instead.
struct Sctp {
    @name("sctp_src")
    bit<16> src;

    @name("sctp_dst")
    bit<16> dst;
}

@of_prereq("icmp")
struct Icmp {
    @name("icmp_type")
    bit<8> type;

    @name("icmp_code")
    bit<8> code;
}

@of_prereq("icmpv6")
struct Icmpv6 {
    @name("icmpv6_type")
    bit<8> type;

    @name("icmpv6_code")
    bit<8> code;
}

// IPv6 neighbor discovery source link layer.
@of_prereq("icmpv6,icmpv6_type=135,icmpv6_code=0")
struct NdSll {
    @name("nd_target")
    bit<128> target;

    @name("nd_reserved")
    bit<32> reserved;

    @name("nd_options_type")
    bit<8> options_type;

    @name("nd_sll")
    bit<48> sll;
}

// IPv6 neighbor discovery target link layer.
@of_prereq("icmpv6,icmpv6_type=136,icmpv6_code=0")
struct NdTll {
    @name("nd_target")
    bit<128> target;

    @name("nd_reserved")
    bit<32> reserved;

    @name("nd_options_type")
    bit<8> options_type;

    @name("nd_tll")
    bit<48> tll;
}

struct Headers {
    Ethernet eth;
    Vlan vlan;
    Mpls mpls;
    Ipv4 ipv4;
    Ipv6 ipv6;
    Arp arp;
    Nsh nsh;
    Tcp tcp;
    Udp udp;
    Sctp sctp;
    Icmp icmp;
    Icmpv6 icmpv6;
    NdSll ndsll;
    NdTll ndtll;
}

control Ingress<M>(inout Headers hdr,
                   out M meta,
                   in input_metadata_t meta_in,
                   inout ingress_to_arch_t itoa,  // for architecture
                   inout output_metadata_t meta_out  // sent to egress
);

control Egress<M>(inout Headers hdr,
                  inout M meta,  // not used outside, but R/W this way
                  in input_metadata_t meta_in,
                  inout output_metadata_t meta_out
);

package OfSwitch<M>(Ingress<M> ig, Egress<M> eg);

#endif  /* _OF_MODEL_P4 */
