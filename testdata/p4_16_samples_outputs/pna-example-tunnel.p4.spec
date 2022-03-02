
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> dscp_ecn
	bit<16> total_len
	bit<16> identification
	bit<16> reserved_do_not_fragment_more_fragments_frag_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> header_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct tunnel_decap_decap_outer_ipv4_0_arg_t {
	bit<24> tunnel_id
}

struct tunnel_encap_set_tunnel_0_arg_t {
	bit<32> dst_addr
}

header outer_ethernet instanceof ethernet_t
header outer_ipv4 instanceof ipv4_t
header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

struct local_metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata__outer_ipv4_dst0
	bit<24> local_metadata__tunnel_id1
	bit<32> local_metadata__tunnel_tun_type3
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_src_addr
	bit<32> MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_dst_addr
}
metadata instanceof local_metadata_t

action NoAction args none {
	return
}

action tunnel_decap_decap_outer_ipv4_0 args instanceof tunnel_decap_decap_outer_ipv4_0_arg_t {
	mov m.local_metadata__tunnel_id1 t.tunnel_id
	return
}

action tunnel_encap_set_tunnel_0 args instanceof tunnel_encap_set_tunnel_0_arg_t {
	mov m.local_metadata__outer_ipv4_dst0 t.dst_addr
	return
}

table tunnel_decap_ipv4_tunnel_term_table {
	key {
		m.MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_src_addr exact
		m.MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_dst_addr exact
		m.local_metadata__tunnel_tun_type3 exact
	}
	actions {
		tunnel_decap_decap_outer_ipv4_0
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table tunnel_encap_set_tunnel_encap {
	key {
		m.pna_main_input_metadata_input_port exact
	}
	actions {
		tunnel_encap_set_tunnel_0
		NoAction
	}
	default_action NoAction args none 
	size 0x100
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.outer_ethernet
	jmpeq PACKET_PARSER_PARSE_IPV4_OTR h.outer_ethernet.ether_type 0x800
	jmp PACKET_PARSER_ACCEPT
	PACKET_PARSER_PARSE_IPV4_OTR :	extract h.outer_ipv4
	PACKET_PARSER_ACCEPT :	jmpneq LABEL_FALSE m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_src_addr h.outer_ipv4.src_addr
	mov m.MainControlT_tunnel_decap_ipv4_tunnel_term_table_outer_ipv4_dst_addr h.outer_ipv4.dst_addr
	table tunnel_decap_ipv4_tunnel_term_table
	jmp LABEL_END
	LABEL_FALSE :	table tunnel_encap_set_tunnel_encap
	LABEL_END :	emit h.outer_ethernet
	emit h.outer_ipv4
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


