#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct hop_metadata_t {
    bit<12> vrf;
    bit<64> ipv6_prefix;
    bit<16> next_hop_index;
    bit<16> mcast_grp;
    bit<1>  urpf_fail;
    bit<8>  drop_reason;
}

header arp_rarp_t {
    bit<16> hwType;
    bit<16> protoType;
    bit<8>  hwAddrLen;
    bit<8>  protoAddrLen;
    bit<16> opcode;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header gre_t {
    bit<1>  C;
    bit<1>  R;
    bit<1>  K;
    bit<1>  S;
    bit<1>  s;
    bit<3>  recurse;
    bit<5>  flags;
    bit<3>  ver;
    bit<16> proto;
}

header icmp_t {
    bit<8>  type_;
    bit<8>  code;
    bit<16> hdrChecksum;
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

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header mpls_t {
    bit<20> label;
    bit<3>  exp;
    bit<1>  bos;
    bit<8>  ttl;
}

header vlan_tag_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct metadata {
    @name(".hop_metadata")
    hop_metadata_t hop_metadata;
}

struct headers {
    @name(".arp_rarp")
    arp_rarp_t    arp_rarp;
    @name(".ethernet")
    ethernet_t    ethernet;
    @name(".gre")
    gre_t         gre;
    @name(".icmp")
    icmp_t        icmp;
    @name(".ipv4")
    ipv4_t        ipv4;
    @name(".ipv6")
    ipv6_t        ipv6;
    @name(".tcp")
    tcp_t         tcp;
    @name(".udp")
    udp_t         udp;
    @name(".mpls")
    mpls_t[1]     mpls;
    @name(".vlan_tag_")
    vlan_tag_t[1] vlan_tag_;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_arp_rarp") state parse_arp_rarp {
        packet.extract(hdr.arp_rarp);
        transition accept;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x8035: parse_arp_rarp;
            default: accept;
        }
    }
    @name(".parse_gre") state parse_gre {
        packet.extract(hdr.gre);
        transition accept;
    }
    @name(".parse_icmp") state parse_icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.fragOffset, hdr.ipv4.ihl, hdr.ipv4.protocol) {
            (13w0x0, 4w0x5, 8w0x1): parse_icmp;
            (13w0x0, 4w0x5, 8w0x6): parse_tcp;
            (13w0x0, 4w0x5, 8w0x11): parse_udp;
            (13w0x0, 4w0x5, 8w0x2f): parse_gre;
            default: accept;
        }
    }
    @name(".parse_ipv6") state parse_ipv6 {
        packet.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            8w58: parse_icmp;
            8w6: parse_tcp;
            8w17: parse_udp;
            8w47: parse_gre;
            default: accept;
        }
    }
    @name(".parse_mpls") state parse_mpls {
        packet.extract(hdr.mpls.next);
        transition select(hdr.mpls.last.bos) {
            1w0: parse_mpls;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
    @name(".parse_udp") state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract(hdr.vlan_tag_.next);
        transition select(hdr.vlan_tag_.last.etherType) {
            16w0x8100: parse_vlan;
            16w0x9100: parse_vlan;
            16w0x8847: parse_mpls;
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp_rarp;
            16w0x8035: parse_arp_rarp;
            default: accept;
        }
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control process_ip(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".action_drop") action action_drop(bit<8> drop_reason) {
        meta.hop_metadata.drop_reason = drop_reason;
        mark_to_drop(standard_metadata);
    }
    @name(".on_hit") action on_hit() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".nop") action nop() {
    }
    @name(".set_next_hop") action set_next_hop(bit<16> dst_index) {
        meta.hop_metadata.next_hop_index = dst_index;
    }
    @name(".set_multicast_replication_list") action set_multicast_replication_list(bit<16> mc_index) {
        meta.hop_metadata.mcast_grp = mc_index;
    }
    @name(".set_ipv6_prefix_ucast") action set_ipv6_prefix_ucast(bit<64> ipv6_prefix) {
        meta.hop_metadata.ipv6_prefix = ipv6_prefix;
    }
    @name(".set_ipv6_prefix_xcast") action set_ipv6_prefix_xcast(bit<64> ipv6_prefix) {
        meta.hop_metadata.ipv6_prefix = ipv6_prefix;
    }
    @name(".set_ethernet_addr") action set_ethernet_addr(bit<48> smac, bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }
    @name(".set_urpf_check_fail") action set_urpf_check_fail() {
        meta.hop_metadata.urpf_fail = 1w1;
    }
    @name(".urpf_check_fail") action urpf_check_fail() {
        set_urpf_check_fail();
        mark_to_drop(standard_metadata);
    }
    @name(".set_vrf") action set_vrf(bit<12> vrf) {
        meta.hop_metadata.vrf = vrf;
    }
    @name(".acl") table acl {
        actions = {
            action_drop;
        }
        key = {
            hdr.ipv4.srcAddr    : lpm;
            hdr.ipv4.dstAddr    : lpm;
            hdr.ethernet.srcAddr: exact;
            hdr.ethernet.dstAddr: exact;
        }
        size = 80000;
    }
    @name(".check_ipv6") table check_ipv6 {
        actions = {
            on_hit;
            on_miss;
        }
        key = {
            hdr.ipv6.isValid(): exact;
        }
        size = 1;
    }
    @name(".check_ucast_ipv4") table check_ucast_ipv4 {
        actions = {
            nop;
        }
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        size = 1;
    }
    @name(".igmp_snooping") table igmp_snooping {
        actions = {
            nop;
        }
        key = {
            hdr.ipv4.dstAddr              : lpm;
            hdr.vlan_tag_[0].vid          : exact;
            standard_metadata.ingress_port: exact;
        }
        size = 16000;
    }
    @name(".ipv4_forwarding") table ipv4_forwarding {
        actions = {
            set_next_hop;
        }
        key = {
            meta.hop_metadata.vrf: exact;
            hdr.ipv4.dstAddr     : lpm;
        }
        size = 160000;
    }
    @name(".ipv4_xcast_forwarding") table ipv4_xcast_forwarding {
        actions = {
            set_multicast_replication_list;
        }
        key = {
            hdr.vlan_tag_[0].vid          : exact;
            hdr.ipv4.dstAddr              : lpm;
            hdr.ipv4.srcAddr              : lpm;
            standard_metadata.ingress_port: exact;
        }
        size = 16000;
    }
    @name(".ipv6_forwarding") table ipv6_forwarding {
        actions = {
            set_next_hop;
        }
        key = {
            meta.hop_metadata.vrf        : exact;
            meta.hop_metadata.ipv6_prefix: exact;
            hdr.ipv6.dstAddr             : lpm;
        }
        size = 5000;
    }
    @name(".ipv6_prefix") table ipv6_prefix {
        actions = {
            set_ipv6_prefix_ucast;
            set_ipv6_prefix_xcast;
        }
        key = {
            hdr.ipv6.dstAddr: lpm;
        }
        size = 1000;
    }
    @name(".ipv6_xcast_forwarding") table ipv6_xcast_forwarding {
        actions = {
            set_multicast_replication_list;
        }
        key = {
            hdr.vlan_tag_[0].vid          : exact;
            hdr.ipv6.dstAddr              : lpm;
            hdr.ipv6.srcAddr              : lpm;
            standard_metadata.ingress_port: exact;
        }
        size = 1000;
    }
    @name(".next_hop") table next_hop {
        actions = {
            set_ethernet_addr;
        }
        key = {
            meta.hop_metadata.next_hop_index: exact;
        }
        size = 41250;
    }
    @name(".urpf_v4") table urpf_v4 {
        actions = {
            urpf_check_fail;
            nop;
        }
        key = {
            hdr.vlan_tag_[0].vid          : exact;
            standard_metadata.ingress_port: exact;
            hdr.ipv4.srcAddr              : exact;
        }
        size = 160000;
    }
    @name(".urpf_v6") table urpf_v6 {
        actions = {
            urpf_check_fail;
            nop;
        }
        key = {
            hdr.vlan_tag_[0].vid          : exact;
            standard_metadata.ingress_port: exact;
            hdr.ipv6.srcAddr              : exact;
        }
        size = 5000;
    }
    @name(".vrf") table vrf {
        actions = {
            set_vrf;
        }
        key = {
            hdr.vlan_tag_[0].isValid()    : exact;
            hdr.vlan_tag_[0].vid          : exact;
            hdr.ethernet.srcAddr          : exact;
            standard_metadata.ingress_port: exact;
        }
        size = 40000;
    }
    apply {
        vrf.apply();
        switch (check_ipv6.apply().action_run) {
            on_hit: {
                switch (ipv6_prefix.apply().action_run) {
                    set_ipv6_prefix_ucast: {
                        urpf_v6.apply();
                        ipv6_forwarding.apply();
                    }
                    set_ipv6_prefix_xcast: {
                        ipv6_xcast_forwarding.apply();
                    }
                }
            }
            on_miss: {
                switch (check_ucast_ipv4.apply().action_run) {
                    on_hit: {
                        urpf_v4.apply();
                        ipv4_forwarding.apply();
                    }
                    on_miss: {
                        igmp_snooping.apply();
                        ipv4_xcast_forwarding.apply();
                    }
                }
            }
        }
        acl.apply();
        next_hop.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_egress_port") action set_egress_port(bit<9> e_port) {
        standard_metadata.egress_spec = e_port;
    }
    @name(".on_hit") action on_hit() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".nop") action nop() {
    }
    @name(".dmac_vlan") table dmac_vlan {
        actions = {
            set_egress_port;
        }
        key = {
            hdr.ethernet.dstAddr          : exact;
            hdr.vlan_tag_[0].vid          : exact;
            standard_metadata.ingress_port: exact;
        }
        size = 160000;
    }
    @name(".routable") table routable {
        actions = {
            on_hit;
            on_miss;
        }
        key = {
            hdr.vlan_tag_[0].isValid()    : exact;
            hdr.vlan_tag_[0].vid          : exact;
            standard_metadata.ingress_port: exact;
            hdr.ethernet.dstAddr          : exact;
            hdr.ethernet.etherType        : exact;
        }
        size = 64;
    }
    @name(".smac_vlan") table smac_vlan {
        actions = {
            nop;
        }
        key = {
            hdr.ethernet.srcAddr          : exact;
            standard_metadata.ingress_port: exact;
        }
        size = 160000;
    }
    @name(".process_ip") process_ip() process_ip_0;
    apply {
        smac_vlan.apply();
        switch (routable.apply().action_run) {
            on_hit: {
                process_ip_0.apply(hdr, meta, standard_metadata);
            }
        }
        dmac_vlan.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.vlan_tag_);
        packet.emit(hdr.arp_rarp);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.gre);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        packet.emit(hdr.icmp);
        packet.emit(hdr.mpls);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
