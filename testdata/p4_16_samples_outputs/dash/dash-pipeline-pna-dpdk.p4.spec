
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> total_len
	bit<16> identification
	bit<16> flags_frag_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdr_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct ipv4options_t {
	varbit<320> options
}

struct ipv6_t {
	;oldname:version_traffic_class_flow_label
	bit<32> version_traffic_class_flow_la0
	bit<16> payload_length
	bit<8> next_header
	bit<8> hop_limit
	bit<128> src_addr
	bit<128> dst_addr
}

struct udp_t {
	bit<16> src_port
	bit<16> dst_port
	bit<16> length
	bit<16> checksum
}

struct tcp_t {
	bit<16> src_port
	bit<16> dst_port
	bit<32> seq_no
	bit<32> ack_no
	bit<16> data_offset_res_ecn_flags
	bit<16> window
	bit<16> checksum
	bit<16> urgent_ptr
}

struct vxlan_t {
	bit<8> flags
	bit<24> reserved
	bit<24> vni
	bit<8> reserved_2
}

struct nvgre_t {
	bit<16> flags_reserved_version
	bit<16> protocol_type
	bit<24> vsid
	bit<8> flow_id
}

struct dpdk_pseudo_header_t {
	bit<32> pseudo
	bit<32> pseudo_0
	bit<64> pseudo_1
	bit<64> pseudo_2
}

struct _p4c_tmp128_t {
	bit<64> inter
}

struct _p4c_sandbox_header_t {
	bit<64> upper_half
	bit<64> lower_half
}

struct eni_lookup_stage_set_eni_0_arg_t {
	bit<16> eni_id
}

struct metering_update_stage_check_ip_addr_family_0_arg_t {
	bit<32> ip_addr_family
}

struct metering_update_stage_meter_bucket_action_0_arg_t {
	bit<32> meter_bucket_index
}

struct metering_update_stage_set_policy_meter_class_0_arg_t {
	bit<16> meter_class
}

struct outbound_outbound_mapping_stage_set_vnet_attrs_0_arg_t {
	bit<24> vni
}

struct route_direct_0_arg_t {
	bit<8> meter_policy_en
	bit<16> meter_class
}

struct route_service_tunnel_0_arg_t {
	bit<8> overlay_dip_is_v6
	bit<128> overlay_dip
	bit<8> overlay_dip_mask_is_v6
	bit<128> overlay_dip_mask
	bit<8> overlay_sip_is_v6
	bit<128> overlay_sip
	bit<8> overlay_sip_mask_is_v6
	bit<128> overlay_sip_mask
	bit<8> underlay_dip_is_v6
	bit<128> underlay_dip
	bit<8> underlay_sip_is_v6
	bit<128> underlay_sip
	bit<16> dash_encapsulation
	bit<24> tunnel_key
	bit<8> meter_policy_en
	bit<16> meter_class
}

struct route_vnet_0_arg_t {
	bit<16> dst_vnet_id
	bit<8> meter_policy_en
	bit<16> meter_class
}

struct route_vnet_direct_0_arg_t {
	bit<16> dst_vnet_id
	bit<8> overlay_ip_is_v6
	bit<128> overlay_ip
	bit<8> meter_policy_en
	bit<16> meter_class
}

struct set_acl_group_attrs_arg_t {
	bit<32> ip_addr_family
}

struct set_appliance_arg_t {
	bit<48> neighbor_mac
	bit<48> mac
}

struct set_eni_attrs_arg_t {
	bit<32> cps
	bit<32> pps
	bit<32> flows
	bit<8> admin_state
	bit<32> vm_underlay_dip
	bit<24> vm_vni
	bit<16> vnet_id
	bit<128> pl_sip
	bit<128> pl_sip_mask
	bit<32> pl_underlay_sip
	bit<16> v4_meter_policy_id
	bit<16> v6_meter_policy_id
	bit<16> dash_tunnel_dscp_mode
	bit<8> dscp
	bit<16> inbound_v4_stage1_dash_acl_group_id
	bit<16> inbound_v4_stage2_dash_acl_group_id
	bit<16> inbound_v4_stage3_dash_acl_group_id
	bit<16> inbound_v4_stage4_dash_acl_group_id
	bit<16> inbound_v4_stage5_dash_acl_group_id
	bit<16> inbound_v6_stage1_dash_acl_group_id
	bit<16> inbound_v6_stage2_dash_acl_group_id
	bit<16> inbound_v6_stage3_dash_acl_group_id
	bit<16> inbound_v6_stage4_dash_acl_group_id
	bit<16> inbound_v6_stage5_dash_acl_group_id
	bit<16> outbound_v4_stage1_dash_acl_group_id
	bit<16> outbound_v4_stage2_dash_acl_group_id
	bit<16> outbound_v4_stage3_dash_acl_group_id
	bit<16> outbound_v4_stage4_dash_acl_group_id
	bit<16> outbound_v4_stage5_dash_acl_group_id
	bit<16> outbound_v6_stage1_dash_acl_group_id
	bit<16> outbound_v6_stage2_dash_acl_group_id
	bit<16> outbound_v6_stage3_dash_acl_group_id
	bit<16> outbound_v6_stage4_dash_acl_group_id
	bit<16> outbound_v6_stage5_dash_acl_group_id
	bit<8> disable_fast_path_icmp_flow_redirection
}

struct set_private_link_mapping_0_arg_t {
	bit<32> underlay_dip
	bit<128> overlay_sip
	bit<128> overlay_dip
	bit<16> dash_encapsulation
	bit<24> tunnel_key
	bit<16> meter_class
	bit<8> meter_class_override
}

struct set_tunnel_mapping_0_arg_t {
	bit<32> underlay_dip
	bit<48> overlay_dmac
	bit<8> use_dst_vnet_vni
	bit<16> meter_class
	bit<8> meter_class_override
}

struct tunnel_decap_pa_validate_arg_t {
	bit<16> src_vnet_id
}

struct underlay_pkt_act_0_arg_t {
	bit<16> packet_action
	bit<16> next_hop_id
}

header u1_ethernet instanceof ethernet_t
header u1_ipv4 instanceof ipv4_t
header u1_ipv4options instanceof ipv4options_t
header u1_ipv6 instanceof ipv6_t
header u1_udp instanceof udp_t
header u1_tcp instanceof tcp_t
header u1_vxlan instanceof vxlan_t
header u1_nvgre instanceof nvgre_t
header u0_ethernet instanceof ethernet_t
header u0_ipv4 instanceof ipv4_t
header u0_ipv4options instanceof ipv4options_t
header u0_ipv6 instanceof ipv6_t
header u0_udp instanceof udp_t
header u0_tcp instanceof tcp_t
header u0_vxlan instanceof vxlan_t
header u0_nvgre instanceof nvgre_t
header customer_ethernet instanceof ethernet_t
header customer_ipv4 instanceof ipv4_t
header customer_ipv6 instanceof ipv6_t
header customer_udp instanceof udp_t
header customer_tcp instanceof tcp_t
header dpdk_pseudo_header instanceof dpdk_pseudo_header_t

struct metadata_t {
	bit<16> pna_pre_input_metadata_parser_error
	bit<32> pna_main_input_metadata_input_port
	bit<16> local_metadata___direction00
	bit<48> local_metadata___eni_addr11
	bit<16> local_metadata___vnet_id22
	bit<16> local_metadata___dst_vnet_id33
	bit<16> local_metadata___eni_id44
	bit<32> local_metadata___eni_data_cps55
	bit<32> local_metadata___eni_data_pps66
	bit<32> local_metadata___eni_data_flows77
	bit<8> local_metadata___eni_data_admin_state88
	bit<128> local_metadata___eni_data_pl_sip99
	bit<128> local_metadata___eni_data_pl_sip_mask1010
	bit<32> local_metadata___eni_data_pl_underlay_sip1111
	bit<8> local_metadata___eni_data_dscp1212
	bit<16> local_metadata___eni_data_dscp_mode1313
	bit<8> local_metadata___appliance_id1515
	bit<8> local_metadata___is_overlay_ip_v61616
	bit<8> local_metadata___is_lkup_dst_ip_v61717
	bit<8> local_metadata___ip_protocol1818
	bit<128> local_metadata___dst_ip_addr1919
	bit<128> local_metadata___src_ip_addr2020
	bit<128> local_metadata___lkup_dst_ip_addr2121
	bit<8> local_metadata___conntrack_data_allow_in2222
	bit<8> local_metadata___conntrack_data_allow_out2323
	bit<16> local_metadata___src_l4_port2424
	bit<16> local_metadata___dst_l4_port2525
	bit<16> local_metadata___stage1_dash_acl_group_id2626
	bit<16> local_metadata___stage2_dash_acl_group_id2727
	bit<16> local_metadata___stage3_dash_acl_group_id2828
	bit<16> local_metadata___stage4_dash_acl_group_id2929
	bit<16> local_metadata___stage5_dash_acl_group_id3030
	bit<8> local_metadata___meter_policy_en3131
	bit<8> local_metadata___mapping_meter_class_override3232
	bit<16> local_metadata___meter_policy_id3333
	bit<16> local_metadata___policy_meter_class3434
	bit<16> local_metadata___route_meter_class3535
	bit<16> local_metadata___mapping_meter_class3636
	bit<16> local_metadata___meter_class3737
	bit<32> local_metadata___meter_bucket_index3838
	bit<16> local_metadata___tunnel_pointer3939
	;oldname:local_metadata___fast_path_icmp_flow_redirection_disabled4141
	bit<8> local_metadata___fast_path_icmp_flow_redirection_disabled411
	bit<16> local_metadata___target_stage4242
	bit<32> local_metadata___routing_actions4343
	bit<8> local_metadata___dropped4444
	bit<24> local_metadata___encap_data_vni4545
	bit<32> local_metadata___encap_data_underlay_sip4747
	bit<32> local_metadata___encap_data_underlay_dip4848
	bit<48> local_metadata___encap_data_underlay_smac4949
	bit<48> local_metadata___encap_data_underlay_dmac5050
	bit<16> local_metadata___encap_data_dash_encapsulation5151
	bit<8> local_metadata___overlay_data_is_ipv65252
	bit<48> local_metadata___overlay_data_dmac5353
	bit<128> local_metadata___overlay_data_sip5454
	bit<128> local_metadata___overlay_data_dip5555
	bit<128> local_metadata___overlay_data_sip_mask5656
	bit<128> local_metadata___overlay_data_dip_mask5757
	bit<8> local_metadata__meta_43_overlay_data_is_ipv658
	bit<48> local_metadata__meta_43_overlay_data_dmac59
	bit<128> local_metadata__meta_43_overlay_data_sip60
	bit<128> local_metadata__meta_43_overlay_data_dip61
	bit<128> local_metadata__meta_43_overlay_data_sip_mask62
	bit<128> local_metadata__meta_43_overlay_data_dip_mask63
	bit<32> pna_main_output_metadata_output_port
	bit<16> dash_ingress_pa_validation_local_metadata___vnet_id22
	bit<32> dash_ingress_pa_validation_u0_ipv4_src_addr
	bit<24> dash_ingress_inbound_routing_u0_vxlan_vni
	bit<32> dash_ingress_inbound_routing_u0_ipv4_src_addr
	;oldname:dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local_metadata___dst_vnet_id33
	bit<16> dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local2
	;oldname:dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local_metadata___is_lkup_dst_ip_v61717
	bit<8> dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local3
	;oldname:dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local_metadata___lkup_dst_ip_addr2121
	bit<128> dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local4
	;oldname:dash_ingress_metering_update_stage_meter_rule_u0_ipv4_dst_addr
	bit<32> dash_ingress_metering_update_stage_meter_rule_u0_ipv4_dst_a5
	;oldname:dash_ingress_metering_update_stage_meter_bucket_local_metadata___eni_id44
	bit<16> dash_ingress_metering_update_stage_meter_bucket_local_metad6
	;oldname:dash_ingress_metering_update_stage_meter_bucket_local_metadata___meter_class3737
	bit<16> dash_ingress_metering_update_stage_meter_bucket_local_metad7
	;oldname:dash_ingress_metering_update_stage_eni_meter_local_metadata___eni_id44
	bit<16> dash_ingress_metering_update_stage_eni_meter_local_metadata8
	;oldname:dash_ingress_metering_update_stage_eni_meter_local_metadata___direction00
	bit<16> dash_ingress_metering_update_stage_eni_meter_local_metadata9
	;oldname:dash_ingress_metering_update_stage_eni_meter_local_metadata___dropped4444
	bit<8> dash_ingress_metering_update_stage_eni_meter_local_metadat10
	bit<8> MainParserT_parser_tmp
	bit<8> MainParserT_parser_tmp_0
	bit<8> MainParserT_parser_tmp_1
	bit<8> MainParserT_parser_tmp_3
	bit<8> MainParserT_parser_tmp_4
	bit<8> MainParserT_parser_tmp_6
	bit<8> MainParserT_parser_tmp_7
	bit<8> MainParserT_parser_tmp_8
	bit<8> MainParserT_parser_tmp_10
	bit<8> MainParserT_parser_tmp_11
	bit<8> MainParserT_parser_tmp_13
	bit<8> MainParserT_parser_tmp_14
	bit<8> MainParserT_parser_tmp_15
	bit<8> MainParserT_parser_tmp_16
	bit<16> MainParserT_parser_tmp_19
	bit<16> MainParserT_parser_tmp_20
	bit<8> MainParserT_parser_tmp_21
	bit<8> MainParserT_parser_tmp_22
	bit<8> MainParserT_parser_tmp_23
	bit<8> MainParserT_parser_tmp_24
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> MainControlT_tmp_1
	bit<128> MainControlT_tmp_2
	bit<128> MainControlT_tmp_3
	bit<128> MainControlT_tmp_4
	bit<128> MainControlT_tmp_5
	bit<128> MainControlT_tmp_6
	bit<128> MainControlT_tmp_7
	bit<32> MainControlT_tmp_8
	bit<128> MainControlT_tmp_9
	bit<128> MainControlT_tmp_10
	bit<128> MainControlT_tmp_11
	bit<128> MainControlT_tmp_12
	bit<128> MainControlT_tmp_13
	bit<128> MainControlT_tmp_14
	bit<128> MainControlT_tmp_15
	bit<128> MainControlT_tmp_16
	bit<32> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<8> MainControlT_tmp_19
	bit<8> MainControlT_tmp_20
	bit<16> MainControlT_tmp_21
	bit<16> MainControlT_tmp_22
	bit<16> MainControlT_tmp_23
	bit<8> MainControlT_tmp_24
	bit<8> MainControlT_tmp_25
	bit<16> MainControlT_tmp_26
	bit<16> MainControlT_tmp_27
	bit<16> MainControlT_tmp_28
	bit<8> MainControlT_tmp_29
	bit<8> MainControlT_tmp_30
	bit<16> MainControlT_tmp_31
	bit<16> MainControlT_tmp_32
	bit<16> MainControlT_tmp_33
	bit<8> MainControlT_tmp_34
	bit<8> MainControlT_tmp_35
	bit<16> MainControlT_tmp_36
	bit<16> MainControlT_tmp_37
	bit<16> MainControlT_tmp_38
	bit<8> MainControlT_tmp_39
	bit<8> MainControlT_tmp_40
	bit<16> MainControlT_tmp_41
	bit<16> MainControlT_tmp_42
	bit<16> MainControlT_tmp_43
	bit<8> MainControlT_tmp_44
	bit<8> MainControlT_tmp_45
	bit<16> MainControlT_tmp_46
	bit<16> MainControlT_tmp_47
	bit<8> MainControlT_tmp_48
	bit<8> MainControlT_tmp_49
	bit<8> MainControlT_tmp_50
	bit<32> MainControlT_tmp_51
	bit<32> MainControlT_tmp_52
	bit<32> MainControlT_tmp_53
	bit<128> MainControlT_tmp_55
	bit<128> MainControlT_tmp_56
	bit<128> MainControlT_tmp_57
	bit<128> MainControlT_tmp_59
	bit<128> MainControlT_tmp_60
	bit<128> MainControlT_tmp_61
	bit<8> MainControlT_tmp_62
	bit<8> MainControlT_tmp_63
	bit<16> MainControlT_tmp_64
	bit<16> MainControlT_tmp_65
	bit<128> MainControlT_tmp_66
	bit<128> MainControlT_tmp_67
	bit<128> MainControlT_tmp_68
	bit<128> MainControlT_tmp_69
	bit<128> MainControlT_tmp_70
	bit<128> MainControlT_tmp_71
	bit<32> MainControlT_tmp_72
	bit<32> MainControlT_tmp_73
	bit<24> MainControlT_tmp_74
	bit<32> MainControlT_tmp_77
	bit<32> MainControlT_tmp_78
	bit<48> MainControlT_tmp_79
	bit<24> MainControlT_tmp_80
	bit<32> MainControlT_tmp_83
	bit<32> MainControlT_tmp_84
	bit<48> MainControlT_tmp_85
	bit<24> MainControlT_tmp_86
	bit<32> MainControlT_tmp_89
	bit<32> MainControlT_tmp_90
	bit<48> MainControlT_tmp_91
	bit<16> MainControlT_customer_ip_len
	bit<16> MainControlT_customer_ip_len_0
	bit<16> MainControlT_customer_ip_len_1
	bit<16> MainControlT_u0_ip_len
	bit<16> MainControlT_u0_ip_len_0
	bit<16> MainControlT_u0_ip_len_1
	bit<48> MainControlT_eni_lookup_stage_tmp
	bit<8> MainControlT_outbound_acl_hasReturned
	bit<8> MainControlT_inbound_acl_hasReturned
	bit<16> MainControlT_meta_42_vnet_id
	bit<32> MainParserT_parser_tmp_26_extract_tmp
}
metadata instanceof metadata_t

header local_metadata___overlay_data_dip_mask5757_128 instanceof _p4c_sandbox_header_t
header local_metadata___overlay_data_dip_mask5757_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_56_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_56_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_55_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_57_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_57_tmp instanceof _p4c_tmp128_t
header dst_addr_128 instanceof _p4c_sandbox_header_t
header dst_addr_tmp instanceof _p4c_tmp128_t
header local_metadata___overlay_data_sip_mask5656_128 instanceof _p4c_sandbox_header_t
header local_metadata___overlay_data_sip_mask5656_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_60_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_60_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_59_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_61_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_61_tmp instanceof _p4c_tmp128_t
header src_addr_128 instanceof _p4c_sandbox_header_t
header src_addr_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_66_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_66_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_67_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_67_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_68_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_68_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_69_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_69_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_70_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_70_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_71_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_71_tmp instanceof _p4c_tmp128_t
header underlay_sip_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_2_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_2_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_3_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_3_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_4_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_4_tmp instanceof _p4c_tmp128_t
header underlay_dip_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_5_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_5_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_6_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_6_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_7_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_7_tmp instanceof _p4c_tmp128_t
header local_metadata___eni_data_pl_sip_mask1010_128 instanceof _p4c_sandbox_header_t
header local_metadata___eni_data_pl_sip_mask1010_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_10_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_10_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_9_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_11_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_11_tmp instanceof _p4c_tmp128_t
header local_metadata___eni_data_pl_sip99_128 instanceof _p4c_sandbox_header_t
header local_metadata__meta_43_overlay_data_sip60_128 instanceof _p4c_sandbox_header_t
header local_metadata__meta_43_overlay_data_sip60_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_12_128 instanceof _p4c_sandbox_header_t
header local_metadata__meta_43_overlay_data_sip_mask62_128 instanceof _p4c_sandbox_header_t
header local_metadata__meta_43_overlay_data_dip_mask63_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_14_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_14_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_13_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_15_128 instanceof _p4c_sandbox_header_t
header MainControlT_tmp_15_tmp instanceof _p4c_tmp128_t
header MainControlT_tmp_16_128 instanceof _p4c_sandbox_header_t
regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action tunnel_decap_1 args none {
	mov m.local_metadata___tunnel_pointer3939 0x0
	return
}

action route_vnet_0 args instanceof route_vnet_0_arg_t {
	mov m.local_metadata___dst_vnet_id33 t.dst_vnet_id
	mov m.local_metadata___meter_policy_en3131 t.meter_policy_en
	mov m.local_metadata___route_meter_class3535 t.meter_class
	mov m.local_metadata___target_stage4242 0xC9
	return
}

action route_vnet_direct_0 args instanceof route_vnet_direct_0_arg_t {
	mov m.local_metadata___dst_vnet_id33 t.dst_vnet_id
	mov m.local_metadata___is_lkup_dst_ip_v61717 t.overlay_ip_is_v6
	mov m.local_metadata___lkup_dst_ip_addr2121 t.overlay_ip
	mov m.local_metadata___meter_policy_en3131 t.meter_policy_en
	mov m.local_metadata___route_meter_class3535 t.meter_class
	mov m.local_metadata___target_stage4242 0xC9
	return
}

action route_direct_0 args instanceof route_direct_0_arg_t {
	mov m.local_metadata___meter_policy_en3131 t.meter_policy_en
	mov m.local_metadata___route_meter_class3535 t.meter_class
	mov m.local_metadata___target_stage4242 0x12C
	return
}

action route_service_tunnel_0 args instanceof route_service_tunnel_0_arg_t {
	movh h.underlay_sip_128.upper_half t.underlay_sip
	mov h.underlay_sip_128.lower_half t.underlay_sip
	xor h.underlay_sip_128.upper_half 0x0
	xor h.underlay_sip_128.lower_half 0x0
	xor h.underlay_sip_128.upper_half h.underlay_sip_128.lower_half
	jmpneq LABEL_FALSE_55 h.underlay_sip_128.upper_half 0x0
	mov m.MainControlT_tmp_72 h.u0_ipv4.src_addr
	jmp LABEL_END_59
	LABEL_FALSE_55 :	mov m.MainControlT_tmp_2 t.underlay_sip
	movh h.MainControlT_tmp_2_128.upper_half m.MainControlT_tmp_2
	mov h.MainControlT_tmp_2_128.lower_half m.MainControlT_tmp_2
	mov h.MainControlT_tmp_2_tmp.inter h.MainControlT_tmp_2_128.lower_half
	and h.MainControlT_tmp_2_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_2 h.MainControlT_tmp_2_tmp.inter
	mov h.MainControlT_tmp_2_tmp.inter h.MainControlT_tmp_2_128.upper_half
	and h.MainControlT_tmp_2_tmp.inter 0x0
	movh m.MainControlT_tmp_2 h.MainControlT_tmp_2_tmp.inter
	mov m.MainControlT_tmp_3 m.MainControlT_tmp_2
	movh h.MainControlT_tmp_3_128.upper_half m.MainControlT_tmp_3
	mov h.MainControlT_tmp_3_128.lower_half m.MainControlT_tmp_3
	mov h.MainControlT_tmp_3_tmp.inter h.MainControlT_tmp_3_128.lower_half
	and h.MainControlT_tmp_3_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_3 h.MainControlT_tmp_3_tmp.inter
	mov h.MainControlT_tmp_3_tmp.inter h.MainControlT_tmp_3_128.upper_half
	and h.MainControlT_tmp_3_tmp.inter 0x0
	movh m.MainControlT_tmp_3 h.MainControlT_tmp_3_tmp.inter
	mov m.MainControlT_tmp_4 m.MainControlT_tmp_3
	movh h.MainControlT_tmp_4_128.upper_half m.MainControlT_tmp_4
	mov h.MainControlT_tmp_4_128.lower_half m.MainControlT_tmp_4
	mov h.MainControlT_tmp_4_tmp.inter h.MainControlT_tmp_4_128.lower_half
	and h.MainControlT_tmp_4_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_4 h.MainControlT_tmp_4_tmp.inter
	mov h.MainControlT_tmp_4_tmp.inter h.MainControlT_tmp_4_128.upper_half
	and h.MainControlT_tmp_4_tmp.inter 0x0
	movh m.MainControlT_tmp_4 h.MainControlT_tmp_4_tmp.inter
	mov h.dpdk_pseudo_header.pseudo m.MainControlT_tmp_4
	mov m.MainControlT_tmp_72 h.dpdk_pseudo_header.pseudo
	LABEL_END_59 :	movh h.underlay_dip_128.upper_half t.underlay_dip
	mov h.underlay_dip_128.lower_half t.underlay_dip
	xor h.underlay_dip_128.upper_half 0x0
	xor h.underlay_dip_128.lower_half 0x0
	xor h.underlay_dip_128.upper_half h.underlay_dip_128.lower_half
	jmpneq LABEL_FALSE_56 h.underlay_dip_128.upper_half 0x0
	mov m.MainControlT_tmp_73 h.u0_ipv4.dst_addr
	jmp LABEL_END_60
	LABEL_FALSE_56 :	mov m.MainControlT_tmp_5 t.underlay_dip
	movh h.MainControlT_tmp_5_128.upper_half m.MainControlT_tmp_5
	mov h.MainControlT_tmp_5_128.lower_half m.MainControlT_tmp_5
	mov h.MainControlT_tmp_5_tmp.inter h.MainControlT_tmp_5_128.lower_half
	and h.MainControlT_tmp_5_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_5 h.MainControlT_tmp_5_tmp.inter
	mov h.MainControlT_tmp_5_tmp.inter h.MainControlT_tmp_5_128.upper_half
	and h.MainControlT_tmp_5_tmp.inter 0x0
	movh m.MainControlT_tmp_5 h.MainControlT_tmp_5_tmp.inter
	mov m.MainControlT_tmp_6 m.MainControlT_tmp_5
	movh h.MainControlT_tmp_6_128.upper_half m.MainControlT_tmp_6
	mov h.MainControlT_tmp_6_128.lower_half m.MainControlT_tmp_6
	mov h.MainControlT_tmp_6_tmp.inter h.MainControlT_tmp_6_128.lower_half
	and h.MainControlT_tmp_6_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_6 h.MainControlT_tmp_6_tmp.inter
	mov h.MainControlT_tmp_6_tmp.inter h.MainControlT_tmp_6_128.upper_half
	and h.MainControlT_tmp_6_tmp.inter 0x0
	movh m.MainControlT_tmp_6 h.MainControlT_tmp_6_tmp.inter
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	movh h.MainControlT_tmp_7_128.upper_half m.MainControlT_tmp_7
	mov h.MainControlT_tmp_7_128.lower_half m.MainControlT_tmp_7
	mov h.MainControlT_tmp_7_tmp.inter h.MainControlT_tmp_7_128.lower_half
	and h.MainControlT_tmp_7_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_7 h.MainControlT_tmp_7_tmp.inter
	mov h.MainControlT_tmp_7_tmp.inter h.MainControlT_tmp_7_128.upper_half
	and h.MainControlT_tmp_7_tmp.inter 0x0
	movh m.MainControlT_tmp_7 h.MainControlT_tmp_7_tmp.inter
	mov h.dpdk_pseudo_header.pseudo_0 m.MainControlT_tmp_7
	mov m.MainControlT_tmp_73 h.dpdk_pseudo_header.pseudo_0
	LABEL_END_60 :	jmpneq LABEL_FALSE_57 t.tunnel_key 0x0
	mov m.MainControlT_tmp_74 m.local_metadata___encap_data_vni4545
	jmp LABEL_END_61
	LABEL_FALSE_57 :	mov m.MainControlT_tmp_74 t.tunnel_key
	LABEL_END_61 :	jmpneq LABEL_FALSE_58 m.MainControlT_tmp_72 0x0
	mov m.MainControlT_tmp_77 m.local_metadata___encap_data_underlay_sip4747
	jmp LABEL_END_62
	LABEL_FALSE_58 :	mov m.MainControlT_tmp_77 m.MainControlT_tmp_72
	LABEL_END_62 :	jmpneq LABEL_FALSE_59 m.MainControlT_tmp_73 0x0
	mov m.MainControlT_tmp_78 m.local_metadata___encap_data_underlay_dip4848
	jmp LABEL_END_63
	LABEL_FALSE_59 :	mov m.MainControlT_tmp_78 m.MainControlT_tmp_73
	LABEL_END_63 :	jmpneq LABEL_FALSE_60 h.u0_ethernet.dst_addr 0x0
	mov m.MainControlT_tmp_79 m.local_metadata___overlay_data_dmac5353
	jmp LABEL_END_64
	LABEL_FALSE_60 :	mov m.MainControlT_tmp_79 h.u0_ethernet.dst_addr
	LABEL_END_64 :	mov m.local_metadata___meter_policy_en3131 t.meter_policy_en
	mov m.local_metadata___route_meter_class3535 t.meter_class
	mov m.local_metadata___target_stage4242 0x12C
	mov m.MainControlT_tmp_8 m.local_metadata___routing_actions4343
	or m.MainControlT_tmp_8 0x2
	mov m.local_metadata___routing_actions4343 m.MainControlT_tmp_8
	or m.local_metadata___routing_actions4343 0x1
	mov m.local_metadata___encap_data_dash_encapsulation5151 t.dash_encapsulation
	mov m.local_metadata___overlay_data_is_ipv65252 1
	mov m.local_metadata___overlay_data_sip5454 t.overlay_sip
	mov m.local_metadata___overlay_data_dip5555 t.overlay_dip
	mov m.local_metadata___overlay_data_sip_mask5656 t.overlay_sip_mask
	mov m.local_metadata___overlay_data_dip_mask5757 t.overlay_dip_mask
	return
}

action drop_1 args none {
	mov m.local_metadata___target_stage4242 0x12C
	mov m.local_metadata___dropped4444 1
	return
}

action drop_2 args none {
	mov m.local_metadata___target_stage4242 0x12C
	mov m.local_metadata___dropped4444 1
	return
}

action set_tunnel_mapping_0 args instanceof set_tunnel_mapping_0_arg_t {
	mov m.MainControlT_meta_42_vnet_id m.local_metadata___vnet_id22
	jmpneq LABEL_END_65 t.use_dst_vnet_vni 0x1
	mov m.MainControlT_meta_42_vnet_id m.local_metadata___dst_vnet_id33
	LABEL_END_65 :	mov m.MainControlT_tmp_80 m.local_metadata___encap_data_vni4545
	mov m.MainControlT_tmp_83 m.local_metadata___encap_data_underlay_sip4747
	jmpneq LABEL_FALSE_62 t.underlay_dip 0x0
	mov m.MainControlT_tmp_84 m.local_metadata___encap_data_underlay_dip4848
	jmp LABEL_END_66
	LABEL_FALSE_62 :	mov m.MainControlT_tmp_84 t.underlay_dip
	LABEL_END_66 :	jmpneq LABEL_FALSE_63 t.overlay_dmac 0x0
	mov m.MainControlT_tmp_85 m.local_metadata___overlay_data_dmac5353
	jmp LABEL_END_67
	LABEL_FALSE_63 :	mov m.MainControlT_tmp_85 t.overlay_dmac
	LABEL_END_67 :	mov m.local_metadata___mapping_meter_class_override3232 t.meter_class_override
	mov m.local_metadata___mapping_meter_class3636 t.meter_class
	mov m.local_metadata___target_stage4242 0x12C
	or m.local_metadata___routing_actions4343 0x1
	mov m.local_metadata___encap_data_vni4545 m.MainControlT_tmp_80
	mov m.local_metadata___encap_data_underlay_sip4747 m.MainControlT_tmp_83
	mov m.local_metadata___encap_data_underlay_dip4848 m.MainControlT_tmp_84
	mov m.local_metadata___encap_data_dash_encapsulation5151 0x1
	mov m.local_metadata___overlay_data_dmac5353 m.MainControlT_tmp_85
	return
}

action set_private_link_mapping_0 args instanceof set_private_link_mapping_0_arg_t {
	mov m.local_metadata__meta_43_overlay_data_is_ipv658 m.local_metadata___overlay_data_is_ipv65252
	mov m.local_metadata__meta_43_overlay_data_dmac59 m.local_metadata___overlay_data_dmac5353
	mov m.local_metadata__meta_43_overlay_data_sip60 m.local_metadata___overlay_data_sip5454
	mov m.local_metadata__meta_43_overlay_data_dip61 m.local_metadata___overlay_data_dip5555
	mov m.local_metadata__meta_43_overlay_data_sip_mask62 m.local_metadata___overlay_data_sip_mask5656
	mov m.local_metadata__meta_43_overlay_data_dip_mask63 m.local_metadata___overlay_data_dip_mask5757
	jmpneq LABEL_FALSE_64 t.tunnel_key 0x0
	mov m.MainControlT_tmp_86 m.local_metadata___encap_data_vni4545
	jmp LABEL_END_68
	LABEL_FALSE_64 :	mov m.MainControlT_tmp_86 t.tunnel_key
	LABEL_END_68 :	jmpneq LABEL_FALSE_65 m.local_metadata___eni_data_pl_underlay_sip1111 0x0
	mov m.MainControlT_tmp_89 m.local_metadata___encap_data_underlay_sip4747
	jmp LABEL_END_69
	LABEL_FALSE_65 :	mov m.MainControlT_tmp_89 m.local_metadata___eni_data_pl_underlay_sip1111
	LABEL_END_69 :	jmpneq LABEL_FALSE_66 t.underlay_dip 0x0
	mov m.MainControlT_tmp_90 m.local_metadata___encap_data_underlay_dip4848
	jmp LABEL_END_70
	LABEL_FALSE_66 :	mov m.MainControlT_tmp_90 t.underlay_dip
	LABEL_END_70 :	jmpneq LABEL_FALSE_67 h.u0_ethernet.dst_addr 0x0
	mov m.MainControlT_tmp_91 m.local_metadata___overlay_data_dmac5353
	jmp LABEL_END_71
	LABEL_FALSE_67 :	mov m.MainControlT_tmp_91 h.u0_ethernet.dst_addr
	LABEL_END_71 :	mov m.local_metadata__meta_43_overlay_data_is_ipv658 m.local_metadata___overlay_data_is_ipv65252
	mov m.local_metadata__meta_43_overlay_data_dmac59 m.MainControlT_tmp_91
	mov m.local_metadata__meta_43_overlay_data_sip60 m.local_metadata___overlay_data_sip5454
	mov m.local_metadata__meta_43_overlay_data_dip61 m.local_metadata___overlay_data_dip5555
	mov m.local_metadata__meta_43_overlay_data_sip_mask62 m.local_metadata___overlay_data_sip_mask5656
	mov m.local_metadata__meta_43_overlay_data_dip_mask63 m.local_metadata___overlay_data_dip_mask5757
	mov m.local_metadata__meta_43_overlay_data_is_ipv658 1
	mov m.local_metadata__meta_43_overlay_data_dmac59 m.MainControlT_tmp_91
	movh h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half m.local_metadata___eni_data_pl_sip_mask1010
	mov h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half m.local_metadata___eni_data_pl_sip_mask1010
	mov h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter 0xFFFFFFFFFFFFFFFF
	xor h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter
	xor h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter
	mov m.MainControlT_tmp_9 h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half
	movh m.MainControlT_tmp_9 h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half
	mov m.MainControlT_tmp_10 t.overlay_sip
	movh h.MainControlT_tmp_10_128.upper_half m.MainControlT_tmp_10
	mov h.MainControlT_tmp_10_128.lower_half m.MainControlT_tmp_10
	movh h.MainControlT_tmp_9_128.upper_half m.MainControlT_tmp_9
	mov h.MainControlT_tmp_9_128.lower_half m.MainControlT_tmp_9
	mov h.MainControlT_tmp_10_tmp.inter h.MainControlT_tmp_10_128.lower_half
	and h.MainControlT_tmp_10_tmp.inter h.MainControlT_tmp_9_128.lower_half
	mov m.MainControlT_tmp_10 h.MainControlT_tmp_10_tmp.inter
	mov h.MainControlT_tmp_10_tmp.inter h.MainControlT_tmp_10_128.upper_half
	and h.MainControlT_tmp_10_tmp.inter h.MainControlT_tmp_9_128.upper_half
	movh m.MainControlT_tmp_10 h.MainControlT_tmp_10_tmp.inter
	mov m.MainControlT_tmp_11 m.MainControlT_tmp_10
	movh h.MainControlT_tmp_11_128.upper_half m.MainControlT_tmp_11
	mov h.MainControlT_tmp_11_128.lower_half m.MainControlT_tmp_11
	movh h.local_metadata___eni_data_pl_sip99_128.upper_half m.local_metadata___eni_data_pl_sip99
	mov h.local_metadata___eni_data_pl_sip99_128.lower_half m.local_metadata___eni_data_pl_sip99
	mov h.MainControlT_tmp_11_tmp.inter h.MainControlT_tmp_11_128.lower_half
	or h.MainControlT_tmp_11_tmp.inter h.local_metadata___eni_data_pl_sip99_128.lower_half
	mov m.MainControlT_tmp_11 h.MainControlT_tmp_11_tmp.inter
	mov h.MainControlT_tmp_11_tmp.inter h.MainControlT_tmp_11_128.upper_half
	or h.MainControlT_tmp_11_tmp.inter h.local_metadata___eni_data_pl_sip99_128.upper_half
	movh m.MainControlT_tmp_11 h.MainControlT_tmp_11_tmp.inter
	mov m.MainControlT_tmp_12 h.u0_ipv4.src_addr
	mov m.local_metadata__meta_43_overlay_data_sip60 m.MainControlT_tmp_11
	movh h.local_metadata__meta_43_overlay_data_sip60_128.upper_half m.local_metadata__meta_43_overlay_data_sip60
	mov h.local_metadata__meta_43_overlay_data_sip60_128.lower_half m.local_metadata__meta_43_overlay_data_sip60
	movh h.MainControlT_tmp_12_128.upper_half m.MainControlT_tmp_12
	mov h.MainControlT_tmp_12_128.lower_half m.MainControlT_tmp_12
	mov h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.local_metadata__meta_43_overlay_data_sip60_128.lower_half
	or h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.MainControlT_tmp_12_128.lower_half
	mov m.local_metadata__meta_43_overlay_data_sip60 h.local_metadata__meta_43_overlay_data_sip60_tmp.inter
	mov h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.local_metadata__meta_43_overlay_data_sip60_128.upper_half
	or h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.MainControlT_tmp_12_128.upper_half
	movh m.local_metadata__meta_43_overlay_data_sip60 h.local_metadata__meta_43_overlay_data_sip60_tmp.inter
	mov m.local_metadata__meta_43_overlay_data_dip61 t.overlay_dip
	mov h.local_metadata__meta_43_overlay_data_sip_mask62_128.upper_half 0xFFFFFFFF
	mov h.local_metadata__meta_43_overlay_data_sip_mask62_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata__meta_43_overlay_data_sip_mask62 h.local_metadata__meta_43_overlay_data_sip_mask62_128.lower_half
	movh m.local_metadata__meta_43_overlay_data_sip_mask62 h.local_metadata__meta_43_overlay_data_sip_mask62_128.upper_half
	mov h.local_metadata__meta_43_overlay_data_dip_mask63_128.upper_half 0xFFFFFFFF
	mov h.local_metadata__meta_43_overlay_data_dip_mask63_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata__meta_43_overlay_data_dip_mask63 h.local_metadata__meta_43_overlay_data_dip_mask63_128.lower_half
	movh m.local_metadata__meta_43_overlay_data_dip_mask63 h.local_metadata__meta_43_overlay_data_dip_mask63_128.upper_half
	mov m.local_metadata__meta_43_overlay_data_is_ipv658 1
	mov m.local_metadata__meta_43_overlay_data_dmac59 m.MainControlT_tmp_91
	movh h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half m.local_metadata___eni_data_pl_sip_mask1010
	mov h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half m.local_metadata___eni_data_pl_sip_mask1010
	mov h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter 0xFFFFFFFFFFFFFFFF
	xor h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter
	xor h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half h.local_metadata___eni_data_pl_sip_mask1010_tmp.inter
	mov m.MainControlT_tmp_13 h.local_metadata___eni_data_pl_sip_mask1010_128.lower_half
	movh m.MainControlT_tmp_13 h.local_metadata___eni_data_pl_sip_mask1010_128.upper_half
	mov m.MainControlT_tmp_14 t.overlay_sip
	movh h.MainControlT_tmp_14_128.upper_half m.MainControlT_tmp_14
	mov h.MainControlT_tmp_14_128.lower_half m.MainControlT_tmp_14
	movh h.MainControlT_tmp_13_128.upper_half m.MainControlT_tmp_13
	mov h.MainControlT_tmp_13_128.lower_half m.MainControlT_tmp_13
	mov h.MainControlT_tmp_14_tmp.inter h.MainControlT_tmp_14_128.lower_half
	and h.MainControlT_tmp_14_tmp.inter h.MainControlT_tmp_13_128.lower_half
	mov m.MainControlT_tmp_14 h.MainControlT_tmp_14_tmp.inter
	mov h.MainControlT_tmp_14_tmp.inter h.MainControlT_tmp_14_128.upper_half
	and h.MainControlT_tmp_14_tmp.inter h.MainControlT_tmp_13_128.upper_half
	movh m.MainControlT_tmp_14 h.MainControlT_tmp_14_tmp.inter
	mov m.MainControlT_tmp_15 m.MainControlT_tmp_14
	movh h.MainControlT_tmp_15_128.upper_half m.MainControlT_tmp_15
	mov h.MainControlT_tmp_15_128.lower_half m.MainControlT_tmp_15
	movh h.local_metadata___eni_data_pl_sip99_128.upper_half m.local_metadata___eni_data_pl_sip99
	mov h.local_metadata___eni_data_pl_sip99_128.lower_half m.local_metadata___eni_data_pl_sip99
	mov h.MainControlT_tmp_15_tmp.inter h.MainControlT_tmp_15_128.lower_half
	or h.MainControlT_tmp_15_tmp.inter h.local_metadata___eni_data_pl_sip99_128.lower_half
	mov m.MainControlT_tmp_15 h.MainControlT_tmp_15_tmp.inter
	mov h.MainControlT_tmp_15_tmp.inter h.MainControlT_tmp_15_128.upper_half
	or h.MainControlT_tmp_15_tmp.inter h.local_metadata___eni_data_pl_sip99_128.upper_half
	movh m.MainControlT_tmp_15 h.MainControlT_tmp_15_tmp.inter
	mov m.MainControlT_tmp_16 h.u0_ipv4.src_addr
	mov m.local_metadata__meta_43_overlay_data_sip60 m.MainControlT_tmp_15
	movh h.local_metadata__meta_43_overlay_data_sip60_128.upper_half m.local_metadata__meta_43_overlay_data_sip60
	mov h.local_metadata__meta_43_overlay_data_sip60_128.lower_half m.local_metadata__meta_43_overlay_data_sip60
	movh h.MainControlT_tmp_16_128.upper_half m.MainControlT_tmp_16
	mov h.MainControlT_tmp_16_128.lower_half m.MainControlT_tmp_16
	mov h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.local_metadata__meta_43_overlay_data_sip60_128.lower_half
	or h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.MainControlT_tmp_16_128.lower_half
	mov m.local_metadata__meta_43_overlay_data_sip60 h.local_metadata__meta_43_overlay_data_sip60_tmp.inter
	mov h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.local_metadata__meta_43_overlay_data_sip60_128.upper_half
	or h.local_metadata__meta_43_overlay_data_sip60_tmp.inter h.MainControlT_tmp_16_128.upper_half
	movh m.local_metadata__meta_43_overlay_data_sip60 h.local_metadata__meta_43_overlay_data_sip60_tmp.inter
	mov m.local_metadata__meta_43_overlay_data_dip61 t.overlay_dip
	mov h.local_metadata__meta_43_overlay_data_sip_mask62_128.upper_half 0xFFFFFFFF
	mov h.local_metadata__meta_43_overlay_data_sip_mask62_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata__meta_43_overlay_data_sip_mask62 h.local_metadata__meta_43_overlay_data_sip_mask62_128.lower_half
	movh m.local_metadata__meta_43_overlay_data_sip_mask62 h.local_metadata__meta_43_overlay_data_sip_mask62_128.upper_half
	mov h.local_metadata__meta_43_overlay_data_dip_mask63_128.upper_half 0xFFFFFFFF
	mov h.local_metadata__meta_43_overlay_data_dip_mask63_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata__meta_43_overlay_data_dip_mask63 h.local_metadata__meta_43_overlay_data_dip_mask63_128.lower_half
	movh m.local_metadata__meta_43_overlay_data_dip_mask63 h.local_metadata__meta_43_overlay_data_dip_mask63_128.upper_half
	mov m.local_metadata___mapping_meter_class_override3232 t.meter_class_override
	mov m.local_metadata___mapping_meter_class3636 t.meter_class
	mov m.local_metadata___target_stage4242 0x12C
	mov m.MainControlT_tmp_17 m.local_metadata___routing_actions4343
	or m.MainControlT_tmp_17 0x1
	mov m.local_metadata___routing_actions4343 m.MainControlT_tmp_17
	or m.local_metadata___routing_actions4343 0x2
	mov m.local_metadata___encap_data_vni4545 m.MainControlT_tmp_86
	mov m.local_metadata___encap_data_underlay_sip4747 m.MainControlT_tmp_89
	mov m.local_metadata___encap_data_underlay_dip4848 m.MainControlT_tmp_90
	mov m.local_metadata___encap_data_dash_encapsulation5151 t.dash_encapsulation
	mov m.local_metadata___overlay_data_is_ipv65252 1
	mov m.local_metadata___overlay_data_dmac5353 m.MainControlT_tmp_91
	mov m.local_metadata___overlay_data_sip5454 m.local_metadata__meta_43_overlay_data_sip60
	mov m.local_metadata___overlay_data_dip5555 t.overlay_dip
	mov h.local_metadata___overlay_data_sip_mask5656_128.upper_half 0xFFFFFFFF
	mov h.local_metadata___overlay_data_sip_mask5656_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata___overlay_data_sip_mask5656 h.local_metadata___overlay_data_sip_mask5656_128.lower_half
	movh m.local_metadata___overlay_data_sip_mask5656 h.local_metadata___overlay_data_sip_mask5656_128.upper_half
	mov h.local_metadata___overlay_data_dip_mask5757_128.upper_half 0xFFFFFFFF
	mov h.local_metadata___overlay_data_dip_mask5757_128.lower_half 0xFFFFFFFFFFFFFFFF
	mov m.local_metadata___overlay_data_dip_mask5757 h.local_metadata___overlay_data_dip_mask5757_128.lower_half
	movh m.local_metadata___overlay_data_dip_mask5757 h.local_metadata___overlay_data_dip_mask5757_128.upper_half
	return
}

action deny args none {
	mov m.local_metadata___dropped4444 1
	return
}

action deny_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action deny_1 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action deny_3 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action accept_1 args none {
	return
}

action set_appliance args instanceof set_appliance_arg_t {
	mov m.local_metadata___encap_data_underlay_dmac5050 t.neighbor_mac
	mov m.local_metadata___encap_data_underlay_smac4949 t.mac
	return
}

action set_eni_attrs args instanceof set_eni_attrs_arg_t {
	mov m.local_metadata___eni_data_cps55 t.cps
	mov m.local_metadata___eni_data_pps66 t.pps
	mov m.local_metadata___eni_data_flows77 t.flows
	mov m.local_metadata___eni_data_admin_state88 t.admin_state
	mov m.local_metadata___eni_data_pl_sip99 t.pl_sip
	mov m.local_metadata___eni_data_pl_sip_mask1010 t.pl_sip_mask
	mov m.local_metadata___eni_data_pl_underlay_sip1111 t.pl_underlay_sip
	mov m.local_metadata___encap_data_underlay_dip4848 t.vm_underlay_dip
	jmpneq LABEL_END_84 t.dash_tunnel_dscp_mode 0x1
	mov m.local_metadata___eni_data_dscp1212 t.dscp
	LABEL_END_84 :	mov m.local_metadata___encap_data_vni4545 t.vm_vni
	mov m.local_metadata___vnet_id22 t.vnet_id
	jmpneq LABEL_FALSE_81 m.local_metadata___is_overlay_ip_v61616 0x1
	jmpneq LABEL_FALSE_82 m.local_metadata___direction00 0x1
	mov m.local_metadata___stage1_dash_acl_group_id2626 t.outbound_v6_stage1_dash_acl_group_id
	mov m.local_metadata___stage2_dash_acl_group_id2727 t.outbound_v6_stage2_dash_acl_group_id
	mov m.local_metadata___stage3_dash_acl_group_id2828 t.outbound_v6_stage3_dash_acl_group_id
	mov m.local_metadata___stage4_dash_acl_group_id2929 t.outbound_v6_stage4_dash_acl_group_id
	mov m.local_metadata___stage5_dash_acl_group_id3030 t.outbound_v6_stage5_dash_acl_group_id
	jmp LABEL_END_86
	LABEL_FALSE_82 :	mov m.local_metadata___stage1_dash_acl_group_id2626 t.inbound_v6_stage1_dash_acl_group_id
	mov m.local_metadata___stage2_dash_acl_group_id2727 t.inbound_v6_stage2_dash_acl_group_id
	mov m.local_metadata___stage3_dash_acl_group_id2828 t.inbound_v6_stage3_dash_acl_group_id
	mov m.local_metadata___stage4_dash_acl_group_id2929 t.inbound_v6_stage4_dash_acl_group_id
	mov m.local_metadata___stage5_dash_acl_group_id3030 t.inbound_v6_stage5_dash_acl_group_id
	LABEL_END_86 :	mov m.local_metadata___meter_policy_id3333 t.v6_meter_policy_id
	jmp LABEL_END_85
	LABEL_FALSE_81 :	jmpneq LABEL_FALSE_83 m.local_metadata___direction00 0x1
	mov m.local_metadata___stage1_dash_acl_group_id2626 t.outbound_v4_stage1_dash_acl_group_id
	mov m.local_metadata___stage2_dash_acl_group_id2727 t.outbound_v4_stage2_dash_acl_group_id
	mov m.local_metadata___stage3_dash_acl_group_id2828 t.outbound_v4_stage3_dash_acl_group_id
	mov m.local_metadata___stage4_dash_acl_group_id2929 t.outbound_v4_stage4_dash_acl_group_id
	mov m.local_metadata___stage5_dash_acl_group_id3030 t.outbound_v4_stage5_dash_acl_group_id
	jmp LABEL_END_87
	LABEL_FALSE_83 :	mov m.local_metadata___stage1_dash_acl_group_id2626 t.inbound_v4_stage1_dash_acl_group_id
	mov m.local_metadata___stage2_dash_acl_group_id2727 t.inbound_v4_stage2_dash_acl_group_id
	mov m.local_metadata___stage3_dash_acl_group_id2828 t.inbound_v4_stage3_dash_acl_group_id
	mov m.local_metadata___stage4_dash_acl_group_id2929 t.inbound_v4_stage4_dash_acl_group_id
	mov m.local_metadata___stage5_dash_acl_group_id3030 t.inbound_v4_stage5_dash_acl_group_id
	LABEL_END_87 :	mov m.local_metadata___meter_policy_id3333 t.v4_meter_policy_id
	LABEL_END_85 :	mov m.local_metadata___fast_path_icmp_flow_redirection_disabled411 t.disable_fast_path_icmp_flow_redirection
	return
}

action permit args none {
	return
}

action tunnel_decap_pa_validate args instanceof tunnel_decap_pa_validate_arg_t {
	mov m.local_metadata___vnet_id22 t.src_vnet_id
	return
}

action set_acl_group_attrs args instanceof set_acl_group_attrs_arg_t {
	jmpneq LABEL_FALSE_84 t.ip_addr_family 0x0
	jmpneq LABEL_END_88 m.local_metadata___is_overlay_ip_v61616 0x1
	mov m.local_metadata___dropped4444 1
	jmp LABEL_END_88
	jmp LABEL_END_88
	LABEL_FALSE_84 :	jmpneq LABEL_END_88 m.local_metadata___is_overlay_ip_v61616 0x0
	mov m.local_metadata___dropped4444 1
	LABEL_END_88 :	return
}

action direction_lookup_stage_set_outbound_direction_0 args none {
	mov m.local_metadata___direction00 0x1
	return
}

action direction_lookup_stage_set_inbound_direction_0 args none {
	mov m.local_metadata___direction00 0x2
	return
}

action eni_lookup_stage_set_eni_0 args instanceof eni_lookup_stage_set_eni_0_arg_t {
	mov m.local_metadata___eni_id44 t.eni_id
	return
}

action eni_lookup_stage_deny_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_permit_0 args none {
	return
}

action outbound_acl_permit_1 args none {
	return
}

action outbound_acl_permit_2 args none {
	return
}

action outbound_acl_permit_and_continue_0 args none {
	return
}

action outbound_acl_permit_and_continue_1 args none {
	return
}

action outbound_acl_permit_and_continue_2 args none {
	return
}

action outbound_acl_deny_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_deny_1 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_deny_2 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_deny_and_continue_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_deny_and_continue_1 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_acl_deny_and_continue_2 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action outbound_outbound_mapping_stage_set_vnet_attrs_0 args instanceof outbound_outbound_mapping_stage_set_vnet_attrs_0_arg_t {
	mov m.local_metadata___encap_data_vni4545 t.vni
	return
}

action inbound_acl_permit_0 args none {
	return
}

action inbound_acl_permit_1 args none {
	return
}

action inbound_acl_permit_2 args none {
	return
}

action inbound_acl_permit_and_continue_0 args none {
	return
}

action inbound_acl_permit_and_continue_1 args none {
	return
}

action inbound_acl_permit_and_continue_2 args none {
	return
}

action inbound_acl_deny_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action inbound_acl_deny_1 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action inbound_acl_deny_2 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action inbound_acl_deny_and_continue_0 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action inbound_acl_deny_and_continue_1 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action inbound_acl_deny_and_continue_2 args none {
	mov m.local_metadata___dropped4444 1
	return
}

action underlay_pkt_act_0 args instanceof underlay_pkt_act_0_arg_t {
	jmpneq LABEL_END_91 t.packet_action 0x0
	mov m.local_metadata___dropped4444 1
	LABEL_END_91 :	return
}

action underlay_def_act_0 args none {
	return
}

action metering_update_stage_check_ip_addr_family_0 args instanceof metering_update_stage_check_ip_addr_family_0_arg_t {
	jmpneq LABEL_FALSE_88 t.ip_addr_family 0x0
	jmpneq LABEL_END_92 m.local_metadata___is_overlay_ip_v61616 0x1
	mov m.local_metadata___dropped4444 1
	jmp LABEL_END_92
	jmp LABEL_END_92
	LABEL_FALSE_88 :	jmpneq LABEL_END_92 m.local_metadata___is_overlay_ip_v61616 0x0
	mov m.local_metadata___dropped4444 1
	LABEL_END_92 :	return
}

action metering_update_stage_set_policy_meter_class_0 args instanceof metering_update_stage_set_policy_meter_class_0_arg_t {
	mov m.local_metadata___policy_meter_class3434 t.meter_class
	return
}

action metering_update_stage_meter_bucket_action_0 args instanceof metering_update_stage_meter_bucket_action_0_arg_t {
	mov m.local_metadata___meter_bucket_index3838 t.meter_bucket_index
	return
}

table vip {
	key {
		h.u0_ipv4.dst_addr exact
	}
	actions {
		accept_1
		deny @defaultonly
	}
	default_action deny args none const
	size 0x10000
}


table appliance {
	key {
		m.local_metadata___appliance_id1515 wildcard
	}
	actions {
		set_appliance
		NoAction @defaultonly
	}
	default_action NoAction args none 
	size 0x10000
}


table eni {
	key {
		m.local_metadata___eni_id44 exact
	}
	actions {
		set_eni_attrs
		deny_0 @defaultonly
	}
	default_action deny_0 args none const
	size 0x10000
}


table pa_validation {
	key {
		m.dash_ingress_pa_validation_local_metadata___vnet_id22 exact
		m.dash_ingress_pa_validation_u0_ipv4_src_addr exact
	}
	actions {
		permit
		deny_1 @defaultonly
	}
	default_action deny_1 args none const
	size 0x10000
}


table inbound_routing {
	key {
		m.local_metadata___eni_id44 exact
		m.dash_ingress_inbound_routing_u0_vxlan_vni exact
		m.dash_ingress_inbound_routing_u0_ipv4_src_addr wildcard
	}
	actions {
		tunnel_decap_1
		tunnel_decap_pa_validate
		deny_3 @defaultonly
	}
	default_action deny_3 args none const
	size 0x10000
}


table acl_group {
	key {
		m.local_metadata___stage1_dash_acl_group_id2626 exact
	}
	actions {
		set_acl_group_attrs
		NoAction @defaultonly
	}
	default_action NoAction args none 
	size 0x10000
}


table direction_lookup_stage_direction_lookup {
	key {
		h.u0_vxlan.vni exact
	}
	actions {
		direction_lookup_stage_set_outbound_direction_0
		direction_lookup_stage_set_inbound_direction_0 @defaultonly
	}
	default_action direction_lookup_stage_set_inbound_direction_0 args none const
	size 0x10000
}


table eni_lookup_stage_eni_ether_address_map {
	key {
		m.MainControlT_eni_lookup_stage_tmp exact
	}
	actions {
		eni_lookup_stage_set_eni_0
		eni_lookup_stage_deny_0 @defaultonly
	}
	default_action eni_lookup_stage_deny_0 args none const
	size 0x10000
}


table outbound_acl_stage1 {
	key {
		m.local_metadata___stage1_dash_acl_group_id2626 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		outbound_acl_permit_0
		outbound_acl_permit_and_continue_0
		outbound_acl_deny_0
		outbound_acl_deny_and_continue_0
	}
	default_action outbound_acl_deny_0 args none 
	size 0x10000
}


table outbound_acl_stage2 {
	key {
		m.local_metadata___stage2_dash_acl_group_id2727 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		outbound_acl_permit_1
		outbound_acl_permit_and_continue_1
		outbound_acl_deny_1
		outbound_acl_deny_and_continue_1
	}
	default_action outbound_acl_deny_1 args none 
	size 0x10000
}


table outbound_acl_stage3 {
	key {
		m.local_metadata___stage3_dash_acl_group_id2828 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		outbound_acl_permit_2
		outbound_acl_permit_and_continue_2
		outbound_acl_deny_2
		outbound_acl_deny_and_continue_2
	}
	default_action outbound_acl_deny_2 args none 
	size 0x10000
}


table outbound_outbound_routing_stage_routing {
	key {
		m.local_metadata___eni_id44 exact
		m.local_metadata___is_overlay_ip_v61616 exact
		m.local_metadata___dst_ip_addr1919 lpm
	}
	actions {
		route_vnet_0
		route_vnet_direct_0
		route_direct_0
		route_service_tunnel_0
		drop_1
	}
	default_action drop_1 args none const
	size 0x10000
}


table outbound_outbound_mapping_stage_ca_to_pa {
	key {
		m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local2 exact
		m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local3 exact
		m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local4 exact
	}
	actions {
		set_tunnel_mapping_0
		set_private_link_mapping_0
		drop_2 @defaultonly
	}
	default_action drop_2 args none const
	size 0x10000
}


table outbound_outbound_mapping_stage_vnet {
	key {
		m.local_metadata___vnet_id22 exact
	}
	actions {
		outbound_outbound_mapping_stage_set_vnet_attrs_0
		NoAction @defaultonly
	}
	default_action NoAction args none 
	size 0x10000
}


table inbound_acl_stage1 {
	key {
		m.local_metadata___stage1_dash_acl_group_id2626 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		inbound_acl_permit_0
		inbound_acl_permit_and_continue_0
		inbound_acl_deny_0
		inbound_acl_deny_and_continue_0
	}
	default_action inbound_acl_deny_0 args none 
	size 0x10000
}


table inbound_acl_stage2 {
	key {
		m.local_metadata___stage2_dash_acl_group_id2727 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		inbound_acl_permit_1
		inbound_acl_permit_and_continue_1
		inbound_acl_deny_1
		inbound_acl_deny_and_continue_1
	}
	default_action inbound_acl_deny_1 args none 
	size 0x10000
}


table inbound_acl_stage3 {
	key {
		m.local_metadata___stage3_dash_acl_group_id2828 exact
		m.local_metadata___dst_ip_addr1919 wildcard
		m.local_metadata___src_ip_addr2020 wildcard
		m.local_metadata___ip_protocol1818 wildcard
		m.local_metadata___src_l4_port2424 wildcard
		m.local_metadata___dst_l4_port2525 wildcard
	}
	actions {
		inbound_acl_permit_2
		inbound_acl_permit_and_continue_2
		inbound_acl_deny_2
		inbound_acl_deny_and_continue_2
	}
	default_action inbound_acl_deny_2 args none 
	size 0x10000
}


table underlay_underlay_routing {
	key {
		m.local_metadata___dst_ip_addr1919 lpm
	}
	actions {
		underlay_pkt_act_0
		underlay_def_act_0 @defaultonly
		NoAction @defaultonly
	}
	default_action NoAction args none 
	size 0x10000
}


table metering_update_stage_meter_policy {
	key {
		m.local_metadata___meter_policy_id3333 exact
	}
	actions {
		metering_update_stage_check_ip_addr_family_0
		NoAction @defaultonly
	}
	default_action NoAction args none 
	size 0x10000
}


table metering_update_stage_meter_rule {
	key {
		m.local_metadata___meter_policy_id3333 exact
		m.dash_ingress_metering_update_stage_meter_rule_u0_ipv4_dst_a5 wildcard
	}
	actions {
		metering_update_stage_set_policy_meter_class_0
		NoAction @defaultonly
	}
	default_action NoAction args none const
	size 0x10000
}


table metering_update_stage_meter_bucket {
	key {
		m.dash_ingress_metering_update_stage_meter_bucket_local_metad6 exact
		m.dash_ingress_metering_update_stage_meter_bucket_local_metad7 exact
	}
	actions {
		metering_update_stage_meter_bucket_action_0
		NoAction @defaultonly
	}
	default_action NoAction args none const
	size 0x10000
}


table metering_update_stage_eni_meter {
	key {
		m.dash_ingress_metering_update_stage_eni_meter_local_metadata8 exact
		m.dash_ingress_metering_update_stage_eni_meter_local_metadata9 exact
		m.dash_ingress_metering_update_stage_eni_meter_local_metadat10 exact
	}
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.u0_ethernet
	jmpeq DASH_PARSER_PARSE_U0_IPV4 h.u0_ethernet.ether_type 0x800
	jmpeq DASH_PARSER_PARSE_U0_IPV6 h.u0_ethernet.ether_type 0x86DD
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_U0_IPV6 :	extract h.u0_ipv6
	jmpeq DASH_PARSER_PARSE_U0_UDP h.u0_ipv6.next_header 0x11
	jmpeq DASH_PARSER_PARSE_U0_TCP h.u0_ipv6.next_header 0x6
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_U0_IPV4 :	extract h.u0_ipv4
	mov m.MainParserT_parser_tmp_6 h.u0_ipv4.version_ihl
	shr m.MainParserT_parser_tmp_6 0x4
	mov m.MainParserT_parser_tmp_7 m.MainParserT_parser_tmp_6
	and m.MainParserT_parser_tmp_7 0xF
	mov m.MainParserT_parser_tmp_8 m.MainParserT_parser_tmp_7
	and m.MainParserT_parser_tmp_8 0xF
	jmpeq LABEL_TRUE m.MainParserT_parser_tmp_8 0x4
	mov m.MainParserT_parser_tmp_23 0x0
	jmp LABEL_END
	LABEL_TRUE :	mov m.MainParserT_parser_tmp_23 0x1
	LABEL_END :	jmpneq LABEL_END_0 m.MainParserT_parser_tmp_23 0
	mov m.pna_pre_input_metadata_parser_error 0x0
	jmp DASH_PARSER_ACCEPT
	LABEL_END_0 :	mov m.MainParserT_parser_tmp_10 h.u0_ipv4.version_ihl
	and m.MainParserT_parser_tmp_10 0xF
	mov m.MainParserT_parser_tmp_11 m.MainParserT_parser_tmp_10
	and m.MainParserT_parser_tmp_11 0xF
	jmplt LABEL_FALSE_0 m.MainParserT_parser_tmp_11 0x5
	mov m.MainParserT_parser_tmp_24 0x1
	jmp LABEL_END_1
	LABEL_FALSE_0 :	mov m.MainParserT_parser_tmp_24 0x0
	LABEL_END_1 :	jmpneq LABEL_END_2 m.MainParserT_parser_tmp_24 0
	mov m.pna_pre_input_metadata_parser_error 0x2
	jmp DASH_PARSER_ACCEPT
	LABEL_END_2 :	mov m.MainParserT_parser_tmp_13 h.u0_ipv4.version_ihl
	and m.MainParserT_parser_tmp_13 0xF
	mov m.MainParserT_parser_tmp_14 m.MainParserT_parser_tmp_13
	and m.MainParserT_parser_tmp_14 0xF
	jmpeq DASH_PARSER_DISPATCH_ON_U0_PROTOCOL m.MainParserT_parser_tmp_14 0x5
	mov m.MainParserT_parser_tmp_15 h.u0_ipv4.version_ihl
	and m.MainParserT_parser_tmp_15 0xF
	mov m.MainParserT_parser_tmp_16 m.MainParserT_parser_tmp_15
	and m.MainParserT_parser_tmp_16 0xF
	mov m.MainParserT_parser_tmp_19 m.MainParserT_parser_tmp_16
	add m.MainParserT_parser_tmp_19 0xFFFB
	mov m.MainParserT_parser_tmp_20 m.MainParserT_parser_tmp_19
	shl m.MainParserT_parser_tmp_20 0x5
	mov m.MainParserT_parser_tmp_26_extract_tmp m.MainParserT_parser_tmp_20
	shr m.MainParserT_parser_tmp_26_extract_tmp 0x3
	extract h.u0_ipv4options m.MainParserT_parser_tmp_26_extract_tmp
	DASH_PARSER_DISPATCH_ON_U0_PROTOCOL :	jmpeq DASH_PARSER_PARSE_U0_UDP h.u0_ipv4.protocol 0x11
	jmpeq DASH_PARSER_PARSE_U0_TCP h.u0_ipv4.protocol 0x6
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_U0_UDP :	extract h.u0_udp
	jmpeq DASH_PARSER_PARSE_U0_VXLAN h.u0_udp.dst_port 0x12B5
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_U0_VXLAN :	extract h.u0_vxlan
	extract h.customer_ethernet
	jmpeq DASH_PARSER_PARSE_CUSTOMER_IPV4 h.customer_ethernet.ether_type 0x800
	jmpeq DASH_PARSER_PARSE_CUSTOMER_IPV6 h.customer_ethernet.ether_type 0x86DD
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_CUSTOMER_IPV6 :	extract h.customer_ipv6
	jmpeq DASH_PARSER_PARSE_CUSTOMER_UDP h.customer_ipv6.next_header 0x11
	jmpeq DASH_PARSER_PARSE_CUSTOMER_TCP h.customer_ipv6.next_header 0x6
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_CUSTOMER_IPV4 :	extract h.customer_ipv4
	mov m.MainParserT_parser_tmp h.customer_ipv4.version_ihl
	shr m.MainParserT_parser_tmp 0x4
	mov m.MainParserT_parser_tmp_0 m.MainParserT_parser_tmp
	and m.MainParserT_parser_tmp_0 0xF
	mov m.MainParserT_parser_tmp_1 m.MainParserT_parser_tmp_0
	and m.MainParserT_parser_tmp_1 0xF
	jmpeq LABEL_TRUE_1 m.MainParserT_parser_tmp_1 0x4
	mov m.MainParserT_parser_tmp_21 0x0
	jmp LABEL_END_3
	LABEL_TRUE_1 :	mov m.MainParserT_parser_tmp_21 0x1
	LABEL_END_3 :	jmpneq LABEL_END_4 m.MainParserT_parser_tmp_21 0
	mov m.pna_pre_input_metadata_parser_error 0x0
	jmp DASH_PARSER_ACCEPT
	LABEL_END_4 :	mov m.MainParserT_parser_tmp_3 h.customer_ipv4.version_ihl
	and m.MainParserT_parser_tmp_3 0xF
	mov m.MainParserT_parser_tmp_4 m.MainParserT_parser_tmp_3
	and m.MainParserT_parser_tmp_4 0xF
	jmpeq LABEL_TRUE_2 m.MainParserT_parser_tmp_4 0x5
	mov m.MainParserT_parser_tmp_22 0x0
	jmp LABEL_END_5
	LABEL_TRUE_2 :	mov m.MainParserT_parser_tmp_22 0x1
	LABEL_END_5 :	jmpneq LABEL_END_6 m.MainParserT_parser_tmp_22 0
	mov m.pna_pre_input_metadata_parser_error 0x1
	jmp DASH_PARSER_ACCEPT
	LABEL_END_6 :	jmpeq DASH_PARSER_PARSE_CUSTOMER_UDP h.customer_ipv4.protocol 0x11
	jmpeq DASH_PARSER_PARSE_CUSTOMER_TCP h.customer_ipv4.protocol 0x6
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_CUSTOMER_UDP :	extract h.customer_udp
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_CUSTOMER_TCP :	extract h.customer_tcp
	jmp DASH_PARSER_ACCEPT
	DASH_PARSER_PARSE_U0_TCP :	extract h.u0_tcp
	DASH_PARSER_ACCEPT :	table vip
	jmpnh LABEL_END_7
	mov m.local_metadata___encap_data_underlay_sip4747 h.u0_ipv4.dst_addr
	LABEL_END_7 :	table direction_lookup_stage_direction_lookup
	table appliance
	jmpneq LABEL_FALSE_4 m.local_metadata___direction00 0x1
	mov m.MainControlT_eni_lookup_stage_tmp h.customer_ethernet.src_addr
	jmp LABEL_END_8
	LABEL_FALSE_4 :	mov m.MainControlT_eni_lookup_stage_tmp h.customer_ethernet.dst_addr
	LABEL_END_8 :	mov m.local_metadata___eni_addr11 m.MainControlT_eni_lookup_stage_tmp
	table eni_lookup_stage_eni_ether_address_map
	mov m.local_metadata___eni_data_dscp_mode1313 0x0
	mov m.MainControlT_tmp_48 h.u0_ipv4.diffserv
	and m.MainControlT_tmp_48 0x3F
	mov m.MainControlT_tmp_49 m.MainControlT_tmp_48
	and m.MainControlT_tmp_49 0x3F
	mov m.MainControlT_tmp_50 m.MainControlT_tmp_49
	and m.MainControlT_tmp_50 0x3F
	mov m.local_metadata___eni_data_dscp1212 m.MainControlT_tmp_50
	jmpneq LABEL_FALSE_6 m.local_metadata___direction00 0x1
	mov m.local_metadata___tunnel_pointer3939 0x0
	jmp LABEL_END_10
	LABEL_FALSE_6 :	jmpneq LABEL_END_10 m.local_metadata___direction00 0x2
	mov m.dash_ingress_inbound_routing_u0_vxlan_vni h.u0_vxlan.vni
	mov m.dash_ingress_inbound_routing_u0_ipv4_src_addr h.u0_ipv4.src_addr
	table inbound_routing
	jmpa LABEL_SWITCH tunnel_decap_pa_validate
	jmp LABEL_END_10
	LABEL_SWITCH :	mov m.dash_ingress_pa_validation_local_metadata___vnet_id22 m.local_metadata___vnet_id22
	mov m.dash_ingress_pa_validation_u0_ipv4_src_addr h.u0_ipv4.src_addr
	table pa_validation
	mov m.local_metadata___tunnel_pointer3939 0x0
	LABEL_END_10 :	mov m.local_metadata___is_overlay_ip_v61616 0x0
	mov m.local_metadata___ip_protocol1818 0x0
	mov h.dpdk_pseudo_header.pseudo_1 0x0
	mov m.local_metadata___dst_ip_addr1919 h.dpdk_pseudo_header.pseudo_1
	mov h.dpdk_pseudo_header.pseudo_2 0x0
	mov m.local_metadata___src_ip_addr2020 h.dpdk_pseudo_header.pseudo_2
	jmpnv LABEL_FALSE_8 h.customer_ipv6
	mov m.local_metadata___ip_protocol1818 h.customer_ipv6.next_header
	mov m.local_metadata___src_ip_addr2020 h.customer_ipv6.src_addr
	mov m.local_metadata___dst_ip_addr1919 h.customer_ipv6.dst_addr
	mov m.local_metadata___is_overlay_ip_v61616 0x1
	jmp LABEL_END_12
	LABEL_FALSE_8 :	jmpnv LABEL_END_12 h.customer_ipv4
	mov m.local_metadata___ip_protocol1818 h.customer_ipv4.protocol
	mov m.local_metadata___src_ip_addr2020 h.customer_ipv4.src_addr
	mov m.local_metadata___dst_ip_addr1919 h.customer_ipv4.dst_addr
	LABEL_END_12 :	jmpnv LABEL_FALSE_10 h.customer_tcp
	mov m.local_metadata___src_l4_port2424 h.customer_tcp.src_port
	mov m.local_metadata___dst_l4_port2525 h.customer_tcp.dst_port
	jmp LABEL_END_14
	LABEL_FALSE_10 :	jmpnv LABEL_END_14 h.customer_udp
	mov m.local_metadata___src_l4_port2424 h.customer_udp.src_port
	mov m.local_metadata___dst_l4_port2525 h.customer_udp.dst_port
	LABEL_END_14 :	table eni
	jmpneq LABEL_END_16 m.local_metadata___eni_data_admin_state88 0x0
	mov m.local_metadata___dropped4444 1
	LABEL_END_16 :	table acl_group
	jmpneq LABEL_FALSE_13 m.local_metadata___direction00 0x1
	mov m.local_metadata___target_stage4242 0xC8
	jmpneq LABEL_FALSE_14 m.local_metadata___conntrack_data_allow_out2323 0x1
	jmp LABEL_END_18
	LABEL_FALSE_14 :	mov m.MainControlT_outbound_acl_hasReturned 0
	jmpeq LABEL_END_19 m.local_metadata___stage1_dash_acl_group_id2626 0x0
	table outbound_acl_stage1
	jmpa LABEL_SWITCH_0 outbound_acl_permit_0
	jmpa LABEL_SWITCH_1 outbound_acl_deny_0
	jmp LABEL_END_19
	LABEL_SWITCH_0 :	mov m.MainControlT_outbound_acl_hasReturned 1
	jmp LABEL_END_19
	LABEL_SWITCH_1 :	mov m.MainControlT_outbound_acl_hasReturned 1
	LABEL_END_19 :	jmpneq LABEL_FALSE_16 m.MainControlT_outbound_acl_hasReturned 0x1
	jmp LABEL_END_20
	LABEL_FALSE_16 :	jmpeq LABEL_END_20 m.local_metadata___stage2_dash_acl_group_id2727 0x0
	table outbound_acl_stage2
	jmpa LABEL_SWITCH_2 outbound_acl_permit_1
	jmpa LABEL_SWITCH_3 outbound_acl_deny_1
	jmp LABEL_END_20
	LABEL_SWITCH_2 :	mov m.MainControlT_outbound_acl_hasReturned 1
	jmp LABEL_END_20
	LABEL_SWITCH_3 :	mov m.MainControlT_outbound_acl_hasReturned 1
	LABEL_END_20 :	jmpneq LABEL_FALSE_18 m.MainControlT_outbound_acl_hasReturned 0x1
	jmp LABEL_END_18
	LABEL_FALSE_18 :	jmpeq LABEL_END_18 m.local_metadata___stage3_dash_acl_group_id2828 0x0
	table outbound_acl_stage3
	LABEL_END_18 :	table outbound_outbound_routing_stage_routing
	jmpeq LABEL_FALSE_20 m.local_metadata___target_stage4242 0xC9
	jmp LABEL_END_17
	LABEL_FALSE_20 :	mov m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local2 m.local_metadata___dst_vnet_id33
	mov m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local3 m.local_metadata___is_overlay_ip_v61616
	mov m.dash_ingress_outbound_outbound_mapping_stage_ca_to_pa_local4 m.local_metadata___dst_ip_addr1919
	table outbound_outbound_mapping_stage_ca_to_pa
	jmpa LABEL_SWITCH_6 set_tunnel_mapping_0
	jmp LABEL_END_17
	LABEL_SWITCH_6 :	table outbound_outbound_mapping_stage_vnet
	jmp LABEL_END_17
	LABEL_FALSE_13 :	jmpneq LABEL_END_17 m.local_metadata___direction00 0x2
	jmpneq LABEL_FALSE_22 m.local_metadata___conntrack_data_allow_in2222 0x1
	jmp LABEL_END_26
	LABEL_FALSE_22 :	mov m.MainControlT_inbound_acl_hasReturned 0
	jmpeq LABEL_END_27 m.local_metadata___stage1_dash_acl_group_id2626 0x0
	table inbound_acl_stage1
	jmpa LABEL_SWITCH_7 inbound_acl_permit_0
	jmpa LABEL_SWITCH_8 inbound_acl_deny_0
	jmp LABEL_END_27
	LABEL_SWITCH_7 :	mov m.MainControlT_inbound_acl_hasReturned 1
	jmp LABEL_END_27
	LABEL_SWITCH_8 :	mov m.MainControlT_inbound_acl_hasReturned 1
	LABEL_END_27 :	jmpneq LABEL_FALSE_24 m.MainControlT_inbound_acl_hasReturned 0x1
	jmp LABEL_END_28
	LABEL_FALSE_24 :	jmpeq LABEL_END_28 m.local_metadata___stage2_dash_acl_group_id2727 0x0
	table inbound_acl_stage2
	jmpa LABEL_SWITCH_9 inbound_acl_permit_1
	jmpa LABEL_SWITCH_10 inbound_acl_deny_1
	jmp LABEL_END_28
	LABEL_SWITCH_9 :	mov m.MainControlT_inbound_acl_hasReturned 1
	jmp LABEL_END_28
	LABEL_SWITCH_10 :	mov m.MainControlT_inbound_acl_hasReturned 1
	LABEL_END_28 :	jmpneq LABEL_FALSE_26 m.MainControlT_inbound_acl_hasReturned 0x1
	jmp LABEL_END_26
	LABEL_FALSE_26 :	jmpeq LABEL_END_26 m.local_metadata___stage3_dash_acl_group_id2828 0x0
	table inbound_acl_stage3
	LABEL_END_26 :	jmpneq LABEL_FALSE_28 m.local_metadata___tunnel_pointer3939 0x0
	mov h.customer_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u0_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u0_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u0_ethernet.ether_type 0x800
	mov m.MainControlT_customer_ip_len 0x0
	jmpnv LABEL_END_33 h.customer_ipv4
	mov m.MainControlT_customer_ip_len h.customer_ipv4.total_len
	LABEL_END_33 :	jmpnv LABEL_END_34 h.customer_ipv6
	mov m.MainControlT_tmp_18 m.MainControlT_customer_ip_len
	add m.MainControlT_tmp_18 0x28
	mov m.MainControlT_customer_ip_len m.MainControlT_tmp_18
	add m.MainControlT_customer_ip_len h.customer_ipv6.payload_length
	LABEL_END_34 :	mov h.u0_ipv4.total_len m.MainControlT_customer_ip_len
	add h.u0_ipv4.total_len 0x32
	mov m.MainControlT_tmp_19 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_19 0xF
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_19
	or h.u0_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_20 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_20 0xF0
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_20
	or h.u0_ipv4.version_ihl 0x5
	mov h.u0_ipv4.diffserv 0x0
	mov h.u0_ipv4.identification 0x1
	mov m.MainControlT_tmp_21 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_21 0x1FFF
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_21
	or h.u0_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_22 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_22 0xE000
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_22
	or h.u0_ipv4.flags_frag_offset 0x0
	mov h.u0_ipv4.ttl 0x40
	mov h.u0_ipv4.protocol 0x11
	mov h.u0_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u0_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u0_ipv4.hdr_checksum 0x0
	mov h.u0_udp.src_port 0x0
	mov h.u0_udp.dst_port 0x12B5
	mov h.u0_udp.length m.MainControlT_customer_ip_len
	add h.u0_udp.length 0x1E
	mov h.u0_udp.checksum 0x0
	mov h.u0_vxlan.reserved 0x0
	mov h.u0_vxlan.reserved_2 0x0
	mov h.u0_vxlan.flags 0x0
	mov h.u0_vxlan.vni m.local_metadata___encap_data_vni4545
	jmp LABEL_END_32
	LABEL_FALSE_28 :	jmpneq LABEL_END_32 m.local_metadata___tunnel_pointer3939 0x1
	mov h.u0_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u1_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u1_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u1_ethernet.ether_type 0x800
	mov m.MainControlT_u0_ip_len 0x0
	jmpnv LABEL_END_36 h.u0_ipv4
	mov m.MainControlT_u0_ip_len h.u0_ipv4.total_len
	LABEL_END_36 :	jmpnv LABEL_END_37 h.u0_ipv6
	mov m.MainControlT_tmp_33 m.MainControlT_u0_ip_len
	add m.MainControlT_tmp_33 0x28
	mov m.MainControlT_u0_ip_len m.MainControlT_tmp_33
	add m.MainControlT_u0_ip_len h.u0_ipv6.payload_length
	LABEL_END_37 :	mov h.u1_ipv4.total_len m.MainControlT_u0_ip_len
	add h.u1_ipv4.total_len 0x32
	mov m.MainControlT_tmp_34 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_34 0xF
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_34
	or h.u1_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_35 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_35 0xF0
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_35
	or h.u1_ipv4.version_ihl 0x5
	mov h.u1_ipv4.diffserv 0x0
	mov h.u1_ipv4.identification 0x1
	mov m.MainControlT_tmp_36 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_36 0x1FFF
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_36
	or h.u1_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_37 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_37 0xE000
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_37
	or h.u1_ipv4.flags_frag_offset 0x0
	mov h.u1_ipv4.ttl 0x40
	mov h.u1_ipv4.protocol 0x11
	mov h.u1_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u1_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u1_ipv4.hdr_checksum 0x0
	mov h.u1_udp.src_port 0x0
	mov h.u1_udp.dst_port 0x12B5
	mov h.u1_udp.length m.MainControlT_u0_ip_len
	add h.u1_udp.length 0x1E
	mov h.u1_udp.checksum 0x0
	mov h.u1_vxlan.reserved 0x0
	mov h.u1_vxlan.reserved_2 0x0
	mov h.u1_vxlan.flags 0x0
	mov h.u1_vxlan.vni m.local_metadata___encap_data_vni4545
	LABEL_END_32 :	add m.local_metadata___tunnel_pointer3939 0x1
	LABEL_END_17 :	mov m.MainControlT_tmp m.local_metadata___routing_actions4343
	and m.MainControlT_tmp 0x2
	jmpneq LABEL_FALSE_34 m.MainControlT_tmp 0x0
	jmp LABEL_END_38
	LABEL_FALSE_34 :	validate h.u0_ipv6
	mov m.MainControlT_tmp_51 h.u0_ipv6.version_traffic_class_flow_la0
	and m.MainControlT_tmp_51 0xFFFFFFF
	mov h.u0_ipv6.version_traffic_class_flow_la0 m.MainControlT_tmp_51
	or h.u0_ipv6.version_traffic_class_flow_la0 0x60000000
	mov m.MainControlT_tmp_52 h.u0_ipv6.version_traffic_class_flow_la0
	and m.MainControlT_tmp_52 0xF00FFFFF
	mov h.u0_ipv6.version_traffic_class_flow_la0 m.MainControlT_tmp_52
	or h.u0_ipv6.version_traffic_class_flow_la0 0x0
	mov m.MainControlT_tmp_53 h.u0_ipv6.version_traffic_class_flow_la0
	and m.MainControlT_tmp_53 0xFFF00000
	mov h.u0_ipv6.version_traffic_class_flow_la0 m.MainControlT_tmp_53
	or h.u0_ipv6.version_traffic_class_flow_la0 0x0
	mov h.u0_ipv6.payload_length h.u0_ipv4.total_len
	add h.u0_ipv6.payload_length 0xFFEC
	mov h.u0_ipv6.next_header h.u0_ipv4.protocol
	mov h.u0_ipv6.hop_limit h.u0_ipv4.ttl
	movh h.local_metadata___overlay_data_dip_mask5757_128.upper_half m.local_metadata___overlay_data_dip_mask5757
	mov h.local_metadata___overlay_data_dip_mask5757_128.lower_half m.local_metadata___overlay_data_dip_mask5757
	mov h.local_metadata___overlay_data_dip_mask5757_tmp.inter 0xFFFFFFFFFFFFFFFF
	xor h.local_metadata___overlay_data_dip_mask5757_128.lower_half h.local_metadata___overlay_data_dip_mask5757_tmp.inter
	xor h.local_metadata___overlay_data_dip_mask5757_128.upper_half h.local_metadata___overlay_data_dip_mask5757_tmp.inter
	mov m.MainControlT_tmp_55 h.local_metadata___overlay_data_dip_mask5757_128.lower_half
	movh m.MainControlT_tmp_55 h.local_metadata___overlay_data_dip_mask5757_128.upper_half
	mov m.MainControlT_tmp_56 h.u0_ipv4.dst_addr
	movh h.MainControlT_tmp_56_128.upper_half m.MainControlT_tmp_56
	mov h.MainControlT_tmp_56_128.lower_half m.MainControlT_tmp_56
	movh h.MainControlT_tmp_55_128.upper_half m.MainControlT_tmp_55
	mov h.MainControlT_tmp_55_128.lower_half m.MainControlT_tmp_55
	mov h.MainControlT_tmp_56_tmp.inter h.MainControlT_tmp_56_128.lower_half
	and h.MainControlT_tmp_56_tmp.inter h.MainControlT_tmp_55_128.lower_half
	mov m.MainControlT_tmp_56 h.MainControlT_tmp_56_tmp.inter
	mov h.MainControlT_tmp_56_tmp.inter h.MainControlT_tmp_56_128.upper_half
	and h.MainControlT_tmp_56_tmp.inter h.MainControlT_tmp_55_128.upper_half
	movh m.MainControlT_tmp_56 h.MainControlT_tmp_56_tmp.inter
	mov m.MainControlT_tmp_57 m.local_metadata___overlay_data_dip5555
	movh h.MainControlT_tmp_57_128.upper_half m.MainControlT_tmp_57
	mov h.MainControlT_tmp_57_128.lower_half m.MainControlT_tmp_57
	movh h.local_metadata___overlay_data_dip_mask5757_128.upper_half m.local_metadata___overlay_data_dip_mask5757
	mov h.local_metadata___overlay_data_dip_mask5757_128.lower_half m.local_metadata___overlay_data_dip_mask5757
	mov h.MainControlT_tmp_57_tmp.inter h.MainControlT_tmp_57_128.lower_half
	and h.MainControlT_tmp_57_tmp.inter h.local_metadata___overlay_data_dip_mask5757_128.lower_half
	mov m.MainControlT_tmp_57 h.MainControlT_tmp_57_tmp.inter
	mov h.MainControlT_tmp_57_tmp.inter h.MainControlT_tmp_57_128.upper_half
	and h.MainControlT_tmp_57_tmp.inter h.local_metadata___overlay_data_dip_mask5757_128.upper_half
	movh m.MainControlT_tmp_57 h.MainControlT_tmp_57_tmp.inter
	mov h.u0_ipv6.dst_addr m.MainControlT_tmp_56
	movh h.dst_addr_128.upper_half h.u0_ipv6.dst_addr
	mov h.dst_addr_128.lower_half h.u0_ipv6.dst_addr
	movh h.MainControlT_tmp_57_128.upper_half m.MainControlT_tmp_57
	mov h.MainControlT_tmp_57_128.lower_half m.MainControlT_tmp_57
	mov h.dst_addr_tmp.inter h.dst_addr_128.lower_half
	or h.dst_addr_tmp.inter h.MainControlT_tmp_57_128.lower_half
	mov h.u0_ipv6.dst_addr h.dst_addr_tmp.inter
	mov h.dst_addr_tmp.inter h.dst_addr_128.upper_half
	or h.dst_addr_tmp.inter h.MainControlT_tmp_57_128.upper_half
	movh h.u0_ipv6.dst_addr h.dst_addr_tmp.inter
	movh h.local_metadata___overlay_data_sip_mask5656_128.upper_half m.local_metadata___overlay_data_sip_mask5656
	mov h.local_metadata___overlay_data_sip_mask5656_128.lower_half m.local_metadata___overlay_data_sip_mask5656
	mov h.local_metadata___overlay_data_sip_mask5656_tmp.inter 0xFFFFFFFFFFFFFFFF
	xor h.local_metadata___overlay_data_sip_mask5656_128.lower_half h.local_metadata___overlay_data_sip_mask5656_tmp.inter
	xor h.local_metadata___overlay_data_sip_mask5656_128.upper_half h.local_metadata___overlay_data_sip_mask5656_tmp.inter
	mov m.MainControlT_tmp_59 h.local_metadata___overlay_data_sip_mask5656_128.lower_half
	movh m.MainControlT_tmp_59 h.local_metadata___overlay_data_sip_mask5656_128.upper_half
	mov m.MainControlT_tmp_60 h.u0_ipv4.src_addr
	movh h.MainControlT_tmp_60_128.upper_half m.MainControlT_tmp_60
	mov h.MainControlT_tmp_60_128.lower_half m.MainControlT_tmp_60
	movh h.MainControlT_tmp_59_128.upper_half m.MainControlT_tmp_59
	mov h.MainControlT_tmp_59_128.lower_half m.MainControlT_tmp_59
	mov h.MainControlT_tmp_60_tmp.inter h.MainControlT_tmp_60_128.lower_half
	and h.MainControlT_tmp_60_tmp.inter h.MainControlT_tmp_59_128.lower_half
	mov m.MainControlT_tmp_60 h.MainControlT_tmp_60_tmp.inter
	mov h.MainControlT_tmp_60_tmp.inter h.MainControlT_tmp_60_128.upper_half
	and h.MainControlT_tmp_60_tmp.inter h.MainControlT_tmp_59_128.upper_half
	movh m.MainControlT_tmp_60 h.MainControlT_tmp_60_tmp.inter
	mov m.MainControlT_tmp_61 m.local_metadata___overlay_data_sip5454
	movh h.MainControlT_tmp_61_128.upper_half m.MainControlT_tmp_61
	mov h.MainControlT_tmp_61_128.lower_half m.MainControlT_tmp_61
	movh h.local_metadata___overlay_data_sip_mask5656_128.upper_half m.local_metadata___overlay_data_sip_mask5656
	mov h.local_metadata___overlay_data_sip_mask5656_128.lower_half m.local_metadata___overlay_data_sip_mask5656
	mov h.MainControlT_tmp_61_tmp.inter h.MainControlT_tmp_61_128.lower_half
	and h.MainControlT_tmp_61_tmp.inter h.local_metadata___overlay_data_sip_mask5656_128.lower_half
	mov m.MainControlT_tmp_61 h.MainControlT_tmp_61_tmp.inter
	mov h.MainControlT_tmp_61_tmp.inter h.MainControlT_tmp_61_128.upper_half
	and h.MainControlT_tmp_61_tmp.inter h.local_metadata___overlay_data_sip_mask5656_128.upper_half
	movh m.MainControlT_tmp_61 h.MainControlT_tmp_61_tmp.inter
	mov h.u0_ipv6.src_addr m.MainControlT_tmp_60
	movh h.src_addr_128.upper_half h.u0_ipv6.src_addr
	mov h.src_addr_128.lower_half h.u0_ipv6.src_addr
	movh h.MainControlT_tmp_61_128.upper_half m.MainControlT_tmp_61
	mov h.MainControlT_tmp_61_128.lower_half m.MainControlT_tmp_61
	mov h.src_addr_tmp.inter h.src_addr_128.lower_half
	or h.src_addr_tmp.inter h.MainControlT_tmp_61_128.lower_half
	mov h.u0_ipv6.src_addr h.src_addr_tmp.inter
	mov h.src_addr_tmp.inter h.src_addr_128.upper_half
	or h.src_addr_tmp.inter h.MainControlT_tmp_61_128.upper_half
	movh h.u0_ipv6.src_addr h.src_addr_tmp.inter
	invalidate h.u0_ipv4
	mov h.u0_ethernet.ether_type 0x86DD
	LABEL_END_38 :	mov m.MainControlT_tmp_0 m.local_metadata___routing_actions4343
	and m.MainControlT_tmp_0 0x4
	jmpneq LABEL_FALSE_35 m.MainControlT_tmp_0 0x0
	jmp LABEL_END_39
	LABEL_FALSE_35 :	validate h.u0_ipv4
	mov m.MainControlT_tmp_62 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_62 0xF
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_62
	or h.u0_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_63 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_63 0xF0
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_63
	or h.u0_ipv4.version_ihl 0x5
	mov h.u0_ipv4.diffserv 0x0
	mov h.u0_ipv4.total_len h.u0_ipv6.payload_length
	add h.u0_ipv4.total_len 0x14
	mov h.u0_ipv4.identification 0x1
	mov m.MainControlT_tmp_64 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_64 0x1FFF
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_64
	or h.u0_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_65 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_65 0xE000
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_65
	or h.u0_ipv4.flags_frag_offset 0x0
	mov h.u0_ipv4.protocol h.u0_ipv6.next_header
	mov h.u0_ipv4.ttl h.u0_ipv6.hop_limit
	mov h.u0_ipv4.hdr_checksum 0x0
	mov m.MainControlT_tmp_66 m.local_metadata___overlay_data_dip5555
	movh h.MainControlT_tmp_66_128.upper_half m.MainControlT_tmp_66
	mov h.MainControlT_tmp_66_128.lower_half m.MainControlT_tmp_66
	mov h.MainControlT_tmp_66_tmp.inter h.MainControlT_tmp_66_128.lower_half
	and h.MainControlT_tmp_66_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_66 h.MainControlT_tmp_66_tmp.inter
	mov h.MainControlT_tmp_66_tmp.inter h.MainControlT_tmp_66_128.upper_half
	and h.MainControlT_tmp_66_tmp.inter 0x0
	movh m.MainControlT_tmp_66 h.MainControlT_tmp_66_tmp.inter
	mov m.MainControlT_tmp_67 m.MainControlT_tmp_66
	movh h.MainControlT_tmp_67_128.upper_half m.MainControlT_tmp_67
	mov h.MainControlT_tmp_67_128.lower_half m.MainControlT_tmp_67
	mov h.MainControlT_tmp_67_tmp.inter h.MainControlT_tmp_67_128.lower_half
	and h.MainControlT_tmp_67_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_67 h.MainControlT_tmp_67_tmp.inter
	mov h.MainControlT_tmp_67_tmp.inter h.MainControlT_tmp_67_128.upper_half
	and h.MainControlT_tmp_67_tmp.inter 0x0
	movh m.MainControlT_tmp_67 h.MainControlT_tmp_67_tmp.inter
	mov m.MainControlT_tmp_68 m.MainControlT_tmp_67
	movh h.MainControlT_tmp_68_128.upper_half m.MainControlT_tmp_68
	mov h.MainControlT_tmp_68_128.lower_half m.MainControlT_tmp_68
	mov h.MainControlT_tmp_68_tmp.inter h.MainControlT_tmp_68_128.lower_half
	and h.MainControlT_tmp_68_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_68 h.MainControlT_tmp_68_tmp.inter
	mov h.MainControlT_tmp_68_tmp.inter h.MainControlT_tmp_68_128.upper_half
	and h.MainControlT_tmp_68_tmp.inter 0x0
	movh m.MainControlT_tmp_68 h.MainControlT_tmp_68_tmp.inter
	mov h.u0_ipv4.dst_addr m.MainControlT_tmp_68
	mov m.MainControlT_tmp_69 m.local_metadata___overlay_data_sip5454
	movh h.MainControlT_tmp_69_128.upper_half m.MainControlT_tmp_69
	mov h.MainControlT_tmp_69_128.lower_half m.MainControlT_tmp_69
	mov h.MainControlT_tmp_69_tmp.inter h.MainControlT_tmp_69_128.lower_half
	and h.MainControlT_tmp_69_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_69 h.MainControlT_tmp_69_tmp.inter
	mov h.MainControlT_tmp_69_tmp.inter h.MainControlT_tmp_69_128.upper_half
	and h.MainControlT_tmp_69_tmp.inter 0x0
	movh m.MainControlT_tmp_69 h.MainControlT_tmp_69_tmp.inter
	mov m.MainControlT_tmp_70 m.MainControlT_tmp_69
	movh h.MainControlT_tmp_70_128.upper_half m.MainControlT_tmp_70
	mov h.MainControlT_tmp_70_128.lower_half m.MainControlT_tmp_70
	mov h.MainControlT_tmp_70_tmp.inter h.MainControlT_tmp_70_128.lower_half
	and h.MainControlT_tmp_70_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_70 h.MainControlT_tmp_70_tmp.inter
	mov h.MainControlT_tmp_70_tmp.inter h.MainControlT_tmp_70_128.upper_half
	and h.MainControlT_tmp_70_tmp.inter 0x0
	movh m.MainControlT_tmp_70 h.MainControlT_tmp_70_tmp.inter
	mov m.MainControlT_tmp_71 m.MainControlT_tmp_70
	movh h.MainControlT_tmp_71_128.upper_half m.MainControlT_tmp_71
	mov h.MainControlT_tmp_71_128.lower_half m.MainControlT_tmp_71
	mov h.MainControlT_tmp_71_tmp.inter h.MainControlT_tmp_71_128.lower_half
	and h.MainControlT_tmp_71_tmp.inter 0xFFFFFFFF
	mov m.MainControlT_tmp_71 h.MainControlT_tmp_71_tmp.inter
	mov h.MainControlT_tmp_71_tmp.inter h.MainControlT_tmp_71_128.upper_half
	and h.MainControlT_tmp_71_tmp.inter 0x0
	movh m.MainControlT_tmp_71 h.MainControlT_tmp_71_tmp.inter
	mov h.u0_ipv4.src_addr m.MainControlT_tmp_71
	invalidate h.u0_ipv6
	mov h.u0_ethernet.ether_type 0x800
	LABEL_END_39 :	mov m.MainControlT_tmp_1 m.local_metadata___routing_actions4343
	and m.MainControlT_tmp_1 0x1
	jmpneq LABEL_FALSE_36 m.MainControlT_tmp_1 0x0
	jmp LABEL_END_40
	LABEL_FALSE_36 :	jmpneq LABEL_FALSE_37 m.local_metadata___encap_data_dash_encapsulation5151 0x1
	jmpneq LABEL_FALSE_38 m.local_metadata___tunnel_pointer3939 0x0
	mov h.customer_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u0_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u0_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u0_ethernet.ether_type 0x800
	mov m.MainControlT_customer_ip_len_0 0x0
	jmpnv LABEL_END_43 h.customer_ipv4
	mov m.MainControlT_customer_ip_len_0 h.customer_ipv4.total_len
	LABEL_END_43 :	jmpnv LABEL_END_44 h.customer_ipv6
	mov m.MainControlT_tmp_23 m.MainControlT_customer_ip_len_0
	add m.MainControlT_tmp_23 0x28
	mov m.MainControlT_customer_ip_len_0 m.MainControlT_tmp_23
	add m.MainControlT_customer_ip_len_0 h.customer_ipv6.payload_length
	LABEL_END_44 :	mov h.u0_ipv4.total_len m.MainControlT_customer_ip_len_0
	add h.u0_ipv4.total_len 0x32
	mov m.MainControlT_tmp_24 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_24 0xF
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_24
	or h.u0_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_25 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_25 0xF0
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_25
	or h.u0_ipv4.version_ihl 0x5
	mov h.u0_ipv4.diffserv 0x0
	mov h.u0_ipv4.identification 0x1
	mov m.MainControlT_tmp_26 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_26 0x1FFF
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_26
	or h.u0_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_27 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_27 0xE000
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_27
	or h.u0_ipv4.flags_frag_offset 0x0
	mov h.u0_ipv4.ttl 0x40
	mov h.u0_ipv4.protocol 0x11
	mov h.u0_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u0_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u0_ipv4.hdr_checksum 0x0
	mov h.u0_udp.src_port 0x0
	mov h.u0_udp.dst_port 0x12B5
	mov h.u0_udp.length m.MainControlT_customer_ip_len_0
	add h.u0_udp.length 0x1E
	mov h.u0_udp.checksum 0x0
	mov h.u0_vxlan.reserved 0x0
	mov h.u0_vxlan.reserved_2 0x0
	mov h.u0_vxlan.flags 0x0
	mov h.u0_vxlan.vni m.local_metadata___encap_data_vni4545
	jmp LABEL_END_41
	LABEL_FALSE_38 :	jmpneq LABEL_END_41 m.local_metadata___tunnel_pointer3939 0x1
	mov h.u0_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u1_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u1_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u1_ethernet.ether_type 0x800
	mov m.MainControlT_u0_ip_len_0 0x0
	jmpnv LABEL_END_46 h.u0_ipv4
	mov m.MainControlT_u0_ip_len_0 h.u0_ipv4.total_len
	LABEL_END_46 :	jmpnv LABEL_END_47 h.u0_ipv6
	mov m.MainControlT_tmp_38 m.MainControlT_u0_ip_len_0
	add m.MainControlT_tmp_38 0x28
	mov m.MainControlT_u0_ip_len_0 m.MainControlT_tmp_38
	add m.MainControlT_u0_ip_len_0 h.u0_ipv6.payload_length
	LABEL_END_47 :	mov h.u1_ipv4.total_len m.MainControlT_u0_ip_len_0
	add h.u1_ipv4.total_len 0x32
	mov m.MainControlT_tmp_39 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_39 0xF
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_39
	or h.u1_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_40 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_40 0xF0
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_40
	or h.u1_ipv4.version_ihl 0x5
	mov h.u1_ipv4.diffserv 0x0
	mov h.u1_ipv4.identification 0x1
	mov m.MainControlT_tmp_41 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_41 0x1FFF
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_41
	or h.u1_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_42 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_42 0xE000
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_42
	or h.u1_ipv4.flags_frag_offset 0x0
	mov h.u1_ipv4.ttl 0x40
	mov h.u1_ipv4.protocol 0x11
	mov h.u1_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u1_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u1_ipv4.hdr_checksum 0x0
	mov h.u1_udp.src_port 0x0
	mov h.u1_udp.dst_port 0x12B5
	mov h.u1_udp.length m.MainControlT_u0_ip_len_0
	add h.u1_udp.length 0x1E
	mov h.u1_udp.checksum 0x0
	mov h.u1_vxlan.reserved 0x0
	mov h.u1_vxlan.reserved_2 0x0
	mov h.u1_vxlan.flags 0x0
	mov h.u1_vxlan.vni m.local_metadata___encap_data_vni4545
	jmp LABEL_END_41
	LABEL_FALSE_37 :	jmpneq LABEL_END_41 m.local_metadata___encap_data_dash_encapsulation5151 0x2
	jmpneq LABEL_FALSE_45 m.local_metadata___tunnel_pointer3939 0x0
	mov h.customer_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u0_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u0_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u0_ethernet.ether_type 0x800
	mov m.MainControlT_customer_ip_len_1 0x0
	jmpnv LABEL_END_50 h.customer_ipv4
	mov m.MainControlT_customer_ip_len_1 h.customer_ipv4.total_len
	LABEL_END_50 :	jmpnv LABEL_END_51 h.customer_ipv6
	mov m.MainControlT_tmp_28 m.MainControlT_customer_ip_len_1
	add m.MainControlT_tmp_28 0x28
	mov m.MainControlT_customer_ip_len_1 m.MainControlT_tmp_28
	add m.MainControlT_customer_ip_len_1 h.customer_ipv6.payload_length
	LABEL_END_51 :	mov h.u0_ipv4.total_len m.MainControlT_customer_ip_len_1
	add h.u0_ipv4.total_len 0x32
	mov m.MainControlT_tmp_29 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_29 0xF
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_29
	or h.u0_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_30 h.u0_ipv4.version_ihl
	and m.MainControlT_tmp_30 0xF0
	mov h.u0_ipv4.version_ihl m.MainControlT_tmp_30
	or h.u0_ipv4.version_ihl 0x5
	mov h.u0_ipv4.diffserv 0x0
	mov h.u0_ipv4.identification 0x1
	mov m.MainControlT_tmp_31 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_31 0x1FFF
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_31
	or h.u0_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_32 h.u0_ipv4.flags_frag_offset
	and m.MainControlT_tmp_32 0xE000
	mov h.u0_ipv4.flags_frag_offset m.MainControlT_tmp_32
	or h.u0_ipv4.flags_frag_offset 0x0
	mov h.u0_ipv4.ttl 0x40
	mov h.u0_ipv4.protocol 0x11
	mov h.u0_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u0_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u0_ipv4.hdr_checksum 0x0
	mov h.u0_udp.src_port 0x0
	mov h.u0_udp.dst_port 0x12B5
	mov h.u0_udp.length m.MainControlT_customer_ip_len_1
	add h.u0_udp.length 0x1E
	mov h.u0_udp.checksum 0x0
	mov h.u0_vxlan.reserved 0x0
	mov h.u0_vxlan.reserved_2 0x0
	mov h.u0_vxlan.flags 0x0
	mov h.u0_vxlan.vni m.local_metadata___encap_data_vni4545
	jmp LABEL_END_41
	LABEL_FALSE_45 :	jmpneq LABEL_END_41 m.local_metadata___tunnel_pointer3939 0x1
	mov h.u0_ethernet.dst_addr m.local_metadata___overlay_data_dmac5353
	mov h.u1_ethernet.dst_addr m.local_metadata___encap_data_underlay_dmac5050
	mov h.u1_ethernet.src_addr m.local_metadata___encap_data_underlay_smac4949
	mov h.u1_ethernet.ether_type 0x800
	mov m.MainControlT_u0_ip_len_1 0x0
	jmpnv LABEL_END_53 h.u0_ipv4
	mov m.MainControlT_u0_ip_len_1 h.u0_ipv4.total_len
	LABEL_END_53 :	jmpnv LABEL_END_54 h.u0_ipv6
	mov m.MainControlT_tmp_43 m.MainControlT_u0_ip_len_1
	add m.MainControlT_tmp_43 0x28
	mov m.MainControlT_u0_ip_len_1 m.MainControlT_tmp_43
	add m.MainControlT_u0_ip_len_1 h.u0_ipv6.payload_length
	LABEL_END_54 :	mov h.u1_ipv4.total_len m.MainControlT_u0_ip_len_1
	add h.u1_ipv4.total_len 0x32
	mov m.MainControlT_tmp_44 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_44 0xF
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_44
	or h.u1_ipv4.version_ihl 0x40
	mov m.MainControlT_tmp_45 h.u1_ipv4.version_ihl
	and m.MainControlT_tmp_45 0xF0
	mov h.u1_ipv4.version_ihl m.MainControlT_tmp_45
	or h.u1_ipv4.version_ihl 0x5
	mov h.u1_ipv4.diffserv 0x0
	mov h.u1_ipv4.identification 0x1
	mov m.MainControlT_tmp_46 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_46 0x1FFF
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_46
	or h.u1_ipv4.flags_frag_offset 0x0
	mov m.MainControlT_tmp_47 h.u1_ipv4.flags_frag_offset
	and m.MainControlT_tmp_47 0xE000
	mov h.u1_ipv4.flags_frag_offset m.MainControlT_tmp_47
	or h.u1_ipv4.flags_frag_offset 0x0
	mov h.u1_ipv4.ttl 0x40
	mov h.u1_ipv4.protocol 0x11
	mov h.u1_ipv4.dst_addr m.local_metadata___encap_data_underlay_dip4848
	mov h.u1_ipv4.src_addr m.local_metadata___encap_data_underlay_sip4747
	mov h.u1_ipv4.hdr_checksum 0x0
	mov h.u1_udp.src_port 0x0
	mov h.u1_udp.dst_port 0x12B5
	mov h.u1_udp.length m.MainControlT_u0_ip_len_1
	add h.u1_udp.length 0x1E
	mov h.u1_udp.checksum 0x0
	mov h.u1_vxlan.reserved 0x0
	mov h.u1_vxlan.reserved_2 0x0
	mov h.u1_vxlan.flags 0x0
	mov h.u1_vxlan.vni m.local_metadata___encap_data_vni4545
	LABEL_END_41 :	add m.local_metadata___tunnel_pointer3939 0x1
	LABEL_END_40 :	mov m.local_metadata___dst_ip_addr1919 h.u0_ipv4.dst_addr
	table underlay_underlay_routing
	jmpneq LABEL_END_55 m.local_metadata___meter_policy_en3131 0x1
	table metering_update_stage_meter_policy
	mov m.dash_ingress_metering_update_stage_meter_rule_u0_ipv4_dst_a5 h.u0_ipv4.dst_addr
	table metering_update_stage_meter_rule
	LABEL_END_55 :	jmpneq LABEL_FALSE_52 m.local_metadata___meter_policy_en3131 0x1
	mov m.local_metadata___meter_class3737 m.local_metadata___policy_meter_class3434
	jmp LABEL_END_56
	LABEL_FALSE_52 :	mov m.local_metadata___meter_class3737 m.local_metadata___route_meter_class3535
	LABEL_END_56 :	jmpeq LABEL_TRUE_53 m.local_metadata___meter_class3737 0x0
	jmpeq LABEL_TRUE_53 m.local_metadata___mapping_meter_class_override3232 0x1
	jmp LABEL_END_57
	LABEL_TRUE_53 :	mov m.local_metadata___meter_class3737 m.local_metadata___mapping_meter_class3636
	LABEL_END_57 :	mov m.dash_ingress_metering_update_stage_meter_bucket_local_metad6 m.local_metadata___eni_id44
	mov m.dash_ingress_metering_update_stage_meter_bucket_local_metad7 m.local_metadata___meter_class3737
	table metering_update_stage_meter_bucket
	mov m.dash_ingress_metering_update_stage_eni_meter_local_metadata8 m.local_metadata___eni_id44
	mov m.dash_ingress_metering_update_stage_eni_meter_local_metadata9 m.local_metadata___direction00
	mov m.dash_ingress_metering_update_stage_eni_meter_local_metadat10 m.local_metadata___dropped4444
	table metering_update_stage_eni_meter
	jmpneq LABEL_END_58 m.local_metadata___dropped4444 0x1
	drop
	LABEL_END_58 :	emit h.u0_ethernet
	emit h.u0_ipv4
	emit h.u0_ipv4options
	emit h.u0_ipv6
	emit h.u0_udp
	emit h.u0_tcp
	emit h.u0_vxlan
	emit h.u0_nvgre
	emit h.customer_ethernet
	emit h.customer_ipv4
	emit h.customer_ipv6
	emit h.customer_tcp
	emit h.customer_udp
	tx m.pna_main_output_metadata_output_port
}


