
struct eth_t {
	bit<48> da
	bit<48> sa
	bit<16> type
}

struct udp_t {
	bit<16> sport
	bit<16> dport
	bit<16> length
	bit<16> csum
}

struct tcp_t {
	bit<16> sport
	bit<16> dport
	bit<32> seqno
	bit<32> ackno
	;oldname:offset_reserved_urg_ack_psh_rst_syn_fin
	bit<16> offset_reserved_urg_ack_psh_r0
	bit<16> window
	bit<16> csum
	bit<16> urgptr
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> dscp_ecn
	bit<16> length
	bit<16> identification
	bit<16> rsvd_df_mf_frag_off
	bit<8> ttl
	bit<8> protocol
	bit<16> csum
	bit<32> src_ip
	bit<32> dst_ip
}

struct encap_one_tunnel_layer_ipv4_arg_t {
	bit<48> mac_da
	bit<48> mac_sa
	bit<32> ipv4_sa
	bit<32> ipv4_da
}

header mac instanceof eth_t
header ipv4_0 instanceof ipv4_t
header ipv4_1 instanceof ipv4_t
header ipv4_2 instanceof ipv4_t
header ipv4_3 instanceof ipv4_t

header udp instanceof udp_t
header tcp instanceof tcp_t

struct user_meta_t {
	bit<32> pna_main_input_metadata_input_port
	bit<16> local_metadata_L2_packet_len_bytes
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControlT_tmp
	bit<8> MainControlT_tmp_0
	bit<8> MainControlT_tmp_1
	bit<8> MainControlT_tmp_2
	bit<16> MainControlT_tmp_3
	bit<16> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<16> MainControlT_tmp_6
}
metadata instanceof user_meta_t

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action encap_one_tunnel_layer_ipv4 args instanceof encap_one_tunnel_layer_ipv4_arg_t {
	jmpnv LABEL_FALSE_0 h.ipv4_2
	validate h.ipv4_3
	jmp LABEL_END_0
	LABEL_FALSE_0 :	invalidate h.ipv4_3
	LABEL_END_0 :	mov h.ipv4_3.version_ihl h.ipv4_2.version_ihl
	mov h.ipv4_3.dscp_ecn h.ipv4_2.dscp_ecn
	mov h.ipv4_3.length h.ipv4_2.length
	mov h.ipv4_3.identification h.ipv4_2.identification
	mov h.ipv4_3.rsvd_df_mf_frag_off h.ipv4_2.rsvd_df_mf_frag_off
	mov h.ipv4_3.ttl h.ipv4_2.ttl
	mov h.ipv4_3.protocol h.ipv4_2.protocol
	mov h.ipv4_3.csum h.ipv4_2.csum
	mov h.ipv4_3.src_ip h.ipv4_2.src_ip
	mov h.ipv4_3.dst_ip h.ipv4_2.dst_ip
	jmpnv LABEL_FALSE_1 h.ipv4_1
	validate h.ipv4_2
	jmp LABEL_END_1
	LABEL_FALSE_1 :	invalidate h.ipv4_2
	LABEL_END_1 :	mov h.ipv4_2.version_ihl h.ipv4_1.version_ihl
	mov h.ipv4_2.dscp_ecn h.ipv4_1.dscp_ecn
	mov h.ipv4_2.length h.ipv4_1.length
	mov h.ipv4_2.identification h.ipv4_1.identification
	mov h.ipv4_2.rsvd_df_mf_frag_off h.ipv4_1.rsvd_df_mf_frag_off
	mov h.ipv4_2.ttl h.ipv4_1.ttl
	mov h.ipv4_2.protocol h.ipv4_1.protocol
	mov h.ipv4_2.csum h.ipv4_1.csum
	mov h.ipv4_2.src_ip h.ipv4_1.src_ip
	mov h.ipv4_2.dst_ip h.ipv4_1.dst_ip
	jmpnv LABEL_FALSE_2 h.ipv4_0
	validate h.ipv4_1
	jmp LABEL_END_2
	LABEL_FALSE_2 :	invalidate h.ipv4_1
	LABEL_END_2 :	mov h.ipv4_1.version_ihl h.ipv4_0.version_ihl
	mov h.ipv4_1.dscp_ecn h.ipv4_0.dscp_ecn
	mov h.ipv4_1.length h.ipv4_0.length
	mov h.ipv4_1.identification h.ipv4_0.identification
	mov h.ipv4_1.rsvd_df_mf_frag_off h.ipv4_0.rsvd_df_mf_frag_off
	mov h.ipv4_1.ttl h.ipv4_0.ttl
	mov h.ipv4_1.protocol h.ipv4_0.protocol
	mov h.ipv4_1.csum h.ipv4_0.csum
	mov h.ipv4_1.src_ip h.ipv4_0.src_ip
	mov h.ipv4_1.dst_ip h.ipv4_0.dst_ip
	invalidate h.ipv4_0
	mov h.mac.da t.mac_da
	mov h.mac.sa t.mac_sa
	mov h.mac.type 0x800
	validate h.ipv4_0
	mov m.MainControlT_tmp h.ipv4_0.version_ihl
	and m.MainControlT_tmp 0xF
	mov h.ipv4_0.version_ihl m.MainControlT_tmp
	or h.ipv4_0.version_ihl 0x40
	mov m.MainControlT_tmp_0 h.ipv4_0.version_ihl
	and m.MainControlT_tmp_0 0xF0
	mov h.ipv4_0.version_ihl m.MainControlT_tmp_0
	or h.ipv4_0.version_ihl 0x5
	mov m.MainControlT_tmp_1 h.ipv4_0.dscp_ecn
	and m.MainControlT_tmp_1 0x3
	mov h.ipv4_0.dscp_ecn m.MainControlT_tmp_1
	or h.ipv4_0.dscp_ecn 0x14
	mov m.MainControlT_tmp_2 h.ipv4_0.dscp_ecn
	and m.MainControlT_tmp_2 0xFC
	mov h.ipv4_0.dscp_ecn m.MainControlT_tmp_2
	or h.ipv4_0.dscp_ecn 0x0
	mov h.ipv4_0.length 0x14
	add h.ipv4_0.length m.local_metadata_L2_packet_len_bytes
	mov h.ipv4_0.identification 0x0
	mov m.MainControlT_tmp_3 h.ipv4_0.rsvd_df_mf_frag_off
	and m.MainControlT_tmp_3 0x7FFF
	mov h.ipv4_0.rsvd_df_mf_frag_off m.MainControlT_tmp_3
	or h.ipv4_0.rsvd_df_mf_frag_off 0x0
	mov m.MainControlT_tmp_4 h.ipv4_0.rsvd_df_mf_frag_off
	and m.MainControlT_tmp_4 0xBFFF
	mov h.ipv4_0.rsvd_df_mf_frag_off m.MainControlT_tmp_4
	or h.ipv4_0.rsvd_df_mf_frag_off 0x0
	mov m.MainControlT_tmp_5 h.ipv4_0.rsvd_df_mf_frag_off
	and m.MainControlT_tmp_5 0xDFFF
	mov h.ipv4_0.rsvd_df_mf_frag_off m.MainControlT_tmp_5
	or h.ipv4_0.rsvd_df_mf_frag_off 0x0
	mov m.MainControlT_tmp_6 h.ipv4_0.rsvd_df_mf_frag_off
	and m.MainControlT_tmp_6 0xE000
	mov h.ipv4_0.rsvd_df_mf_frag_off m.MainControlT_tmp_6
	or h.ipv4_0.rsvd_df_mf_frag_off 0x0
	mov h.ipv4_0.ttl 0x40
	mov h.ipv4_0.protocol 0x4
	mov h.ipv4_0.csum 0x0
	mov h.ipv4_0.src_ip t.ipv4_sa
	mov h.ipv4_0.dst_ip t.ipv4_da
	return
}

action decap_one_tunnel_layer_just_before_eth args none {
	return
}

table header_mod {
	key {
		h.mac.da exact
	}
	actions {
		encap_one_tunnel_layer_ipv4
		decap_one_tunnel_layer_just_before_eth
		NoAction
	}
	default_action NoAction args none const
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.mac
	jmpeq MAINPARSERIMPL_PARSE_IPV4_DEPTH0 h.mac.type 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_DEPTH0 :	extract h.ipv4_0
	jmpeq MAINPARSERIMPL_PARSE_IPV4_DEPTH1 h.ipv4_0.protocol 0x4
	jmpeq MAINPARSERIMPL_PARSE_UDP h.ipv4_0.protocol 0x11
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4_0.protocol 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_DEPTH1 :	extract h.ipv4_1
	jmpeq MAINPARSERIMPL_PARSE_IPV4_DEPTH2 h.ipv4_1.protocol 0x4
	jmpeq MAINPARSERIMPL_PARSE_UDP h.ipv4_1.protocol 0x11
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4_1.protocol 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_DEPTH2 :	extract h.ipv4_2
	jmpeq MAINPARSERIMPL_PARSE_UDP h.ipv4_2.protocol 0x11
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4_2.protocol 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_UDP :	extract h.udp
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_TCP :	extract h.tcp
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.mac
	table header_mod
	LABEL_END :	mov m.pna_main_output_metadata_output_port 0x1
	emit h.mac
	emit h.ipv4_0
	emit h.ipv4_1
	emit h.ipv4_2
	emit h.ipv4_3
	emit h.tcp
	emit h.udp
	tx m.pna_main_output_metadata_output_port
}


