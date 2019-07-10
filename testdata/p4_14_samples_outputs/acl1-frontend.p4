#include <core.p4>
#include <v1model.p4>

struct acl_metadata_t {
    bit<1>  acl_deny;
    bit<1>  racl_deny;
    bit<16> acl_nexthop;
    bit<16> racl_nexthop;
    bit<1>  acl_nexthop_type;
    bit<1>  racl_nexthop_type;
    bit<1>  acl_redirect;
    bit<1>  racl_redirect;
    bit<15> if_label;
    bit<16> bd_label;
    bit<10> mirror_session_id;
}

struct fabric_metadata_t {
    bit<3>  packetType;
    bit<1>  fabric_header_present;
    bit<16> reason_code;
}

struct ingress_metadata_t {
    bit<9>  ingress_port;
    bit<16> ifindex;
    bit<16> egress_ifindex;
    bit<2>  port_type;
    bit<16> outer_bd;
    bit<16> bd;
    bit<1>  drop_flag;
    bit<8>  drop_reason;
    bit<1>  control_frame;
    bit<1>  enable_dod;
}

struct ipv4_metadata_t {
    bit<32> lkp_ipv4_sa;
    bit<32> lkp_ipv4_da;
    bit<1>  ipv4_unicast_enabled;
    bit<2>  ipv4_urpf_mode;
}

struct ipv6_metadata_t {
    bit<128> lkp_ipv6_sa;
    bit<128> lkp_ipv6_da;
    bit<1>   ipv6_unicast_enabled;
    bit<1>   ipv6_src_is_link_local;
    bit<2>   ipv6_urpf_mode;
}

struct l2_metadata_t {
    bit<3>  lkp_pkt_type;
    bit<48> lkp_mac_sa;
    bit<48> lkp_mac_da;
    bit<16> lkp_mac_type;
    bit<16> l2_nexthop;
    bit<1>  l2_nexthop_type;
    bit<1>  l2_redirect;
    bit<1>  l2_src_miss;
    bit<16> l2_src_move;
    bit<10> stp_group;
    bit<3>  stp_state;
    bit<16> bd_stats_idx;
    bit<1>  learning_enabled;
    bit<1>  port_vlan_mapping_miss;
    bit<16> same_if_check;
}

struct l3_metadata_t {
    bit<2>  lkp_ip_type;
    bit<4>  lkp_ip_version;
    bit<8>  lkp_ip_proto;
    bit<8>  lkp_ip_tc;
    bit<8>  lkp_ip_ttl;
    bit<16> lkp_l4_sport;
    bit<16> lkp_l4_dport;
    bit<16> lkp_inner_l4_sport;
    bit<16> lkp_inner_l4_dport;
    bit<12> vrf;
    bit<10> rmac_group;
    bit<1>  rmac_hit;
    bit<2>  urpf_mode;
    bit<1>  urpf_hit;
    bit<1>  urpf_check_fail;
    bit<16> urpf_bd_group;
    bit<1>  fib_hit;
    bit<16> fib_nexthop;
    bit<1>  fib_nexthop_type;
    bit<16> same_bd_check;
    bit<16> nexthop_index;
    bit<1>  routed;
    bit<1>  outer_routed;
    bit<8>  mtu_index;
    @saturating 
    bit<16> l3_mtu_check;
}

struct security_metadata_t {
    bit<1> storm_control_color;
    bit<1> ipsg_enabled;
    bit<1> ipsg_check_fail;
}

struct tunnel_metadata_t {
    bit<5>  ingress_tunnel_type;
    bit<24> tunnel_vni;
    bit<1>  mpls_enabled;
    bit<20> mpls_label;
    bit<3>  mpls_exp;
    bit<8>  mpls_ttl;
    bit<5>  egress_tunnel_type;
    bit<14> tunnel_index;
    bit<9>  tunnel_src_index;
    bit<9>  tunnel_smac_index;
    bit<14> tunnel_dst_index;
    bit<14> tunnel_dmac_index;
    bit<24> vnid;
    bit<1>  tunnel_terminate;
    bit<1>  tunnel_if_check;
    bit<4>  egress_header_count;
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

struct metadata {
    @pa_solitary("ingress", "acl_metadata.if_label") @name(".acl_metadata") 
    acl_metadata_t      acl_metadata;
    @name(".fabric_metadata") 
    fabric_metadata_t   fabric_metadata;
    @name(".ingress_metadata") 
    ingress_metadata_t  ingress_metadata;
    @name(".ipv4_metadata") 
    ipv4_metadata_t     ipv4_metadata;
    @name(".ipv6_metadata") 
    ipv6_metadata_t     ipv6_metadata;
    @name(".l2_metadata") 
    l2_metadata_t       l2_metadata;
    @name(".l3_metadata") 
    l3_metadata_t       l3_metadata;
    @name(".security_metadata") 
    security_metadata_t security_metadata;
    @name(".tunnel_metadata") 
    tunnel_metadata_t   tunnel_metadata;
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

@name(".drop_stats") counter(32w256, CounterType.packets) drop_stats;

@name(".drop_stats_2") counter(32w256, CounterType.packets) drop_stats_2;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".drop_stats_update") action drop_stats_update() {
        drop_stats_2.count((bit<32>)meta.ingress_metadata.drop_reason);
    }
    @name(".nop") action nop() {
    }
    @name(".copy_to_cpu") action copy_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
    }
    @name(".redirect_to_cpu") action redirect_to_cpu(bit<16> reason_code) {
        meta.fabric_metadata.reason_code = reason_code;
    }
    @name(".drop_packet") action drop_packet() {
    }
    @name(".drop_packet_with_reason") action drop_packet_with_reason(bit<32> drop_reason) {
        drop_stats.count(drop_reason);
    }
    @name(".negative_mirror") action negative_mirror(bit<8> session_id) {
    }
    @name(".congestion_mirror_set") action congestion_mirror_set() {
    }
    @name(".drop_stats") table drop_stats_1 {
        actions = {
            drop_stats_update();
            @defaultonly NoAction_0();
        }
        size = 256;
        default_action = NoAction_0();
    }
    @name(".system_acl") table system_acl_0 {
        actions = {
            nop();
            redirect_to_cpu();
            copy_to_cpu();
            drop_packet();
            drop_packet_with_reason();
            negative_mirror();
            congestion_mirror_set();
            @defaultonly NoAction_3();
        }
        key = {
            meta.acl_metadata.if_label               : ternary @name("acl_metadata.if_label") ;
            meta.acl_metadata.bd_label               : ternary @name("acl_metadata.bd_label") ;
            meta.ipv4_metadata.lkp_ipv4_sa           : ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta.ipv4_metadata.lkp_ipv4_da           : ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta.l3_metadata.lkp_ip_proto            : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta.l2_metadata.lkp_mac_sa              : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta.l2_metadata.lkp_mac_da              : ternary @name("l2_metadata.lkp_mac_da") ;
            meta.l2_metadata.lkp_mac_type            : ternary @name("l2_metadata.lkp_mac_type") ;
            meta.ingress_metadata.ifindex            : ternary @name("ingress_metadata.ifindex") ;
            meta.l2_metadata.port_vlan_mapping_miss  : ternary @name("l2_metadata.port_vlan_mapping_miss") ;
            meta.security_metadata.ipsg_check_fail   : ternary @name("security_metadata.ipsg_check_fail") ;
            meta.acl_metadata.acl_deny               : ternary @name("acl_metadata.acl_deny") ;
            meta.acl_metadata.racl_deny              : ternary @name("acl_metadata.racl_deny") ;
            meta.l3_metadata.urpf_check_fail         : ternary @name("l3_metadata.urpf_check_fail") ;
            meta.ingress_metadata.drop_flag          : ternary @name("ingress_metadata.drop_flag") ;
            meta.l3_metadata.rmac_hit                : ternary @name("l3_metadata.rmac_hit") ;
            meta.l3_metadata.routed                  : ternary @name("l3_metadata.routed") ;
            meta.ipv6_metadata.ipv6_src_is_link_local: ternary @name("ipv6_metadata.ipv6_src_is_link_local") ;
            meta.l2_metadata.same_if_check           : ternary @name("l2_metadata.same_if_check") ;
            meta.tunnel_metadata.tunnel_if_check     : ternary @name("tunnel_metadata.tunnel_if_check") ;
            meta.l3_metadata.same_bd_check           : ternary @name("l3_metadata.same_bd_check") ;
            meta.l3_metadata.lkp_ip_ttl              : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta.l2_metadata.stp_state               : ternary @name("l2_metadata.stp_state") ;
            meta.ingress_metadata.control_frame      : ternary @name("ingress_metadata.control_frame") ;
            meta.ipv4_metadata.ipv4_unicast_enabled  : ternary @name("ipv4_metadata.ipv4_unicast_enabled") ;
            meta.ingress_metadata.egress_ifindex     : ternary @name("ingress_metadata.egress_ifindex") ;
            meta.ingress_metadata.enable_dod         : ternary @name("ingress_metadata.enable_dod") ;
        }
        size = 512;
        default_action = NoAction_3();
    }
    apply {
        system_acl_0.apply();
        if (meta.ingress_metadata.drop_flag == 1w1) {
            drop_stats_1.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

