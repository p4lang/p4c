
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
	bit<16> reserved_do_not_0
	bit<8> ttl
	bit<8> protocol
	bit<16> header_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct tunnel_decap_de_1 {
	bit<24> tunnel_id
}

struct tunnel_encap_se_2 {
	bit<32> dst_addr
}

header outer_ethernet instanceof ethernet_t
header outer_ipv4 instanceof ipv4_t
header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

struct local_metadata__3 {
	bit<32> pna_main_input__4
	bit<32> pna_main_input__5
	bit<32> local_metadata__6
	bit<24> local_metadata__7
	bit<32> local_metadata__8
	bit<32> pna_main_output_9
	bit<32> MainControlT_tu_10
	bit<32> MainControlT_tu_11
}
metadata instanceof local_metadata__3

action NoAction args none {
	return
}

action tunnel_decap_de_12 args instanceof tunnel_decap_de_1 {
	mov m.local_metadata__7 t.tunnel_id
	return
}

action tunnel_encap_se_13 args instanceof tunnel_encap_se_2 {
	mov m.local_metadata__6 t.dst_addr
	return
}

table tunnel_decap_ip_14 {
	key {
		m.MainControlT_tu_10 exact
		m.MainControlT_tu_11 exact
		m.local_metadata__8 exact
	}
	actions {
		tunnel_decap_de_12
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table tunnel_encap_se_15 {
	key {
		m.pna_main_input__5 exact
	}
	actions {
		tunnel_encap_se_13
		NoAction
	}
	default_action NoAction args none 
	size 0x100
}


apply {
	rx m.pna_main_input__5
	extract h.outer_ethernet
	jmpeq PACKET_PARSER_PARSE_IPV4_OTR h.outer_ethernet.ether_type 0x800
	jmp PACKET_PARSER_ACCEPT
	PACKET_PARSER_PARSE_IPV4_OTR :	extract h.outer_ipv4
	PACKET_PARSER_ACCEPT :	jmpneq LABEL_FALSE m.pna_main_input__4 0x0
	mov m.MainControlT_tu_10 h.outer_ipv4.src_addr
	mov m.MainControlT_tu_11 h.outer_ipv4.dst_addr
	table tunnel_decap_ipv4_tunnel_term_table
	jmp LABEL_END
	LABEL_FALSE :	table tunnel_encap_set_tunnel_encap
	LABEL_END :	emit h.outer_ethernet
	emit h.outer_ipv4
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_9
}


