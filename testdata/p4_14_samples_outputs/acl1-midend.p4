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
    bit<1>   _acl_metadata_acl_deny0;
    bit<1>   _acl_metadata_racl_deny1;
    bit<16>  _acl_metadata_acl_nexthop2;
    bit<16>  _acl_metadata_racl_nexthop3;
    bit<1>   _acl_metadata_acl_nexthop_type4;
    bit<1>   _acl_metadata_racl_nexthop_type5;
    bit<1>   _acl_metadata_acl_redirect6;
    bit<1>   _acl_metadata_racl_redirect7;
    bit<15>  _acl_metadata_if_label8;
    bit<16>  _acl_metadata_bd_label9;
    bit<10>  _acl_metadata_mirror_session_id10;
    bit<3>   _fabric_metadata_packetType11;
    bit<1>   _fabric_metadata_fabric_header_present12;
    bit<16>  _fabric_metadata_reason_code13;
    bit<9>   _ingress_metadata_ingress_port14;
    bit<16>  _ingress_metadata_ifindex15;
    bit<16>  _ingress_metadata_egress_ifindex16;
    bit<2>   _ingress_metadata_port_type17;
    bit<16>  _ingress_metadata_outer_bd18;
    bit<16>  _ingress_metadata_bd19;
    bit<1>   _ingress_metadata_drop_flag20;
    bit<8>   _ingress_metadata_drop_reason21;
    bit<1>   _ingress_metadata_control_frame22;
    bit<1>   _ingress_metadata_enable_dod23;
    bit<32>  _ipv4_metadata_lkp_ipv4_sa24;
    bit<32>  _ipv4_metadata_lkp_ipv4_da25;
    bit<1>   _ipv4_metadata_ipv4_unicast_enabled26;
    bit<2>   _ipv4_metadata_ipv4_urpf_mode27;
    bit<128> _ipv6_metadata_lkp_ipv6_sa28;
    bit<128> _ipv6_metadata_lkp_ipv6_da29;
    bit<1>   _ipv6_metadata_ipv6_unicast_enabled30;
    bit<1>   _ipv6_metadata_ipv6_src_is_link_local31;
    bit<2>   _ipv6_metadata_ipv6_urpf_mode32;
    bit<3>   _l2_metadata_lkp_pkt_type33;
    bit<48>  _l2_metadata_lkp_mac_sa34;
    bit<48>  _l2_metadata_lkp_mac_da35;
    bit<16>  _l2_metadata_lkp_mac_type36;
    bit<16>  _l2_metadata_l2_nexthop37;
    bit<1>   _l2_metadata_l2_nexthop_type38;
    bit<1>   _l2_metadata_l2_redirect39;
    bit<1>   _l2_metadata_l2_src_miss40;
    bit<16>  _l2_metadata_l2_src_move41;
    bit<10>  _l2_metadata_stp_group42;
    bit<3>   _l2_metadata_stp_state43;
    bit<16>  _l2_metadata_bd_stats_idx44;
    bit<1>   _l2_metadata_learning_enabled45;
    bit<1>   _l2_metadata_port_vlan_mapping_miss46;
    bit<16>  _l2_metadata_same_if_check47;
    bit<2>   _l3_metadata_lkp_ip_type48;
    bit<4>   _l3_metadata_lkp_ip_version49;
    bit<8>   _l3_metadata_lkp_ip_proto50;
    bit<8>   _l3_metadata_lkp_ip_tc51;
    bit<8>   _l3_metadata_lkp_ip_ttl52;
    bit<16>  _l3_metadata_lkp_l4_sport53;
    bit<16>  _l3_metadata_lkp_l4_dport54;
    bit<16>  _l3_metadata_lkp_inner_l4_sport55;
    bit<16>  _l3_metadata_lkp_inner_l4_dport56;
    bit<12>  _l3_metadata_vrf57;
    bit<10>  _l3_metadata_rmac_group58;
    bit<1>   _l3_metadata_rmac_hit59;
    bit<2>   _l3_metadata_urpf_mode60;
    bit<1>   _l3_metadata_urpf_hit61;
    bit<1>   _l3_metadata_urpf_check_fail62;
    bit<16>  _l3_metadata_urpf_bd_group63;
    bit<1>   _l3_metadata_fib_hit64;
    bit<16>  _l3_metadata_fib_nexthop65;
    bit<1>   _l3_metadata_fib_nexthop_type66;
    bit<16>  _l3_metadata_same_bd_check67;
    bit<16>  _l3_metadata_nexthop_index68;
    bit<1>   _l3_metadata_routed69;
    bit<1>   _l3_metadata_outer_routed70;
    bit<8>   _l3_metadata_mtu_index71;
    bit<16>  _l3_metadata_l3_mtu_check72;
    bit<1>   _security_metadata_storm_control_color73;
    bit<1>   _security_metadata_ipsg_enabled74;
    bit<1>   _security_metadata_ipsg_check_fail75;
    bit<5>   _tunnel_metadata_ingress_tunnel_type76;
    bit<24>  _tunnel_metadata_tunnel_vni77;
    bit<1>   _tunnel_metadata_mpls_enabled78;
    bit<20>  _tunnel_metadata_mpls_label79;
    bit<3>   _tunnel_metadata_mpls_exp80;
    bit<8>   _tunnel_metadata_mpls_ttl81;
    bit<5>   _tunnel_metadata_egress_tunnel_type82;
    bit<14>  _tunnel_metadata_tunnel_index83;
    bit<9>   _tunnel_metadata_tunnel_src_index84;
    bit<9>   _tunnel_metadata_tunnel_smac_index85;
    bit<14>  _tunnel_metadata_tunnel_dst_index86;
    bit<14>  _tunnel_metadata_tunnel_dmac_index87;
    bit<24>  _tunnel_metadata_vnid88;
    bit<1>   _tunnel_metadata_tunnel_terminate89;
    bit<1>   _tunnel_metadata_tunnel_if_check90;
    bit<4>   _tunnel_metadata_egress_header_count91;
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
        drop_stats_2.count((bit<32>)meta._ingress_metadata_drop_reason21);
    }
    @name(".nop") action nop() {
    }
    @name(".copy_to_cpu") action copy_to_cpu(bit<16> reason_code) {
        meta._fabric_metadata_reason_code13 = reason_code;
    }
    @name(".redirect_to_cpu") action redirect_to_cpu(bit<16> reason_code) {
        meta._fabric_metadata_reason_code13 = reason_code;
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
            meta._acl_metadata_if_label8                : ternary @name("acl_metadata.if_label") ;
            meta._acl_metadata_bd_label9                : ternary @name("acl_metadata.bd_label") ;
            meta._ipv4_metadata_lkp_ipv4_sa24           : ternary @name("ipv4_metadata.lkp_ipv4_sa") ;
            meta._ipv4_metadata_lkp_ipv4_da25           : ternary @name("ipv4_metadata.lkp_ipv4_da") ;
            meta._l3_metadata_lkp_ip_proto50            : ternary @name("l3_metadata.lkp_ip_proto") ;
            meta._l2_metadata_lkp_mac_sa34              : ternary @name("l2_metadata.lkp_mac_sa") ;
            meta._l2_metadata_lkp_mac_da35              : ternary @name("l2_metadata.lkp_mac_da") ;
            meta._l2_metadata_lkp_mac_type36            : ternary @name("l2_metadata.lkp_mac_type") ;
            meta._ingress_metadata_ifindex15            : ternary @name("ingress_metadata.ifindex") ;
            meta._l2_metadata_port_vlan_mapping_miss46  : ternary @name("l2_metadata.port_vlan_mapping_miss") ;
            meta._security_metadata_ipsg_check_fail75   : ternary @name("security_metadata.ipsg_check_fail") ;
            meta._acl_metadata_acl_deny0                : ternary @name("acl_metadata.acl_deny") ;
            meta._acl_metadata_racl_deny1               : ternary @name("acl_metadata.racl_deny") ;
            meta._l3_metadata_urpf_check_fail62         : ternary @name("l3_metadata.urpf_check_fail") ;
            meta._ingress_metadata_drop_flag20          : ternary @name("ingress_metadata.drop_flag") ;
            meta._l3_metadata_rmac_hit59                : ternary @name("l3_metadata.rmac_hit") ;
            meta._l3_metadata_routed69                  : ternary @name("l3_metadata.routed") ;
            meta._ipv6_metadata_ipv6_src_is_link_local31: ternary @name("ipv6_metadata.ipv6_src_is_link_local") ;
            meta._l2_metadata_same_if_check47           : ternary @name("l2_metadata.same_if_check") ;
            meta._tunnel_metadata_tunnel_if_check90     : ternary @name("tunnel_metadata.tunnel_if_check") ;
            meta._l3_metadata_same_bd_check67           : ternary @name("l3_metadata.same_bd_check") ;
            meta._l3_metadata_lkp_ip_ttl52              : ternary @name("l3_metadata.lkp_ip_ttl") ;
            meta._l2_metadata_stp_state43               : ternary @name("l2_metadata.stp_state") ;
            meta._ingress_metadata_control_frame22      : ternary @name("ingress_metadata.control_frame") ;
            meta._ipv4_metadata_ipv4_unicast_enabled26  : ternary @name("ipv4_metadata.ipv4_unicast_enabled") ;
            meta._ingress_metadata_egress_ifindex16     : ternary @name("ingress_metadata.egress_ifindex") ;
            meta._ingress_metadata_enable_dod23         : ternary @name("ingress_metadata.enable_dod") ;
        }
        size = 512;
        default_action = NoAction_3();
    }
    apply {
        system_acl_0.apply();
        if (meta._ingress_metadata_drop_flag20 == 1w1) {
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

