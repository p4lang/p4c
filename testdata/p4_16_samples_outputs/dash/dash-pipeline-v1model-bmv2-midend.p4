error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    InvalidIPv4Header
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header ipv4options_t {
    varbit<320> options;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

header nvgre_t {
    bit<4>  flags;
    bit<9>  reserved;
    bit<3>  version;
    bit<16> protocol_type;
    bit<24> vsid;
    bit<8>  flow_id;
}

header tcp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header ipv6_t {
    bit<4>   version;
    bit<8>   traffic_class;
    bit<20>  flow_label;
    bit<16>  payload_length;
    bit<8>   next_header;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
}

struct headers_t {
    ethernet_t    u1_ethernet;
    ipv4_t        u1_ipv4;
    ipv4options_t u1_ipv4options;
    ipv6_t        u1_ipv6;
    udp_t         u1_udp;
    tcp_t         u1_tcp;
    vxlan_t       u1_vxlan;
    nvgre_t       u1_nvgre;
    ethernet_t    u0_ethernet;
    ipv4_t        u0_ipv4;
    ipv4options_t u0_ipv4options;
    ipv6_t        u0_ipv6;
    udp_t         u0_udp;
    tcp_t         u0_tcp;
    vxlan_t       u0_vxlan;
    nvgre_t       u0_nvgre;
    ethernet_t    customer_ethernet;
    ipv4_t        customer_ipv4;
    ipv6_t        customer_ipv6;
    udp_t         customer_udp;
    tcp_t         customer_tcp;
}

struct conntrack_data_t {
    bool allow_in;
    bool allow_out;
}

struct eni_data_t {
    bit<32>  cps;
    bit<32>  pps;
    bit<32>  flows;
    bit<1>   admin_state;
    bit<128> pl_sip;
    bit<128> pl_sip_mask;
    bit<32>  pl_underlay_sip;
    bit<6>   dscp;
    bit<16>  dscp_mode;
}

struct encap_data_t {
    bit<24> vni;
    bit<24> dest_vnet_vni;
    bit<32> underlay_sip;
    bit<32> underlay_dip;
    bit<48> underlay_smac;
    bit<48> underlay_dmac;
    bit<16> dash_encapsulation;
}

struct overlay_rewrite_data_t {
    bool     is_ipv6;
    bit<48>  dmac;
    bit<128> sip;
    bit<128> dip;
    bit<128> sip_mask;
    bit<128> dip_mask;
}

struct metadata_t {
    bit<16>  _direction0;
    bit<48>  _eni_addr1;
    bit<16>  _vnet_id2;
    bit<16>  _dst_vnet_id3;
    bit<16>  _eni_id4;
    bit<32>  _eni_data_cps5;
    bit<32>  _eni_data_pps6;
    bit<32>  _eni_data_flows7;
    bit<1>   _eni_data_admin_state8;
    bit<128> _eni_data_pl_sip9;
    bit<128> _eni_data_pl_sip_mask10;
    bit<32>  _eni_data_pl_underlay_sip11;
    bit<6>   _eni_data_dscp12;
    bit<16>  _eni_data_dscp_mode13;
    bit<16>  _inbound_vm_id14;
    bit<8>   _appliance_id15;
    bit<1>   _is_overlay_ip_v616;
    bit<1>   _is_lkup_dst_ip_v617;
    bit<8>   _ip_protocol18;
    bit<128> _dst_ip_addr19;
    bit<128> _src_ip_addr20;
    bit<128> _lkup_dst_ip_addr21;
    bool     _conntrack_data_allow_in22;
    bool     _conntrack_data_allow_out23;
    bit<16>  _src_l4_port24;
    bit<16>  _dst_l4_port25;
    bit<16>  _stage1_dash_acl_group_id26;
    bit<16>  _stage2_dash_acl_group_id27;
    bit<16>  _stage3_dash_acl_group_id28;
    bit<16>  _stage4_dash_acl_group_id29;
    bit<16>  _stage5_dash_acl_group_id30;
    bit<1>   _meter_policy_en31;
    bit<1>   _mapping_meter_class_override32;
    bit<16>  _meter_policy_id33;
    bit<16>  _policy_meter_class34;
    bit<16>  _route_meter_class35;
    bit<16>  _mapping_meter_class36;
    bit<16>  _meter_class37;
    bit<32>  _meter_bucket_index38;
    bit<16>  _tunnel_pointer39;
    bool     _is_fast_path_icmp_flow_redirection_packet40;
    bit<1>   _fast_path_icmp_flow_redirection_disabled41;
    bit<16>  _target_stage42;
    bit<32>  _routing_actions43;
    bool     _dropped44;
    bit<24>  _encap_data_vni45;
    bit<24>  _encap_data_dest_vnet_vni46;
    bit<32>  _encap_data_underlay_sip47;
    bit<32>  _encap_data_underlay_dip48;
    bit<48>  _encap_data_underlay_smac49;
    bit<48>  _encap_data_underlay_dmac50;
    bit<16>  _encap_data_dash_encapsulation51;
    bool     _overlay_data_is_ipv652;
    bit<48>  _overlay_data_dmac53;
    bit<128> _overlay_data_sip54;
    bit<128> _overlay_data_dip55;
    bit<128> _overlay_data_sip_mask56;
    bit<128> _overlay_data_dip_mask57;
}

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, inout standard_metadata_t standard_meta) {
    state start {
        packet.extract<ethernet_t>(hd.u0_ethernet);
        transition select(hd.u0_ethernet.ether_type) {
            16w0x800: parse_u0_ipv4;
            16w0x86dd: parse_u0_ipv6;
            default: accept;
        }
    }
    state parse_u0_ipv4 {
        packet.extract<ipv4_t>(hd.u0_ipv4);
        verify(hd.u0_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.u0_ipv4.ihl >= 4w5, error.InvalidIPv4Header);
        transition select(hd.u0_ipv4.ihl) {
            4w5: dispatch_on_u0_protocol;
            default: parse_u0_ipv4options;
        }
    }
    state parse_u0_ipv4options {
        packet.extract<ipv4options_t>(hd.u0_ipv4options, (bit<32>)((bit<16>)hd.u0_ipv4.ihl + 16w65531 << 5));
        transition dispatch_on_u0_protocol;
    }
    state dispatch_on_u0_protocol {
        transition select(hd.u0_ipv4.protocol) {
            8w17: parse_u0_udp;
            8w6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_ipv6 {
        packet.extract<ipv6_t>(hd.u0_ipv6);
        transition select(hd.u0_ipv6.next_header) {
            8w17: parse_u0_udp;
            8w6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_udp {
        packet.extract<udp_t>(hd.u0_udp);
        transition select(hd.u0_udp.dst_port) {
            16w4789: parse_u0_vxlan;
            default: accept;
        }
    }
    state parse_u0_tcp {
        packet.extract<tcp_t>(hd.u0_tcp);
        transition accept;
    }
    state parse_u0_vxlan {
        packet.extract<vxlan_t>(hd.u0_vxlan);
        packet.extract<ethernet_t>(hd.customer_ethernet);
        transition select(hd.customer_ethernet.ether_type) {
            16w0x800: parse_customer_ipv4;
            16w0x86dd: parse_customer_ipv6;
            default: accept;
        }
    }
    state parse_customer_ipv4 {
        packet.extract<ipv4_t>(hd.customer_ipv4);
        verify(hd.customer_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.customer_ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hd.customer_ipv4.protocol) {
            8w17: parse_customer_udp;
            8w6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_ipv6 {
        packet.extract<ipv6_t>(hd.customer_ipv6);
        transition select(hd.customer_ipv6.next_header) {
            8w17: parse_customer_udp;
            8w6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_tcp {
        packet.extract<tcp_t>(hd.customer_tcp);
        transition accept;
    }
    state parse_customer_udp {
        packet.extract<udp_t>(hd.customer_udp);
        transition accept;
    }
}

control dash_deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.u0_ethernet);
        packet.emit<ipv4_t>(hdr.u0_ipv4);
        packet.emit<ipv4options_t>(hdr.u0_ipv4options);
        packet.emit<ipv6_t>(hdr.u0_ipv6);
        packet.emit<udp_t>(hdr.u0_udp);
        packet.emit<tcp_t>(hdr.u0_tcp);
        packet.emit<vxlan_t>(hdr.u0_vxlan);
        packet.emit<nvgre_t>(hdr.u0_nvgre);
        packet.emit<ethernet_t>(hdr.customer_ethernet);
        packet.emit<ipv4_t>(hdr.customer_ipv4);
        packet.emit<ipv6_t>(hdr.customer_ipv6);
        packet.emit<tcp_t>(hdr.customer_tcp);
        packet.emit<udp_t>(hdr.customer_udp);
    }
}

match_kind {
    list,
    range_list
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @name("dash_ingress.tmp_7") bit<32> tmp;
    @name("dash_ingress.tmp_8") bit<32> tmp_0;
    @name("dash_ingress.tmp_1") bit<24> tmp_1;
    @name("dash_ingress.tmp_4") bit<32> tmp_4;
    @name("dash_ingress.tmp_5") bit<32> tmp_5;
    @name("dash_ingress.tmp_6") bit<48> tmp_6;
    @name("dash_ingress.tmp_5") bit<32> tmp_39;
    @name("dash_ingress.tmp_6") bit<48> tmp_40;
    @name("dash_ingress.tmp_1") bit<24> tmp_43;
    @name("dash_ingress.tmp_4") bit<32> tmp_46;
    @name("dash_ingress.tmp_5") bit<32> tmp_47;
    @name("dash_ingress.tmp_6") bit<48> tmp_48;
    @name("dash_ingress.tmp") bit<48> tmp_49;
    @name("dash_ingress.tmp") bit<48> tmp_50;
    @name("dash_ingress.tmp") bit<48> tmp_51;
    @name("dash_ingress.tmp_0") bit<48> tmp_52;
    @name("dash_ingress.tmp_0") bit<48> tmp_53;
    @name("dash_ingress.tmp_0") bit<48> tmp_54;
    @name("dash_ingress.eni_lookup_stage.tmp_11") bit<48> eni_lookup_stage_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.outbound.outbound_mapping_stage.hasReturned_4") bool outbound_outbound_mapping_stage_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_nat46.hasReturned_1") bool routing_action_apply_do_action_nat46_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_nat64.hasReturned_2") bool routing_action_apply_do_action_nat64_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_static_encap.hasReturned_0") bool routing_action_apply_do_action_static_encap_hasReturned;
    ethernet_t hdr_0_u0_ethernet;
    ipv4_t hdr_0_u0_ipv4;
    ipv6_t hdr_0_u0_ipv6;
    udp_t hdr_0_u0_udp;
    vxlan_t hdr_0_u0_vxlan;
    nvgre_t hdr_0_u0_nvgre;
    ethernet_t hdr_1_u0_ethernet;
    ipv4_t hdr_1_u0_ipv4;
    ipv6_t hdr_1_u0_ipv6;
    udp_t hdr_1_u0_udp;
    vxlan_t hdr_1_u0_vxlan;
    nvgre_t hdr_1_u0_nvgre;
    ethernet_t hdr_2_u0_ethernet;
    ipv4_t hdr_2_u0_ipv4;
    ipv6_t hdr_2_u0_ipv6;
    udp_t hdr_2_u0_udp;
    vxlan_t hdr_2_u0_vxlan;
    nvgre_t hdr_2_u0_nvgre;
    ethernet_t hdr_8_u0_ethernet;
    ipv4_t hdr_8_u0_ipv4;
    bit<16> meta_42_vnet_id;
    ethernet_t hdr_10_u0_ethernet;
    ipv4_t hdr_10_u0_ipv4;
    ethernet_t hdr_24_u0_ethernet;
    ipv4_t hdr_24_u0_ipv4;
    udp_t hdr_24_u0_udp;
    vxlan_t hdr_24_u0_vxlan;
    ethernet_t hdr_24_customer_ethernet;
    ipv4_t hdr_24_customer_ipv4;
    ipv6_t hdr_24_customer_ipv6;
    ethernet_t hdr_25_u0_ethernet;
    ipv4_t hdr_25_u0_ipv4;
    udp_t hdr_25_u0_udp;
    vxlan_t hdr_25_u0_vxlan;
    ethernet_t hdr_25_customer_ethernet;
    ipv4_t hdr_25_customer_ipv4;
    ipv6_t hdr_25_customer_ipv6;
    ethernet_t hdr_26_u0_ethernet;
    ipv4_t hdr_26_u0_ipv4;
    udp_t hdr_26_u0_udp;
    vxlan_t hdr_26_u0_vxlan;
    ethernet_t hdr_26_customer_ethernet;
    ipv4_t hdr_26_customer_ipv4;
    ipv6_t hdr_26_customer_ipv6;
    ethernet_t hdr_27_u1_ethernet;
    ipv4_t hdr_27_u1_ipv4;
    udp_t hdr_27_u1_udp;
    vxlan_t hdr_27_u1_vxlan;
    ethernet_t hdr_27_u0_ethernet;
    ipv4_t hdr_27_u0_ipv4;
    ipv6_t hdr_27_u0_ipv6;
    ethernet_t hdr_28_u1_ethernet;
    ipv4_t hdr_28_u1_ipv4;
    udp_t hdr_28_u1_udp;
    vxlan_t hdr_28_u1_vxlan;
    ethernet_t hdr_28_u0_ethernet;
    ipv4_t hdr_28_u0_ipv4;
    ipv6_t hdr_28_u0_ipv6;
    ethernet_t hdr_29_u1_ethernet;
    ipv4_t hdr_29_u1_ipv4;
    udp_t hdr_29_u1_udp;
    vxlan_t hdr_29_u1_vxlan;
    ethernet_t hdr_29_u0_ethernet;
    ipv4_t hdr_29_u0_ipv4;
    ipv6_t hdr_29_u0_ipv6;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_8() {
    }
    @name(".tunnel_decap") action tunnel_decap_1() {
        hdr_0_u0_ethernet = hdr.u0_ethernet;
        hdr_0_u0_ipv4 = hdr.u0_ipv4;
        hdr_0_u0_ipv6 = hdr.u0_ipv6;
        hdr_0_u0_udp = hdr.u0_udp;
        hdr_0_u0_vxlan = hdr.u0_vxlan;
        hdr_0_u0_nvgre = hdr.u0_nvgre;
        hdr_0_u0_ethernet.setInvalid();
        hdr_0_u0_ipv4.setInvalid();
        hdr_0_u0_ipv6.setInvalid();
        hdr_0_u0_nvgre.setInvalid();
        hdr_0_u0_vxlan.setInvalid();
        hdr_0_u0_udp.setInvalid();
        hdr.u0_ethernet = hdr_0_u0_ethernet;
        hdr.u0_ipv4 = hdr_0_u0_ipv4;
        hdr.u0_ipv6 = hdr_0_u0_ipv6;
        hdr.u0_udp = hdr_0_u0_udp;
        hdr.u0_vxlan = hdr_0_u0_vxlan;
        hdr.u0_nvgre = hdr_0_u0_nvgre;
        meta._tunnel_pointer39 = 16w0;
    }
    @name(".tunnel_decap") action tunnel_decap_2() {
        hdr_1_u0_ethernet = hdr.u0_ethernet;
        hdr_1_u0_ipv4 = hdr.u0_ipv4;
        hdr_1_u0_ipv6 = hdr.u0_ipv6;
        hdr_1_u0_udp = hdr.u0_udp;
        hdr_1_u0_vxlan = hdr.u0_vxlan;
        hdr_1_u0_nvgre = hdr.u0_nvgre;
        hdr_1_u0_ethernet.setInvalid();
        hdr_1_u0_ipv4.setInvalid();
        hdr_1_u0_ipv6.setInvalid();
        hdr_1_u0_nvgre.setInvalid();
        hdr_1_u0_vxlan.setInvalid();
        hdr_1_u0_udp.setInvalid();
        hdr.u0_ethernet = hdr_1_u0_ethernet;
        hdr.u0_ipv4 = hdr_1_u0_ipv4;
        hdr.u0_ipv6 = hdr_1_u0_ipv6;
        hdr.u0_udp = hdr_1_u0_udp;
        hdr.u0_vxlan = hdr_1_u0_vxlan;
        hdr.u0_nvgre = hdr_1_u0_nvgre;
        meta._tunnel_pointer39 = 16w0;
    }
    @name(".tunnel_decap") action tunnel_decap_3() {
        hdr_2_u0_ethernet = hdr.u0_ethernet;
        hdr_2_u0_ipv4 = hdr.u0_ipv4;
        hdr_2_u0_ipv6 = hdr.u0_ipv6;
        hdr_2_u0_udp = hdr.u0_udp;
        hdr_2_u0_vxlan = hdr.u0_vxlan;
        hdr_2_u0_nvgre = hdr.u0_nvgre;
        hdr_2_u0_ethernet.setInvalid();
        hdr_2_u0_ipv4.setInvalid();
        hdr_2_u0_ipv6.setInvalid();
        hdr_2_u0_nvgre.setInvalid();
        hdr_2_u0_vxlan.setInvalid();
        hdr_2_u0_udp.setInvalid();
        hdr.u0_ethernet = hdr_2_u0_ethernet;
        hdr.u0_ipv4 = hdr_2_u0_ipv4;
        hdr.u0_ipv6 = hdr_2_u0_ipv6;
        hdr.u0_udp = hdr_2_u0_udp;
        hdr.u0_vxlan = hdr_2_u0_vxlan;
        hdr.u0_nvgre = hdr_2_u0_nvgre;
        meta._tunnel_pointer39 = 16w0;
    }
    @name(".route_vnet") action route_vnet_0(@SaiVal[type="sai_object_id_t"] @name("dst_vnet_id") bit<16> dst_vnet_id_2, @name("meter_policy_en") bit<1> meter_policy_en_0, @name("meter_class") bit<16> meter_class_9) {
        meta._dst_vnet_id3 = dst_vnet_id_2;
        meta._meter_policy_en31 = meter_policy_en_0;
        meta._route_meter_class35 = meter_class_9;
        meta._target_stage42 = 16w201;
    }
    @name(".route_vnet_direct") action route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("overlay_ip_is_v6") bit<1> overlay_ip_is_v6, @SaiVal[type="sai_ip_address_t"] @name("overlay_ip") bit<128> overlay_ip, @name("meter_policy_en") bit<1> meter_policy_en_5, @name("meter_class") bit<16> meter_class_10) {
        meta._dst_vnet_id3 = dst_vnet_id_3;
        meta._is_lkup_dst_ip_v617 = overlay_ip_is_v6;
        meta._lkup_dst_ip_addr21 = overlay_ip;
        meta._meter_policy_en31 = meter_policy_en_5;
        meta._route_meter_class35 = meter_class_10;
        meta._target_stage42 = 16w201;
    }
    @name(".route_direct") action route_direct_0(@name("meter_policy_en") bit<1> meter_policy_en_6, @name("meter_class") bit<16> meter_class_11) {
        meta._meter_policy_en31 = meter_policy_en_6;
        meta._route_meter_class35 = meter_class_11;
        meta._target_stage42 = 16w300;
    }
    @name(".route_service_tunnel") action route_service_tunnel_0(@name("overlay_dip_is_v6") bit<1> overlay_dip_is_v6, @name("overlay_dip") bit<128> overlay_dip, @name("overlay_dip_mask_is_v6") bit<1> overlay_dip_mask_is_v6, @name("overlay_dip_mask") bit<128> overlay_dip_mask, @name("overlay_sip_is_v6") bit<1> overlay_sip_is_v6, @name("overlay_sip") bit<128> overlay_sip, @name("overlay_sip_mask_is_v6") bit<1> overlay_sip_mask_is_v6, @name("overlay_sip_mask") bit<128> overlay_sip_mask, @name("underlay_dip_is_v6") bit<1> underlay_dip_is_v6, @name("underlay_dip") bit<128> underlay_dip_6, @name("underlay_sip_is_v6") bit<1> underlay_sip_is_v6, @name("underlay_sip") bit<128> underlay_sip_4, @SaiVal[type="sai_dash_encapsulation_t", default_value="SAI_DASH_ENCAPSULATION_VXLAN"] @name("dash_encapsulation") bit<16> dash_encapsulation_2, @name("tunnel_key") bit<24> tunnel_key, @name("meter_policy_en") bit<1> meter_policy_en_7, @name("meter_class") bit<16> meter_class_12) {
        hdr_8_u0_ethernet = hdr.u0_ethernet;
        hdr_8_u0_ipv4 = hdr.u0_ipv4;
        if (underlay_sip_4 == 128w0) {
            tmp = hdr_8_u0_ipv4.src_addr;
        } else {
            tmp = (bit<32>)underlay_sip_4;
        }
        if (underlay_dip_6 == 128w0) {
            tmp_0 = hdr_8_u0_ipv4.dst_addr;
        } else {
            tmp_0 = (bit<32>)underlay_dip_6;
        }
        if (tunnel_key == 24w0) {
            tmp_1 = meta._encap_data_vni45;
        } else {
            tmp_1 = tunnel_key;
        }
        if (tmp == 32w0) {
            tmp_4 = meta._encap_data_underlay_sip47;
        } else {
            tmp_4 = tmp;
        }
        if (tmp_0 == 32w0) {
            tmp_5 = meta._encap_data_underlay_dip48;
        } else {
            tmp_5 = tmp_0;
        }
        if (hdr_8_u0_ethernet.dst_addr == 48w0) {
            tmp_6 = meta._overlay_data_dmac53;
        } else {
            tmp_6 = hdr_8_u0_ethernet.dst_addr;
        }
        meta._meter_policy_en31 = meter_policy_en_7;
        meta._route_meter_class35 = meter_class_12;
        meta._target_stage42 = 16w300;
        meta._routing_actions43 = meta._routing_actions43 | 32w2 | 32w1;
        meta._encap_data_vni45 = tmp_1;
        meta._encap_data_underlay_sip47 = tmp_4;
        meta._encap_data_underlay_dip48 = tmp_5;
        meta._encap_data_dash_encapsulation51 = dash_encapsulation_2;
        meta._overlay_data_is_ipv652 = true;
        meta._overlay_data_dmac53 = tmp_6;
        meta._overlay_data_sip54 = overlay_sip;
        meta._overlay_data_dip55 = overlay_dip;
        meta._overlay_data_sip_mask56 = overlay_sip_mask;
        meta._overlay_data_dip_mask57 = overlay_dip_mask;
    }
    @name(".drop") action drop_1() {
        meta._target_stage42 = 16w300;
        meta._dropped44 = true;
    }
    @name(".drop") action drop_2() {
        meta._target_stage42 = 16w300;
        meta._dropped44 = true;
    }
    @name(".set_tunnel_mapping") action set_tunnel_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") bit<32> underlay_dip_7, @name("overlay_dmac") bit<48> overlay_dmac, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni, @name("meter_class") bit<16> meter_class_13, @name("meter_class_override") bit<1> meter_class_override) {
        meta_42_vnet_id = meta._vnet_id2;
        if (use_dst_vnet_vni == 1w1) {
            meta_42_vnet_id = meta._dst_vnet_id3;
        }
        if (underlay_dip_7 == 32w0) {
            tmp_39 = meta._encap_data_underlay_dip48;
        } else {
            tmp_39 = underlay_dip_7;
        }
        if (overlay_dmac == 48w0) {
            tmp_40 = meta._overlay_data_dmac53;
        } else {
            tmp_40 = overlay_dmac;
        }
        meta._vnet_id2 = meta_42_vnet_id;
        meta._mapping_meter_class_override32 = meter_class_override;
        meta._mapping_meter_class36 = meter_class_13;
        meta._target_stage42 = 16w300;
        meta._routing_actions43 = meta._routing_actions43 | 32w1;
        meta._encap_data_underlay_dip48 = tmp_39;
        meta._encap_data_dash_encapsulation51 = 16w1;
        meta._overlay_data_dmac53 = tmp_40;
    }
    @name(".set_private_link_mapping") action set_private_link_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") bit<32> underlay_dip_8, @name("overlay_sip") bit<128> overlay_sip_2, @name("overlay_dip") bit<128> overlay_dip_2, @SaiVal[type="sai_dash_encapsulation_t"] @name("dash_encapsulation") bit<16> dash_encapsulation_3, @name("tunnel_key") bit<24> tunnel_key_4, @name("meter_class") bit<16> meter_class_14, @name("meter_class_override") bit<1> meter_class_override_3) {
        hdr_10_u0_ethernet = hdr.u0_ethernet;
        hdr_10_u0_ipv4 = hdr.u0_ipv4;
        if (tunnel_key_4 == 24w0) {
            tmp_43 = meta._encap_data_vni45;
        } else {
            tmp_43 = tunnel_key_4;
        }
        if (meta._eni_data_pl_underlay_sip11 == 32w0) {
            tmp_46 = meta._encap_data_underlay_sip47;
        } else {
            tmp_46 = meta._eni_data_pl_underlay_sip11;
        }
        if (underlay_dip_8 == 32w0) {
            tmp_47 = meta._encap_data_underlay_dip48;
        } else {
            tmp_47 = underlay_dip_8;
        }
        if (hdr_10_u0_ethernet.dst_addr == 48w0) {
            tmp_48 = meta._overlay_data_dmac53;
        } else {
            tmp_48 = hdr_10_u0_ethernet.dst_addr;
        }
        meta._mapping_meter_class_override32 = meter_class_override_3;
        meta._mapping_meter_class36 = meter_class_14;
        meta._target_stage42 = 16w300;
        meta._routing_actions43 = meta._routing_actions43 | 32w1 | 32w2;
        meta._encap_data_vni45 = tmp_43;
        meta._encap_data_underlay_sip47 = tmp_46;
        meta._encap_data_underlay_dip48 = tmp_47;
        meta._encap_data_dash_encapsulation51 = dash_encapsulation_3;
        meta._overlay_data_is_ipv652 = true;
        meta._overlay_data_dmac53 = tmp_48;
        meta._overlay_data_sip54 = overlay_sip_2 & ~meta._eni_data_pl_sip_mask10 | meta._eni_data_pl_sip9 | (bit<128>)hdr_10_u0_ipv4.src_addr;
        meta._overlay_data_dip55 = overlay_dip_2;
        meta._overlay_data_sip_mask56 = 128w0xffffffffffffffffffffffff;
        meta._overlay_data_dip_mask57 = 128w0xffffffffffffffffffffffff;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_1() {
        hdr_24_u0_ethernet = hdr.u0_ethernet;
        hdr_24_u0_ipv4 = hdr.u0_ipv4;
        hdr_24_u0_udp = hdr.u0_udp;
        hdr_24_u0_vxlan = hdr.u0_vxlan;
        hdr_24_customer_ethernet = hdr.customer_ethernet;
        hdr_24_customer_ipv4 = hdr.customer_ipv4;
        hdr_24_customer_ipv6 = hdr.customer_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_49 = hdr_24_customer_ethernet.dst_addr;
        } else {
            tmp_49 = meta._overlay_data_dmac53;
        }
        hdr_24_customer_ethernet.dst_addr = tmp_49;
        hdr_24_u0_ethernet.setValid();
        hdr_24_u0_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_24_u0_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_24_u0_ethernet.ether_type = 16w0x800;
        hdr_24_u0_ipv4.setValid();
        hdr_24_u0_ipv4.total_len = hdr_24_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_24_customer_ipv4.isValid() + hdr_24_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_24_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_24_customer_ipv6.isValid() + 16w50;
        hdr_24_u0_ipv4.version = 4w4;
        hdr_24_u0_ipv4.ihl = 4w5;
        hdr_24_u0_ipv4.diffserv = 8w0;
        hdr_24_u0_ipv4.identification = 16w1;
        hdr_24_u0_ipv4.flags = 3w0;
        hdr_24_u0_ipv4.frag_offset = 13w0;
        hdr_24_u0_ipv4.ttl = 8w64;
        hdr_24_u0_ipv4.protocol = 8w17;
        hdr_24_u0_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_24_u0_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_24_u0_ipv4.hdr_checksum = 16w0;
        hdr_24_u0_udp.setValid();
        hdr_24_u0_udp.src_port = 16w0;
        hdr_24_u0_udp.dst_port = 16w4789;
        hdr_24_u0_udp.length = hdr_24_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_24_customer_ipv4.isValid() + hdr_24_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_24_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_24_customer_ipv6.isValid() + 16w30;
        hdr_24_u0_udp.checksum = 16w0;
        hdr_24_u0_vxlan.setValid();
        hdr_24_u0_vxlan.reserved = 24w0;
        hdr_24_u0_vxlan.reserved_2 = 8w0;
        hdr_24_u0_vxlan.flags = 8w0x8;
        hdr_24_u0_vxlan.vni = meta._encap_data_vni45;
        hdr.u0_ethernet = hdr_24_u0_ethernet;
        hdr.u0_ipv4 = hdr_24_u0_ipv4;
        hdr.u0_udp = hdr_24_u0_udp;
        hdr.u0_vxlan = hdr_24_u0_vxlan;
        hdr.customer_ethernet = hdr_24_customer_ethernet;
        hdr.customer_ipv4 = hdr_24_customer_ipv4;
        hdr.customer_ipv6 = hdr_24_customer_ipv6;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_2() {
        hdr_25_u0_ethernet = hdr.u0_ethernet;
        hdr_25_u0_ipv4 = hdr.u0_ipv4;
        hdr_25_u0_udp = hdr.u0_udp;
        hdr_25_u0_vxlan = hdr.u0_vxlan;
        hdr_25_customer_ethernet = hdr.customer_ethernet;
        hdr_25_customer_ipv4 = hdr.customer_ipv4;
        hdr_25_customer_ipv6 = hdr.customer_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_50 = hdr_25_customer_ethernet.dst_addr;
        } else {
            tmp_50 = meta._overlay_data_dmac53;
        }
        hdr_25_customer_ethernet.dst_addr = tmp_50;
        hdr_25_u0_ethernet.setValid();
        hdr_25_u0_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_25_u0_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_25_u0_ethernet.ether_type = 16w0x800;
        hdr_25_u0_ipv4.setValid();
        hdr_25_u0_ipv4.total_len = hdr_25_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_25_customer_ipv4.isValid() + hdr_25_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_25_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_25_customer_ipv6.isValid() + 16w50;
        hdr_25_u0_ipv4.version = 4w4;
        hdr_25_u0_ipv4.ihl = 4w5;
        hdr_25_u0_ipv4.diffserv = 8w0;
        hdr_25_u0_ipv4.identification = 16w1;
        hdr_25_u0_ipv4.flags = 3w0;
        hdr_25_u0_ipv4.frag_offset = 13w0;
        hdr_25_u0_ipv4.ttl = 8w64;
        hdr_25_u0_ipv4.protocol = 8w17;
        hdr_25_u0_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_25_u0_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_25_u0_ipv4.hdr_checksum = 16w0;
        hdr_25_u0_udp.setValid();
        hdr_25_u0_udp.src_port = 16w0;
        hdr_25_u0_udp.dst_port = 16w4789;
        hdr_25_u0_udp.length = hdr_25_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_25_customer_ipv4.isValid() + hdr_25_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_25_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_25_customer_ipv6.isValid() + 16w30;
        hdr_25_u0_udp.checksum = 16w0;
        hdr_25_u0_vxlan.setValid();
        hdr_25_u0_vxlan.reserved = 24w0;
        hdr_25_u0_vxlan.reserved_2 = 8w0;
        hdr_25_u0_vxlan.flags = 8w0x8;
        hdr_25_u0_vxlan.vni = meta._encap_data_vni45;
        hdr.u0_ethernet = hdr_25_u0_ethernet;
        hdr.u0_ipv4 = hdr_25_u0_ipv4;
        hdr.u0_udp = hdr_25_u0_udp;
        hdr.u0_vxlan = hdr_25_u0_vxlan;
        hdr.customer_ethernet = hdr_25_customer_ethernet;
        hdr.customer_ipv4 = hdr_25_customer_ipv4;
        hdr.customer_ipv6 = hdr_25_customer_ipv6;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_3() {
        hdr_26_u0_ethernet = hdr.u0_ethernet;
        hdr_26_u0_ipv4 = hdr.u0_ipv4;
        hdr_26_u0_udp = hdr.u0_udp;
        hdr_26_u0_vxlan = hdr.u0_vxlan;
        hdr_26_customer_ethernet = hdr.customer_ethernet;
        hdr_26_customer_ipv4 = hdr.customer_ipv4;
        hdr_26_customer_ipv6 = hdr.customer_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_51 = hdr_26_customer_ethernet.dst_addr;
        } else {
            tmp_51 = meta._overlay_data_dmac53;
        }
        hdr_26_customer_ethernet.dst_addr = tmp_51;
        hdr_26_u0_ethernet.setValid();
        hdr_26_u0_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_26_u0_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_26_u0_ethernet.ether_type = 16w0x800;
        hdr_26_u0_ipv4.setValid();
        hdr_26_u0_ipv4.total_len = hdr_26_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_26_customer_ipv4.isValid() + hdr_26_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_26_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_26_customer_ipv6.isValid() + 16w50;
        hdr_26_u0_ipv4.version = 4w4;
        hdr_26_u0_ipv4.ihl = 4w5;
        hdr_26_u0_ipv4.diffserv = 8w0;
        hdr_26_u0_ipv4.identification = 16w1;
        hdr_26_u0_ipv4.flags = 3w0;
        hdr_26_u0_ipv4.frag_offset = 13w0;
        hdr_26_u0_ipv4.ttl = 8w64;
        hdr_26_u0_ipv4.protocol = 8w17;
        hdr_26_u0_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_26_u0_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_26_u0_ipv4.hdr_checksum = 16w0;
        hdr_26_u0_udp.setValid();
        hdr_26_u0_udp.src_port = 16w0;
        hdr_26_u0_udp.dst_port = 16w4789;
        hdr_26_u0_udp.length = hdr_26_customer_ipv4.total_len * (bit<16>)(bit<1>)hdr_26_customer_ipv4.isValid() + hdr_26_customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr_26_customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_26_customer_ipv6.isValid() + 16w30;
        hdr_26_u0_udp.checksum = 16w0;
        hdr_26_u0_vxlan.setValid();
        hdr_26_u0_vxlan.reserved = 24w0;
        hdr_26_u0_vxlan.reserved_2 = 8w0;
        hdr_26_u0_vxlan.flags = 8w0x8;
        hdr_26_u0_vxlan.vni = meta._encap_data_vni45;
        hdr.u0_ethernet = hdr_26_u0_ethernet;
        hdr.u0_ipv4 = hdr_26_u0_ipv4;
        hdr.u0_udp = hdr_26_u0_udp;
        hdr.u0_vxlan = hdr_26_u0_vxlan;
        hdr.customer_ethernet = hdr_26_customer_ethernet;
        hdr.customer_ipv4 = hdr_26_customer_ipv4;
        hdr.customer_ipv6 = hdr_26_customer_ipv6;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_1() {
        hdr_27_u1_ethernet = hdr.u1_ethernet;
        hdr_27_u1_ipv4 = hdr.u1_ipv4;
        hdr_27_u1_udp = hdr.u1_udp;
        hdr_27_u1_vxlan = hdr.u1_vxlan;
        hdr_27_u0_ethernet = hdr.u0_ethernet;
        hdr_27_u0_ipv4 = hdr.u0_ipv4;
        hdr_27_u0_ipv6 = hdr.u0_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_52 = hdr_27_u0_ethernet.dst_addr;
        } else {
            tmp_52 = meta._overlay_data_dmac53;
        }
        hdr_27_u0_ethernet.dst_addr = tmp_52;
        hdr_27_u1_ethernet.setValid();
        hdr_27_u1_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_27_u1_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_27_u1_ethernet.ether_type = 16w0x800;
        hdr_27_u1_ipv4.setValid();
        hdr_27_u1_ipv4.total_len = hdr_27_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_27_u0_ipv4.isValid() + hdr_27_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_27_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_27_u0_ipv6.isValid() + 16w50;
        hdr_27_u1_ipv4.version = 4w4;
        hdr_27_u1_ipv4.ihl = 4w5;
        hdr_27_u1_ipv4.diffserv = 8w0;
        hdr_27_u1_ipv4.identification = 16w1;
        hdr_27_u1_ipv4.flags = 3w0;
        hdr_27_u1_ipv4.frag_offset = 13w0;
        hdr_27_u1_ipv4.ttl = 8w64;
        hdr_27_u1_ipv4.protocol = 8w17;
        hdr_27_u1_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_27_u1_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_27_u1_ipv4.hdr_checksum = 16w0;
        hdr_27_u1_udp.setValid();
        hdr_27_u1_udp.src_port = 16w0;
        hdr_27_u1_udp.dst_port = 16w4789;
        hdr_27_u1_udp.length = hdr_27_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_27_u0_ipv4.isValid() + hdr_27_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_27_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_27_u0_ipv6.isValid() + 16w30;
        hdr_27_u1_udp.checksum = 16w0;
        hdr_27_u1_vxlan.setValid();
        hdr_27_u1_vxlan.reserved = 24w0;
        hdr_27_u1_vxlan.reserved_2 = 8w0;
        hdr_27_u1_vxlan.flags = 8w0x8;
        hdr_27_u1_vxlan.vni = meta._encap_data_vni45;
        hdr.u1_ethernet = hdr_27_u1_ethernet;
        hdr.u1_ipv4 = hdr_27_u1_ipv4;
        hdr.u1_udp = hdr_27_u1_udp;
        hdr.u1_vxlan = hdr_27_u1_vxlan;
        hdr.u0_ethernet = hdr_27_u0_ethernet;
        hdr.u0_ipv4 = hdr_27_u0_ipv4;
        hdr.u0_ipv6 = hdr_27_u0_ipv6;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_2() {
        hdr_28_u1_ethernet = hdr.u1_ethernet;
        hdr_28_u1_ipv4 = hdr.u1_ipv4;
        hdr_28_u1_udp = hdr.u1_udp;
        hdr_28_u1_vxlan = hdr.u1_vxlan;
        hdr_28_u0_ethernet = hdr.u0_ethernet;
        hdr_28_u0_ipv4 = hdr.u0_ipv4;
        hdr_28_u0_ipv6 = hdr.u0_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_53 = hdr_28_u0_ethernet.dst_addr;
        } else {
            tmp_53 = meta._overlay_data_dmac53;
        }
        hdr_28_u0_ethernet.dst_addr = tmp_53;
        hdr_28_u1_ethernet.setValid();
        hdr_28_u1_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_28_u1_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_28_u1_ethernet.ether_type = 16w0x800;
        hdr_28_u1_ipv4.setValid();
        hdr_28_u1_ipv4.total_len = hdr_28_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_28_u0_ipv4.isValid() + hdr_28_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_28_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_28_u0_ipv6.isValid() + 16w50;
        hdr_28_u1_ipv4.version = 4w4;
        hdr_28_u1_ipv4.ihl = 4w5;
        hdr_28_u1_ipv4.diffserv = 8w0;
        hdr_28_u1_ipv4.identification = 16w1;
        hdr_28_u1_ipv4.flags = 3w0;
        hdr_28_u1_ipv4.frag_offset = 13w0;
        hdr_28_u1_ipv4.ttl = 8w64;
        hdr_28_u1_ipv4.protocol = 8w17;
        hdr_28_u1_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_28_u1_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_28_u1_ipv4.hdr_checksum = 16w0;
        hdr_28_u1_udp.setValid();
        hdr_28_u1_udp.src_port = 16w0;
        hdr_28_u1_udp.dst_port = 16w4789;
        hdr_28_u1_udp.length = hdr_28_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_28_u0_ipv4.isValid() + hdr_28_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_28_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_28_u0_ipv6.isValid() + 16w30;
        hdr_28_u1_udp.checksum = 16w0;
        hdr_28_u1_vxlan.setValid();
        hdr_28_u1_vxlan.reserved = 24w0;
        hdr_28_u1_vxlan.reserved_2 = 8w0;
        hdr_28_u1_vxlan.flags = 8w0x8;
        hdr_28_u1_vxlan.vni = meta._encap_data_vni45;
        hdr.u1_ethernet = hdr_28_u1_ethernet;
        hdr.u1_ipv4 = hdr_28_u1_ipv4;
        hdr.u1_udp = hdr_28_u1_udp;
        hdr.u1_vxlan = hdr_28_u1_vxlan;
        hdr.u0_ethernet = hdr_28_u0_ethernet;
        hdr.u0_ipv4 = hdr_28_u0_ipv4;
        hdr.u0_ipv6 = hdr_28_u0_ipv6;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_3() {
        hdr_29_u1_ethernet = hdr.u1_ethernet;
        hdr_29_u1_ipv4 = hdr.u1_ipv4;
        hdr_29_u1_udp = hdr.u1_udp;
        hdr_29_u1_vxlan = hdr.u1_vxlan;
        hdr_29_u0_ethernet = hdr.u0_ethernet;
        hdr_29_u0_ipv4 = hdr.u0_ipv4;
        hdr_29_u0_ipv6 = hdr.u0_ipv6;
        if (meta._overlay_data_dmac53 == 48w0) {
            tmp_54 = hdr_29_u0_ethernet.dst_addr;
        } else {
            tmp_54 = meta._overlay_data_dmac53;
        }
        hdr_29_u0_ethernet.dst_addr = tmp_54;
        hdr_29_u1_ethernet.setValid();
        hdr_29_u1_ethernet.dst_addr = meta._encap_data_underlay_dmac50;
        hdr_29_u1_ethernet.src_addr = meta._encap_data_underlay_smac49;
        hdr_29_u1_ethernet.ether_type = 16w0x800;
        hdr_29_u1_ipv4.setValid();
        hdr_29_u1_ipv4.total_len = hdr_29_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_29_u0_ipv4.isValid() + hdr_29_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_29_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_29_u0_ipv6.isValid() + 16w50;
        hdr_29_u1_ipv4.version = 4w4;
        hdr_29_u1_ipv4.ihl = 4w5;
        hdr_29_u1_ipv4.diffserv = 8w0;
        hdr_29_u1_ipv4.identification = 16w1;
        hdr_29_u1_ipv4.flags = 3w0;
        hdr_29_u1_ipv4.frag_offset = 13w0;
        hdr_29_u1_ipv4.ttl = 8w64;
        hdr_29_u1_ipv4.protocol = 8w17;
        hdr_29_u1_ipv4.dst_addr = meta._encap_data_underlay_dip48;
        hdr_29_u1_ipv4.src_addr = meta._encap_data_underlay_sip47;
        hdr_29_u1_ipv4.hdr_checksum = 16w0;
        hdr_29_u1_udp.setValid();
        hdr_29_u1_udp.src_port = 16w0;
        hdr_29_u1_udp.dst_port = 16w4789;
        hdr_29_u1_udp.length = hdr_29_u0_ipv4.total_len * (bit<16>)(bit<1>)hdr_29_u0_ipv4.isValid() + hdr_29_u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr_29_u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_29_u0_ipv6.isValid() + 16w30;
        hdr_29_u1_udp.checksum = 16w0;
        hdr_29_u1_vxlan.setValid();
        hdr_29_u1_vxlan.reserved = 24w0;
        hdr_29_u1_vxlan.reserved_2 = 8w0;
        hdr_29_u1_vxlan.flags = 8w0x8;
        hdr_29_u1_vxlan.vni = meta._encap_data_vni45;
        hdr.u1_ethernet = hdr_29_u1_ethernet;
        hdr.u1_ipv4 = hdr_29_u1_ipv4;
        hdr.u1_udp = hdr_29_u1_udp;
        hdr.u1_vxlan = hdr_29_u1_vxlan;
        hdr.u0_ethernet = hdr_29_u0_ethernet;
        hdr.u0_ipv4 = hdr_29_u0_ipv4;
        hdr.u0_ipv6 = hdr_29_u0_ipv6;
    }
    @name("dash_ingress.drop_action") action drop_action() {
        mark_to_drop(standard_metadata);
    }
    @name("dash_ingress.deny") action deny() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.deny") action deny_0() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.deny") action deny_1() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.deny") action deny_3() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.deny") action deny_4() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.accept") action accept_1() {
    }
    @SaiCounter[name="lb_fast_path_icmp_in", attr_type="stats"] @name("dash_ingress.port_lb_fast_path_icmp_in_counter") counter(32w1, CounterType.packets_and_bytes) port_lb_fast_path_icmp_in_counter_0;
    @SaiTable[name="vip", api="dash_vip"] @name("dash_ingress.vip") table vip_0 {
        key = {
            hdr.u0_ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            accept_1();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @name("dash_ingress.set_appliance") action set_appliance(@name("neighbor_mac") bit<48> neighbor_mac, @name("mac") bit<48> mac) {
        meta._encap_data_underlay_dmac50 = neighbor_mac;
        meta._encap_data_underlay_smac49 = mac;
    }
    @SaiTable[ignored="true"] @name("dash_ingress.appliance") table appliance_0 {
        key = {
            meta._appliance_id15: ternary @name("meta.appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @SaiCounter[name="lb_fast_path_icmp_in", attr_type="stats", action_names="set_eni_attrs"] @name("dash_ingress.eni_lb_fast_path_icmp_in_counter") counter(32w64, CounterType.packets_and_bytes) eni_lb_fast_path_icmp_in_counter_0;
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @SaiVal[type="sai_ip_address_t"] @name("vm_underlay_dip") bit<32> vm_underlay_dip, @SaiVal[type="sai_uint32_t"] @name("vm_vni") bit<24> vm_vni, @SaiVal[type="sai_object_id_t"] @name("vnet_id") bit<16> vnet_id_1, @name("pl_sip") bit<128> pl_sip_1, @name("pl_sip_mask") bit<128> pl_sip_mask_1, @SaiVal[type="sai_ip_address_t"] @name("pl_underlay_sip") bit<32> pl_underlay_sip_1, @SaiVal[type="sai_object_id_t"] @name("v4_meter_policy_id") bit<16> v4_meter_policy_id, @SaiVal[type="sai_object_id_t"] @name("v6_meter_policy_id") bit<16> v6_meter_policy_id, @SaiVal[type="sai_dash_tunnel_dscp_mode_t"] @name("dash_tunnel_dscp_mode") bit<16> dash_tunnel_dscp_mode, @SaiVal[type="sai_uint8_t", validonly="SAI_ENI_ATTR_DASH_TUNNEL_DSCP_MODE == SAI_DASH_TUNNEL_DSCP_MODE_PIPE_MODEL"] @name("dscp") bit<6> dscp_1, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id, @name("disable_fast_path_icmp_flow_redirection") bit<1> disable_fast_path_icmp_flow_redirection) {
        meta._eni_data_cps5 = cps_1;
        meta._eni_data_pps6 = pps_1;
        meta._eni_data_flows7 = flows_1;
        meta._eni_data_admin_state8 = admin_state_1;
        meta._eni_data_pl_sip9 = pl_sip_1;
        meta._eni_data_pl_sip_mask10 = pl_sip_mask_1;
        meta._eni_data_pl_underlay_sip11 = pl_underlay_sip_1;
        meta._encap_data_underlay_dip48 = vm_underlay_dip;
        if (dash_tunnel_dscp_mode == 16w1) {
            meta._eni_data_dscp12 = dscp_1;
        }
        meta._encap_data_vni45 = vm_vni;
        meta._vnet_id2 = vnet_id_1;
        if (meta._is_overlay_ip_v616 == 1w1) {
            if (meta._direction0 == 16w1) {
                meta._stage1_dash_acl_group_id26 = outbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id27 = outbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id28 = outbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id29 = outbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id30 = outbound_v6_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id26 = inbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id27 = inbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id28 = inbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id29 = inbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id30 = inbound_v6_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id33 = v6_meter_policy_id;
        } else {
            if (meta._direction0 == 16w1) {
                meta._stage1_dash_acl_group_id26 = outbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id27 = outbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id28 = outbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id29 = outbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id30 = outbound_v4_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id26 = inbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id27 = inbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id28 = inbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id29 = inbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id30 = inbound_v4_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id33 = v4_meter_policy_id;
        }
        meta._fast_path_icmp_flow_redirection_disabled41 = disable_fast_path_icmp_flow_redirection;
    }
    @SaiTable[name="eni", api="dash_eni", order=1, isobject="true"] @name("dash_ingress.eni") table eni_0 {
        key = {
            meta._eni_id4: exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny_0();
        }
        const default_action = deny_0();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.tunnel_decap_pa_validate") action tunnel_decap_pa_validate(@SaiVal[type="sai_object_id_t"] @name("src_vnet_id") bit<16> src_vnet_id) {
        meta._vnet_id2 = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] @name("dash_ingress.pa_validation") table pa_validation_0 {
        key = {
            meta._vnet_id2      : exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
            hdr.u0_ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            permit();
            @defaultonly deny_1();
        }
        const default_action = deny_1();
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] @name("dash_ingress.inbound_routing") table inbound_routing_0 {
        key = {
            meta._eni_id4       : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            hdr.u0_vxlan.vni    : exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
            hdr.u0_ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            tunnel_decap_1();
            tunnel_decap_pa_validate();
            @defaultonly deny_3();
        }
        const default_action = deny_3();
    }
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta._is_overlay_ip_v616 == 1w1) {
                meta._dropped44 = true;
            }
        } else if (meta._is_overlay_ip_v616 == 1w0) {
            meta._dropped44 = true;
        }
    }
    @SaiTable[name="dash_acl_group", api="dash_acl", order=0, isobject="true"] @name("dash_ingress.acl_group") table acl_group_0 {
        key = {
            meta._stage1_dash_acl_group_id26: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("dash_ingress.direction_lookup_stage.set_outbound_direction") action direction_lookup_stage_set_outbound_direction_0() {
        meta._direction0 = 16w1;
    }
    @name("dash_ingress.direction_lookup_stage.set_inbound_direction") action direction_lookup_stage_set_inbound_direction_0() {
        meta._direction0 = 16w2;
    }
    @SaiTable[name="direction_lookup", api="dash_direction_lookup"] @name("dash_ingress.direction_lookup_stage.direction_lookup") table direction_lookup_stage_direction_lookup {
        key = {
            hdr.u0_vxlan.vni: exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
        }
        actions = {
            direction_lookup_stage_set_outbound_direction_0();
            @defaultonly direction_lookup_stage_set_inbound_direction_0();
        }
        const default_action = direction_lookup_stage_set_inbound_direction_0();
    }
    @SaiCounter[name="lb_fast_path_eni_miss", attr_type="stats"] @name("dash_ingress.eni_lookup_stage.port_lb_fast_path_eni_miss_counter") counter(32w1, CounterType.packets_and_bytes) eni_lookup_stage_port_lb_fast_path_eni_miss_counter;
    @name("dash_ingress.eni_lookup_stage.set_eni") action eni_lookup_stage_set_eni_0(@SaiVal[type="sai_object_id_t"] @name("eni_id") bit<16> eni_id_1) {
        meta._eni_id4 = eni_id_1;
    }
    @name("dash_ingress.eni_lookup_stage.deny") action eni_lookup_stage_deny_0() {
        meta._dropped44 = true;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", order=0] @name("dash_ingress.eni_lookup_stage.eni_ether_address_map") table eni_lookup_stage_eni_ether_address_map {
        key = {
            eni_lookup_stage_tmp: exact @SaiVal[name="address", type="sai_mac_t"] @name("meta.eni_addr");
        }
        actions = {
            eni_lookup_stage_set_eni_0();
            @defaultonly eni_lookup_stage_deny_0();
        }
        const default_action = eni_lookup_stage_deny_0();
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_0() {
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_1() {
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_2() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_0() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_1() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_2() {
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_0() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_1() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_2() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_0() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_1() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_2() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.outbound.acl.stage1_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage1_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage1") table outbound_acl_stage1 {
        key = {
            meta._stage1_dash_acl_group_id26: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage1_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_0();
            outbound_acl_permit_and_continue_0();
            outbound_acl_deny_0();
            outbound_acl_deny_and_continue_0();
        }
        default_action = outbound_acl_deny_0();
        counters = outbound_acl_stage1_counter;
    }
    @name("dash_ingress.outbound.acl.stage2_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage2_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage2") table outbound_acl_stage2 {
        key = {
            meta._stage2_dash_acl_group_id27: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage2_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_1();
            outbound_acl_permit_and_continue_1();
            outbound_acl_deny_1();
            outbound_acl_deny_and_continue_1();
        }
        default_action = outbound_acl_deny_1();
        counters = outbound_acl_stage2_counter;
    }
    @name("dash_ingress.outbound.acl.stage3_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage3_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage3") table outbound_acl_stage3 {
        key = {
            meta._stage3_dash_acl_group_id28: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage3_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_2();
            outbound_acl_permit_and_continue_2();
            outbound_acl_deny_2();
            outbound_acl_deny_and_continue_2();
        }
        default_action = outbound_acl_deny_2();
        counters = outbound_acl_stage3_counter;
    }
    @name("dash_ingress.outbound.outbound_routing_stage.routing_counter") direct_counter(CounterType.packets_and_bytes) outbound_outbound_routing_stage_routing_counter;
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] @name("dash_ingress.outbound.outbound_routing_stage.routing") table outbound_outbound_routing_stage_routing {
        key = {
            meta._eni_id4           : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._is_overlay_ip_v616: exact @SaiVal[name="destination_is_v6"] @name("meta.is_overlay_ip_v6");
            meta._dst_ip_addr19     : lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            route_vnet_0();
            route_vnet_direct_0();
            route_direct_0();
            route_service_tunnel_0();
            drop_1();
        }
        const default_action = drop_1();
        counters = outbound_outbound_routing_stage_routing_counter;
    }
    @name("dash_ingress.outbound.outbound_mapping_stage.ca_to_pa_counter") direct_counter(CounterType.packets_and_bytes) outbound_outbound_mapping_stage_ca_to_pa_counter;
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] @name("dash_ingress.outbound.outbound_mapping_stage.ca_to_pa") table outbound_outbound_mapping_stage_ca_to_pa {
        key = {
            meta._dst_vnet_id3       : exact @SaiVal[type="sai_object_id_t"] @name("meta.dst_vnet_id");
            meta._is_lkup_dst_ip_v617: exact @SaiVal[name="dip_is_v6"] @name("meta.is_lkup_dst_ip_v6");
            meta._lkup_dst_ip_addr21 : exact @SaiVal[name="dip"] @name("meta.lkup_dst_ip_addr");
        }
        actions = {
            set_tunnel_mapping_0();
            set_private_link_mapping_0();
            @defaultonly drop_2();
        }
        const default_action = drop_2();
        counters = outbound_outbound_mapping_stage_ca_to_pa_counter;
    }
    @name("dash_ingress.outbound.outbound_mapping_stage.set_vnet_attrs") action outbound_outbound_mapping_stage_set_vnet_attrs_0(@name("vni") bit<24> vni_2) {
        meta._encap_data_vni45 = vni_2;
    }
    @SaiTable[name="vnet", api="dash_vnet", isobject="true"] @name("dash_ingress.outbound.outbound_mapping_stage.vnet") table outbound_outbound_mapping_stage_vnet {
        key = {
            meta._vnet_id2: exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
        }
        actions = {
            outbound_outbound_mapping_stage_set_vnet_attrs_0();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_0() {
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_1() {
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_2() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_0() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_1() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_2() {
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_0() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_1() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_2() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_0() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_1() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_2() {
        meta._dropped44 = true;
    }
    @name("dash_ingress.inbound.acl.stage1_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage1_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage1") table inbound_acl_stage1 {
        key = {
            meta._stage1_dash_acl_group_id26: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage1_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_0();
            inbound_acl_permit_and_continue_0();
            inbound_acl_deny_0();
            inbound_acl_deny_and_continue_0();
        }
        default_action = inbound_acl_deny_0();
        counters = inbound_acl_stage1_counter;
    }
    @name("dash_ingress.inbound.acl.stage2_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage2_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage2") table inbound_acl_stage2 {
        key = {
            meta._stage2_dash_acl_group_id27: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage2_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_1();
            inbound_acl_permit_and_continue_1();
            inbound_acl_deny_1();
            inbound_acl_deny_and_continue_1();
        }
        default_action = inbound_acl_deny_1();
        counters = inbound_acl_stage2_counter;
    }
    @name("dash_ingress.inbound.acl.stage3_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage3_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage3") table inbound_acl_stage3 {
        key = {
            meta._stage3_dash_acl_group_id28: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage3_dash_acl_group_id");
            meta._dst_ip_addr19             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr20             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol18             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port24             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port25             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_2();
            inbound_acl_permit_and_continue_2();
            inbound_acl_deny_2();
            inbound_acl_deny_and_continue_2();
        }
        default_action = inbound_acl_deny_2();
        counters = inbound_acl_stage3_counter;
    }
    @name("dash_ingress.underlay.pkt_act") action underlay_pkt_act_0(@name("packet_action") bit<9> packet_action, @name("next_hop_id") bit<9> next_hop_id) {
        if (packet_action == 9w0) {
            meta._dropped44 = true;
        } else if (packet_action == 9w1) {
            standard_metadata.egress_spec = next_hop_id;
        }
    }
    @name("dash_ingress.underlay.def_act") action underlay_def_act_0() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @SaiTable[name="route", api="route", api_type="underlay"] @name("dash_ingress.underlay.underlay_routing") table underlay_underlay_routing {
        key = {
            meta._dst_ip_addr19: lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            underlay_pkt_act_0();
            @defaultonly underlay_def_act_0();
            @defaultonly NoAction_4();
        }
        default_action = NoAction_4();
    }
    @name("dash_ingress.metering_update_stage.check_ip_addr_family") action metering_update_stage_check_ip_addr_family_0(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family_2) {
        if (ip_addr_family_2 == 32w0) {
            if (meta._is_overlay_ip_v616 == 1w1) {
                meta._dropped44 = true;
            }
        } else if (meta._is_overlay_ip_v616 == 1w0) {
            meta._dropped44 = true;
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", order=1, isobject="true"] @name("dash_ingress.metering_update_stage.meter_policy") table metering_update_stage_meter_policy {
        key = {
            meta._meter_policy_id33: exact @name("meta.meter_policy_id");
        }
        actions = {
            metering_update_stage_check_ip_addr_family_0();
            @defaultonly NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name("dash_ingress.metering_update_stage.set_policy_meter_class") action metering_update_stage_set_policy_meter_class_0(@name("meter_class") bit<16> meter_class_15) {
        meta._policy_meter_class34 = meter_class_15;
    }
    @SaiTable[name="meter_rule", api="dash_meter", order=2, isobject="true"] @name("dash_ingress.metering_update_stage.meter_rule") table metering_update_stage_meter_rule {
        key = {
            meta._meter_policy_id33: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"] @name("meta.meter_policy_id");
            hdr.u0_ipv4.dst_addr   : ternary @SaiVal[name="dip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            metering_update_stage_set_policy_meter_class_0();
            @defaultonly NoAction_6();
        }
        const default_action = NoAction_6();
    }
    @SaiCounter[name="outbound", action_names="meter_bucket_action", attr_type="counter_attr"] @name("dash_ingress.metering_update_stage.meter_bucket_outbound") counter(32w262144, CounterType.bytes) metering_update_stage_meter_bucket_outbound;
    @SaiCounter[name="inbound", action_names="meter_bucket_action", attr_type="counter_attr"] @name("dash_ingress.metering_update_stage.meter_bucket_inbound") counter(32w262144, CounterType.bytes) metering_update_stage_meter_bucket_inbound;
    @name("dash_ingress.metering_update_stage.meter_bucket_action") action metering_update_stage_meter_bucket_action_0(@SaiVal[type="sai_uint32_t", skipattr="true"] @name("meter_bucket_index") bit<32> meter_bucket_index_1) {
        meta._meter_bucket_index38 = meter_bucket_index_1;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", order=0, isobject="true"] @name("dash_ingress.metering_update_stage.meter_bucket") table metering_update_stage_meter_bucket {
        key = {
            meta._eni_id4      : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._meter_class37: exact @name("meta.meter_class");
        }
        actions = {
            metering_update_stage_meter_bucket_action_0();
            @defaultonly NoAction_7();
        }
        const default_action = NoAction_7();
    }
    @name("dash_ingress.metering_update_stage.eni_counter") direct_counter(CounterType.packets_and_bytes) metering_update_stage_eni_counter;
    @SaiTable[ignored="true"] @name("dash_ingress.metering_update_stage.eni_meter") table metering_update_stage_eni_meter {
        key = {
            meta._eni_id4   : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._direction0: exact @name("meta.direction");
            meta._dropped44 : exact @name("meta.dropped");
        }
        actions = {
            NoAction_8();
        }
        counters = metering_update_stage_eni_counter;
        default_action = NoAction_8();
    }
    @hidden action dashpipelinev1modelbmv2l1153() {
        port_lb_fast_path_icmp_in_counter_0.count(32w0);
    }
    @hidden action dashpipelinev1modelbmv2l1156() {
        meta._encap_data_underlay_sip47 = hdr.u0_ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l877() {
        eni_lookup_stage_tmp = hdr.customer_ethernet.src_addr;
    }
    @hidden action dashpipelinev1modelbmv2l877_0() {
        eni_lookup_stage_tmp = hdr.customer_ethernet.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l880() {
        eni_lookup_stage_port_lb_fast_path_eni_miss_counter.count(32w0);
    }
    @hidden action dashpipelinev1modelbmv2l877_1() {
        meta._eni_addr1 = eni_lookup_stage_tmp;
    }
    @hidden action dashpipelinev1modelbmv2l1164() {
        meta._eni_data_dscp_mode13 = 16w0;
        meta._eni_data_dscp12 = (bit<6>)hdr.u0_ipv4.diffserv;
    }
    @hidden action dashpipelinev1modelbmv2l1181() {
        meta._ip_protocol18 = hdr.customer_ipv6.next_header;
        meta._src_ip_addr20 = hdr.customer_ipv6.src_addr;
        meta._dst_ip_addr19 = hdr.customer_ipv6.dst_addr;
        meta._is_overlay_ip_v616 = 1w1;
    }
    @hidden action dashpipelinev1modelbmv2l1186() {
        meta._ip_protocol18 = hdr.customer_ipv4.protocol;
        meta._src_ip_addr20 = (bit<128>)hdr.customer_ipv4.src_addr;
        meta._dst_ip_addr19 = (bit<128>)hdr.customer_ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l1176() {
        meta._is_overlay_ip_v616 = 1w0;
        meta._ip_protocol18 = 8w0;
        meta._dst_ip_addr19 = 128w0;
        meta._src_ip_addr20 = 128w0;
    }
    @hidden action dashpipelinev1modelbmv2l1191() {
        meta._src_l4_port24 = hdr.customer_tcp.src_port;
        meta._dst_l4_port25 = hdr.customer_tcp.dst_port;
    }
    @hidden action dashpipelinev1modelbmv2l1194() {
        meta._src_l4_port24 = hdr.customer_udp.src_port;
        meta._dst_l4_port25 = hdr.customer_udp.dst_port;
    }
    @hidden action dashpipelinev1modelbmv2l1202() {
        eni_lb_fast_path_icmp_in_counter_0.count((bit<32>)meta._eni_id4);
    }
    @hidden action dashpipelinev1modelbmv2l536() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l539() {
        outbound_acl_hasReturned = true;
    }
    @hidden action act() {
        outbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l546() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l549() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l1206() {
        meta._target_stage42 = 16w200;
    }
    @hidden action dashpipelinev1modelbmv2l774() {
        meta._lkup_dst_ip_addr21 = meta._dst_ip_addr19;
        meta._is_lkup_dst_ip_v617 = meta._is_overlay_ip_v616;
    }
    @hidden action dashpipelinev1modelbmv2l759() {
        outbound_outbound_mapping_stage_hasReturned = true;
    }
    @hidden action act_0() {
        outbound_outbound_mapping_stage_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l536_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l539_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_1() {
        inbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l546_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l549_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l830() {
        meta._tunnel_pointer39 = meta._tunnel_pointer39 + 16w1;
    }
    @hidden action dashpipelinev1modelbmv2l609() {
        routing_action_apply_do_action_nat46_hasReturned = true;
    }
    @hidden action act_2() {
        routing_action_apply_do_action_nat46_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l611() {
        assert(meta._overlay_data_is_ipv652);
        hdr.u0_ipv6.setValid();
        hdr.u0_ipv6.version = 4w6;
        hdr.u0_ipv6.traffic_class = 8w0;
        hdr.u0_ipv6.flow_label = 20w0;
        hdr.u0_ipv6.payload_length = hdr.u0_ipv4.total_len + 16w65516;
        hdr.u0_ipv6.next_header = hdr.u0_ipv4.protocol;
        hdr.u0_ipv6.hop_limit = hdr.u0_ipv4.ttl;
        hdr.u0_ipv6.dst_addr = (bit<128>)hdr.u0_ipv4.dst_addr & ~meta._overlay_data_dip_mask57 | meta._overlay_data_dip55 & meta._overlay_data_dip_mask57;
        hdr.u0_ipv6.src_addr = (bit<128>)hdr.u0_ipv4.src_addr & ~meta._overlay_data_sip_mask56 | meta._overlay_data_sip54 & meta._overlay_data_sip_mask56;
        hdr.u0_ipv4.setInvalid();
        hdr.u0_ethernet.ether_type = 16w0x86dd;
    }
    @hidden action dashpipelinev1modelbmv2l635() {
        routing_action_apply_do_action_nat64_hasReturned = true;
    }
    @hidden action act_3() {
        routing_action_apply_do_action_nat64_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l637() {
        assert(!meta._overlay_data_is_ipv652);
        hdr.u0_ipv4.setValid();
        hdr.u0_ipv4.version = 4w4;
        hdr.u0_ipv4.ihl = 4w5;
        hdr.u0_ipv4.diffserv = 8w0;
        hdr.u0_ipv4.total_len = hdr.u0_ipv6.payload_length + 16w20;
        hdr.u0_ipv4.identification = 16w1;
        hdr.u0_ipv4.flags = 3w0;
        hdr.u0_ipv4.frag_offset = 13w0;
        hdr.u0_ipv4.protocol = hdr.u0_ipv6.next_header;
        hdr.u0_ipv4.ttl = hdr.u0_ipv6.hop_limit;
        hdr.u0_ipv4.hdr_checksum = 16w0;
        hdr.u0_ipv4.dst_addr = (bit<32>)meta._overlay_data_dip55;
        hdr.u0_ipv4.src_addr = (bit<32>)meta._overlay_data_sip54;
        hdr.u0_ipv6.setInvalid();
        hdr.u0_ethernet.ether_type = 16w0x800;
    }
    @hidden action dashpipelinev1modelbmv2l579() {
        routing_action_apply_do_action_static_encap_hasReturned = true;
    }
    @hidden action act_4() {
        routing_action_apply_do_action_static_encap_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l594() {
        meta._tunnel_pointer39 = meta._tunnel_pointer39 + 16w1;
    }
    @hidden action dashpipelinev1modelbmv2l1212() {
        meta._dst_ip_addr19 = (bit<128>)hdr.u0_ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l963() {
        meta._meter_class37 = meta._policy_meter_class34;
    }
    @hidden action dashpipelinev1modelbmv2l965() {
        meta._meter_class37 = meta._route_meter_class35;
    }
    @hidden action dashpipelinev1modelbmv2l968() {
        meta._meter_class37 = meta._mapping_meter_class36;
    }
    @hidden action dashpipelinev1modelbmv2l973() {
        metering_update_stage_meter_bucket_outbound.count(meta._meter_bucket_index38);
    }
    @hidden action dashpipelinev1modelbmv2l975() {
        metering_update_stage_meter_bucket_inbound.count(meta._meter_bucket_index38);
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1153 {
        actions = {
            dashpipelinev1modelbmv2l1153();
        }
        const default_action = dashpipelinev1modelbmv2l1153();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1156 {
        actions = {
            dashpipelinev1modelbmv2l1156();
        }
        const default_action = dashpipelinev1modelbmv2l1156();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l877 {
        actions = {
            dashpipelinev1modelbmv2l877();
        }
        const default_action = dashpipelinev1modelbmv2l877();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l877_0 {
        actions = {
            dashpipelinev1modelbmv2l877_0();
        }
        const default_action = dashpipelinev1modelbmv2l877_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l877_1 {
        actions = {
            dashpipelinev1modelbmv2l877_1();
        }
        const default_action = dashpipelinev1modelbmv2l877_1();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l880 {
        actions = {
            dashpipelinev1modelbmv2l880();
        }
        const default_action = dashpipelinev1modelbmv2l880();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1164 {
        actions = {
            dashpipelinev1modelbmv2l1164();
        }
        const default_action = dashpipelinev1modelbmv2l1164();
    }
    @hidden table tbl_tunnel_decap {
        actions = {
            tunnel_decap_2();
        }
        const default_action = tunnel_decap_2();
    }
    @hidden table tbl_tunnel_decap_0 {
        actions = {
            tunnel_decap_3();
        }
        const default_action = tunnel_decap_3();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1176 {
        actions = {
            dashpipelinev1modelbmv2l1176();
        }
        const default_action = dashpipelinev1modelbmv2l1176();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1181 {
        actions = {
            dashpipelinev1modelbmv2l1181();
        }
        const default_action = dashpipelinev1modelbmv2l1181();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1186 {
        actions = {
            dashpipelinev1modelbmv2l1186();
        }
        const default_action = dashpipelinev1modelbmv2l1186();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1191 {
        actions = {
            dashpipelinev1modelbmv2l1191();
        }
        const default_action = dashpipelinev1modelbmv2l1191();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1194 {
        actions = {
            dashpipelinev1modelbmv2l1194();
        }
        const default_action = dashpipelinev1modelbmv2l1194();
    }
    @hidden table tbl_deny {
        actions = {
            deny_4();
        }
        const default_action = deny_4();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1202 {
        actions = {
            dashpipelinev1modelbmv2l1202();
        }
        const default_action = dashpipelinev1modelbmv2l1202();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1206 {
        actions = {
            dashpipelinev1modelbmv2l1206();
        }
        const default_action = dashpipelinev1modelbmv2l1206();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l536 {
        actions = {
            dashpipelinev1modelbmv2l536();
        }
        const default_action = dashpipelinev1modelbmv2l536();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l539 {
        actions = {
            dashpipelinev1modelbmv2l539();
        }
        const default_action = dashpipelinev1modelbmv2l539();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l546 {
        actions = {
            dashpipelinev1modelbmv2l546();
        }
        const default_action = dashpipelinev1modelbmv2l546();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l549 {
        actions = {
            dashpipelinev1modelbmv2l549();
        }
        const default_action = dashpipelinev1modelbmv2l549();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l774 {
        actions = {
            dashpipelinev1modelbmv2l774();
        }
        const default_action = dashpipelinev1modelbmv2l774();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l759 {
        actions = {
            dashpipelinev1modelbmv2l759();
        }
        const default_action = dashpipelinev1modelbmv2l759();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l536_0 {
        actions = {
            dashpipelinev1modelbmv2l536_0();
        }
        const default_action = dashpipelinev1modelbmv2l536_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l539_0 {
        actions = {
            dashpipelinev1modelbmv2l539_0();
        }
        const default_action = dashpipelinev1modelbmv2l539_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l546_0 {
        actions = {
            dashpipelinev1modelbmv2l546_0();
        }
        const default_action = dashpipelinev1modelbmv2l546_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l549_0 {
        actions = {
            dashpipelinev1modelbmv2l549_0();
        }
        const default_action = dashpipelinev1modelbmv2l549_0();
    }
    @hidden table tbl_push_vxlan_tunnel_u0 {
        actions = {
            push_vxlan_tunnel_u0_1();
        }
        const default_action = push_vxlan_tunnel_u0_1();
    }
    @hidden table tbl_push_vxlan_tunnel_u1 {
        actions = {
            push_vxlan_tunnel_u1_1();
        }
        const default_action = push_vxlan_tunnel_u1_1();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l830 {
        actions = {
            dashpipelinev1modelbmv2l830();
        }
        const default_action = dashpipelinev1modelbmv2l830();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l609 {
        actions = {
            dashpipelinev1modelbmv2l609();
        }
        const default_action = dashpipelinev1modelbmv2l609();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l611 {
        actions = {
            dashpipelinev1modelbmv2l611();
        }
        const default_action = dashpipelinev1modelbmv2l611();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l635 {
        actions = {
            dashpipelinev1modelbmv2l635();
        }
        const default_action = dashpipelinev1modelbmv2l635();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l637 {
        actions = {
            dashpipelinev1modelbmv2l637();
        }
        const default_action = dashpipelinev1modelbmv2l637();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l579 {
        actions = {
            dashpipelinev1modelbmv2l579();
        }
        const default_action = dashpipelinev1modelbmv2l579();
    }
    @hidden table tbl_push_vxlan_tunnel_u0_0 {
        actions = {
            push_vxlan_tunnel_u0_2();
        }
        const default_action = push_vxlan_tunnel_u0_2();
    }
    @hidden table tbl_push_vxlan_tunnel_u1_0 {
        actions = {
            push_vxlan_tunnel_u1_2();
        }
        const default_action = push_vxlan_tunnel_u1_2();
    }
    @hidden table tbl_push_vxlan_tunnel_u0_1 {
        actions = {
            push_vxlan_tunnel_u0_3();
        }
        const default_action = push_vxlan_tunnel_u0_3();
    }
    @hidden table tbl_push_vxlan_tunnel_u1_1 {
        actions = {
            push_vxlan_tunnel_u1_3();
        }
        const default_action = push_vxlan_tunnel_u1_3();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l594 {
        actions = {
            dashpipelinev1modelbmv2l594();
        }
        const default_action = dashpipelinev1modelbmv2l594();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l1212 {
        actions = {
            dashpipelinev1modelbmv2l1212();
        }
        const default_action = dashpipelinev1modelbmv2l1212();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l963 {
        actions = {
            dashpipelinev1modelbmv2l963();
        }
        const default_action = dashpipelinev1modelbmv2l963();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l965 {
        actions = {
            dashpipelinev1modelbmv2l965();
        }
        const default_action = dashpipelinev1modelbmv2l965();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l968 {
        actions = {
            dashpipelinev1modelbmv2l968();
        }
        const default_action = dashpipelinev1modelbmv2l968();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l973 {
        actions = {
            dashpipelinev1modelbmv2l973();
        }
        const default_action = dashpipelinev1modelbmv2l973();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l975 {
        actions = {
            dashpipelinev1modelbmv2l975();
        }
        const default_action = dashpipelinev1modelbmv2l975();
    }
    @hidden table tbl_drop_action {
        actions = {
            drop_action();
        }
        const default_action = drop_action();
    }
    apply {
        if (meta._is_fast_path_icmp_flow_redirection_packet40) {
            tbl_dashpipelinev1modelbmv2l1153.apply();
        }
        if (vip_0.apply().hit) {
            tbl_dashpipelinev1modelbmv2l1156.apply();
        } else {
            ;
        }
        direction_lookup_stage_direction_lookup.apply();
        appliance_0.apply();
        if (meta._direction0 == 16w1) {
            tbl_dashpipelinev1modelbmv2l877.apply();
        } else {
            tbl_dashpipelinev1modelbmv2l877_0.apply();
        }
        tbl_dashpipelinev1modelbmv2l877_1.apply();
        if (eni_lookup_stage_eni_ether_address_map.apply().hit) {
            ;
        } else if (meta._is_fast_path_icmp_flow_redirection_packet40) {
            tbl_dashpipelinev1modelbmv2l880.apply();
        }
        tbl_dashpipelinev1modelbmv2l1164.apply();
        if (meta._direction0 == 16w1) {
            tbl_tunnel_decap.apply();
        } else if (meta._direction0 == 16w2) {
            switch (inbound_routing_0.apply().action_run) {
                tunnel_decap_pa_validate: {
                    pa_validation_0.apply();
                    tbl_tunnel_decap_0.apply();
                }
                default: {
                }
            }
        }
        tbl_dashpipelinev1modelbmv2l1176.apply();
        if (hdr.customer_ipv6.isValid()) {
            tbl_dashpipelinev1modelbmv2l1181.apply();
        } else if (hdr.customer_ipv4.isValid()) {
            tbl_dashpipelinev1modelbmv2l1186.apply();
        }
        if (hdr.customer_tcp.isValid()) {
            tbl_dashpipelinev1modelbmv2l1191.apply();
        } else if (hdr.customer_udp.isValid()) {
            tbl_dashpipelinev1modelbmv2l1194.apply();
        }
        eni_0.apply();
        if (meta._eni_data_admin_state8 == 1w0) {
            tbl_deny.apply();
        }
        if (meta._is_fast_path_icmp_flow_redirection_packet40) {
            tbl_dashpipelinev1modelbmv2l1202.apply();
        }
        acl_group_0.apply();
        if (meta._direction0 == 16w1) {
            tbl_dashpipelinev1modelbmv2l1206.apply();
            if (meta._conntrack_data_allow_out23) {
                ;
            } else {
                tbl_act.apply();
                if (meta._stage1_dash_acl_group_id26 != 16w0) {
                    switch (outbound_acl_stage1.apply().action_run) {
                        outbound_acl_permit_0: {
                            tbl_dashpipelinev1modelbmv2l536.apply();
                        }
                        outbound_acl_deny_0: {
                            tbl_dashpipelinev1modelbmv2l539.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id27 != 16w0) {
                    switch (outbound_acl_stage2.apply().action_run) {
                        outbound_acl_permit_1: {
                            tbl_dashpipelinev1modelbmv2l546.apply();
                        }
                        outbound_acl_deny_1: {
                            tbl_dashpipelinev1modelbmv2l549.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id28 != 16w0) {
                    switch (outbound_acl_stage3.apply().action_run) {
                        outbound_acl_permit_2: {
                        }
                        outbound_acl_deny_2: {
                        }
                        default: {
                        }
                    }
                }
            }
            tbl_dashpipelinev1modelbmv2l774.apply();
            outbound_outbound_routing_stage_routing.apply();
            tbl_act_0.apply();
            if (meta._target_stage42 != 16w201) {
                tbl_dashpipelinev1modelbmv2l759.apply();
            }
            if (outbound_outbound_mapping_stage_hasReturned) {
                ;
            } else {
                switch (outbound_outbound_mapping_stage_ca_to_pa.apply().action_run) {
                    set_tunnel_mapping_0: {
                        outbound_outbound_mapping_stage_vnet.apply();
                    }
                    default: {
                    }
                }
            }
        } else if (meta._direction0 == 16w2) {
            if (meta._conntrack_data_allow_in22) {
                ;
            } else {
                tbl_act_1.apply();
                if (meta._stage1_dash_acl_group_id26 != 16w0) {
                    switch (inbound_acl_stage1.apply().action_run) {
                        inbound_acl_permit_0: {
                            tbl_dashpipelinev1modelbmv2l536_0.apply();
                        }
                        inbound_acl_deny_0: {
                            tbl_dashpipelinev1modelbmv2l539_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id27 != 16w0) {
                    switch (inbound_acl_stage2.apply().action_run) {
                        inbound_acl_permit_1: {
                            tbl_dashpipelinev1modelbmv2l546_0.apply();
                        }
                        inbound_acl_deny_1: {
                            tbl_dashpipelinev1modelbmv2l549_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id28 != 16w0) {
                    switch (inbound_acl_stage3.apply().action_run) {
                        inbound_acl_permit_2: {
                        }
                        inbound_acl_deny_2: {
                        }
                        default: {
                        }
                    }
                }
            }
            if (meta._tunnel_pointer39 == 16w0) {
                tbl_push_vxlan_tunnel_u0.apply();
            } else if (meta._tunnel_pointer39 == 16w1) {
                tbl_push_vxlan_tunnel_u1.apply();
            }
            tbl_dashpipelinev1modelbmv2l830.apply();
        }
        tbl_act_2.apply();
        if (meta._routing_actions43 & 32w2 == 32w0) {
            tbl_dashpipelinev1modelbmv2l609.apply();
        }
        if (routing_action_apply_do_action_nat46_hasReturned) {
            ;
        } else {
            tbl_dashpipelinev1modelbmv2l611.apply();
        }
        tbl_act_3.apply();
        if (meta._routing_actions43 & 32w4 == 32w0) {
            tbl_dashpipelinev1modelbmv2l635.apply();
        }
        if (routing_action_apply_do_action_nat64_hasReturned) {
            ;
        } else {
            tbl_dashpipelinev1modelbmv2l637.apply();
        }
        tbl_act_4.apply();
        if (meta._routing_actions43 & 32w1 == 32w0) {
            tbl_dashpipelinev1modelbmv2l579.apply();
        }
        if (routing_action_apply_do_action_static_encap_hasReturned) {
            ;
        } else {
            if (meta._encap_data_dash_encapsulation51 == 16w1) {
                if (meta._tunnel_pointer39 == 16w0) {
                    tbl_push_vxlan_tunnel_u0_0.apply();
                } else if (meta._tunnel_pointer39 == 16w1) {
                    tbl_push_vxlan_tunnel_u1_0.apply();
                }
            } else if (meta._encap_data_dash_encapsulation51 == 16w2) {
                if (meta._tunnel_pointer39 == 16w0) {
                    tbl_push_vxlan_tunnel_u0_1.apply();
                } else if (meta._tunnel_pointer39 == 16w1) {
                    tbl_push_vxlan_tunnel_u1_1.apply();
                }
            }
            tbl_dashpipelinev1modelbmv2l594.apply();
        }
        tbl_dashpipelinev1modelbmv2l1212.apply();
        underlay_underlay_routing.apply();
        if (meta._meter_policy_en31 == 1w1) {
            metering_update_stage_meter_policy.apply();
            metering_update_stage_meter_rule.apply();
        }
        if (meta._meter_policy_en31 == 1w1) {
            tbl_dashpipelinev1modelbmv2l963.apply();
        } else {
            tbl_dashpipelinev1modelbmv2l965.apply();
        }
        if (meta._meter_class37 == 16w0 || meta._mapping_meter_class_override32 == 1w1) {
            tbl_dashpipelinev1modelbmv2l968.apply();
        }
        metering_update_stage_meter_bucket.apply();
        if (meta._direction0 == 16w1) {
            tbl_dashpipelinev1modelbmv2l973.apply();
        } else if (meta._direction0 == 16w2) {
            tbl_dashpipelinev1modelbmv2l975.apply();
        }
        metering_update_stage_eni_meter.apply();
        if (meta._dropped44) {
            tbl_drop_action.apply();
        }
    }
}

control dash_verify_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<13> f5;
    bit<3>  f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control dash_compute_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.u0_ipv4.isValid(), (tuple_0){f0 = hdr.u0_ipv4.version,f1 = hdr.u0_ipv4.ihl,f2 = hdr.u0_ipv4.diffserv,f3 = hdr.u0_ipv4.total_len,f4 = hdr.u0_ipv4.identification,f5 = hdr.u0_ipv4.frag_offset,f6 = hdr.u0_ipv4.flags,f7 = hdr.u0_ipv4.ttl,f8 = hdr.u0_ipv4.protocol,f9 = hdr.u0_ipv4.src_addr,f10 = hdr.u0_ipv4.dst_addr}, hdr.u0_ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

control dash_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(dash_parser(), dash_verify_checksum(), dash_ingress(), dash_egress(), dash_compute_checksum(), dash_deparser()) main;
