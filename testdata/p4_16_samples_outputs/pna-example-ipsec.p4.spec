
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> dscp_ecn
	bit<16> total_length
	bit<16> identification
	bit<16> flags_fragment_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> header_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct esp_t {
	bit<32> spi
	bit<32> seq
}

struct platform_hdr_t {
	bit<32> sa_id
}

struct ipsec_enable_1_arg_t {
	bit<32> sa_index
}

struct ipsec_enable_arg_t {
	bit<32> sa_index
}

struct next_hop_id_set_arg_t {
	bit<32> next_hop_id
}

struct next_hop_set_arg_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
	bit<32> port_id
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header esp instanceof esp_t
header ipsec_hdr instanceof platform_hdr_t

struct metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_next_hop_id
	bit<8> local_metadata_bypass
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlImpl_inbound_table_ipv4_src_addr
	bit<32> MainControlImpl_inbound_table_ipv4_dst_addr
	bit<32> MainControlImpl_inbound_table_esp_spi
	bit<8> MainParserT_parser_from_ipsec
	bit<32> MainParserT_parser_status
	bit<32> MainParserT_parser_tmp_0
	bit<32> MainControlT_status
	bit<32> MainControlT_status_0
	bit<8> MainControlT_tmp
	bit<8> MainControlT_tmp_0
	bit<32> ipsec_port_inbound
	bit<32> ipsec_port_outbound
	bit<32> ipsec_port_inbound_0
	bit<32> ipsec_port_outbound_0
	bit<32> ipsec_port_inbound_1
	bit<32> ipsec_port_outbound_1
}
metadata instanceof metadata_t

regarray ipsec_port_out_inbound size 0x1 initval 0
regarray ipsec_port_out_outbound size 0x1 initval 0
regarray ipsec_port_in_inbound size 0x1 initval 0
regarray ipsec_port_in_outbound size 0x1 initval 0
regarray direction size 0x100 initval 0
action ipsec_enable args instanceof ipsec_enable_arg_t {
	validate h.ipsec_hdr
	mov h.ipsec_hdr.sa_id t.sa_index
	invalidate h.ethernet
	return
}

action ipsec_enable_1 args instanceof ipsec_enable_1_arg_t {
	validate h.ipsec_hdr
	mov h.ipsec_hdr.sa_id t.sa_index
	invalidate h.ethernet
	return
}

action ipsec_bypass args none {
	mov m.local_metadata_bypass 1
	invalidate h.ipsec_hdr
	return
}

action ipsec_bypass_1 args none {
	mov m.local_metadata_bypass 1
	invalidate h.ipsec_hdr
	return
}

action drop args none {
	drop
	return
}

action drop_1 args none {
	drop
	return
}

action drop_2 args none {
	drop
	return
}

action drop_3 args none {
	drop
	return
}

action next_hop_id_set args instanceof next_hop_id_set_arg_t {
	mov m.local_metadata_next_hop_id t.next_hop_id
	return
}

action next_hop_set args instanceof next_hop_set_arg_t {
	validate h.ethernet
	mov h.ethernet.dst_addr t.dst_addr
	mov h.ethernet.src_addr t.src_addr
	mov h.ethernet.ether_type t.ether_type
	mov m.pna_main_output_metadata_output_port t.port_id
	return
}

table inbound_table {
	key {
		m.MainControlImpl_inbound_table_ipv4_src_addr exact
		m.MainControlImpl_inbound_table_ipv4_dst_addr exact
		m.MainControlImpl_inbound_table_esp_spi exact
	}
	actions {
		ipsec_enable
		ipsec_bypass
		drop
	}
	default_action drop args none const
	size 0x10000
}


table outbound_table {
	key {
		h.ipv4.src_addr exact
		h.ipv4.dst_addr exact
	}
	actions {
		ipsec_enable_1
		ipsec_bypass_1
		drop_1
	}
	default_action ipsec_bypass_1 args none 
	size 0x10000
}


table routing_table {
	key {
		h.ipv4.dst_addr lpm
	}
	actions {
		next_hop_id_set
		drop_2
	}
	default_action next_hop_id_set args next_hop_id 0x0 
	size 0x10000
}


table next_hop_table {
	key {
		m.local_metadata_next_hop_id exact
	}
	actions {
		next_hop_set
		drop_3
	}
	default_action drop_3 args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	regrd m.pna_main_input_metadata_direction direction m.pna_main_input_metadata_input_port
	mov m.MainParserT_parser_status 0x0
	regrd m.ipsec_port_inbound ipsec_port_in_inbound 0x0
	regrd m.ipsec_port_outbound ipsec_port_in_outbound 0x0
	mov m.MainParserT_parser_from_ipsec 0x0
	jmpeq LABEL_TRUE m.pna_main_input_metadata_input_port m.ipsec_port_inbound
	jmpeq LABEL_TRUE m.pna_main_input_metadata_input_port m.ipsec_port_outbound
	jmp LABEL_END
	LABEL_TRUE :	mov m.MainParserT_parser_from_ipsec 0x1
	LABEL_END :	jmpneq LABEL_FALSE m.MainParserT_parser_from_ipsec 0x1
	mov m.MainParserT_parser_tmp_0 0x1
	jmp LABEL_END_0
	LABEL_FALSE :	mov m.MainParserT_parser_tmp_0 0x0
	LABEL_END_0 :	jmpeq MAINPARSERIMPL_PARSE_IPV4 m.MainParserT_parser_tmp_0 0x1
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.ether_type 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq MAINPARSERIMPL_PARSE_ESP h.ipv4.protocol 0x32
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_ESP :	extract h.esp
	MAINPARSERIMPL_ACCEPT :	jmpneq LABEL_FALSE_0 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_status 0x0
	regrd m.ipsec_port_inbound_0 ipsec_port_in_inbound 0x0
	regrd m.ipsec_port_outbound_0 ipsec_port_in_outbound 0x0
	mov m.MainControlT_tmp 0x0
	jmpeq LABEL_TRUE_2 m.pna_main_input_metadata_input_port m.ipsec_port_inbound_0
	jmpeq LABEL_TRUE_2 m.pna_main_input_metadata_input_port m.ipsec_port_outbound_0
	jmp LABEL_END_2
	LABEL_TRUE_2 :	mov m.MainControlT_tmp 0x1
	LABEL_END_2 :	jmpneq LABEL_FALSE_1 m.MainControlT_tmp 0x1
	jmpneq LABEL_FALSE_2 m.MainControlT_status 0x0
	table routing_table
	table next_hop_table
	jmp LABEL_END_1
	LABEL_FALSE_2 :	drop
	jmp LABEL_END_1
	LABEL_FALSE_1 :	jmpnv LABEL_FALSE_3 h.ipv4
	jmpnv LABEL_FALSE_4 h.esp
	mov m.MainControlImpl_inbound_table_ipv4_src_addr h.ipv4.src_addr
	mov m.MainControlImpl_inbound_table_ipv4_dst_addr h.ipv4.dst_addr
	mov m.MainControlImpl_inbound_table_esp_spi h.esp.spi
	table inbound_table
	jmpneq LABEL_END_1 m.local_metadata_bypass 0x1
	table routing_table
	table next_hop_table
	jmp LABEL_END_1
	jmp LABEL_END_1
	LABEL_FALSE_4 :	table routing_table
	table next_hop_table
	jmp LABEL_END_1
	LABEL_FALSE_3 :	drop
	jmp LABEL_END_1
	LABEL_FALSE_0 :	mov m.MainControlT_status_0 0x0
	regrd m.ipsec_port_inbound_1 ipsec_port_in_inbound 0x0
	regrd m.ipsec_port_outbound_1 ipsec_port_in_outbound 0x0
	mov m.MainControlT_tmp_0 0x0
	jmpeq LABEL_TRUE_8 m.pna_main_input_metadata_input_port m.ipsec_port_inbound_1
	jmpeq LABEL_TRUE_8 m.pna_main_input_metadata_input_port m.ipsec_port_outbound_1
	jmp LABEL_END_8
	LABEL_TRUE_8 :	mov m.MainControlT_tmp_0 0x1
	LABEL_END_8 :	jmpneq LABEL_FALSE_6 m.MainControlT_tmp_0 0x1
	jmpneq LABEL_FALSE_7 m.MainControlT_status_0 0x0
	table routing_table
	table next_hop_table
	jmp LABEL_END_1
	LABEL_FALSE_7 :	drop
	jmp LABEL_END_1
	LABEL_FALSE_6 :	jmpnv LABEL_FALSE_8 h.ipv4
	table outbound_table
	jmp LABEL_END_1
	LABEL_FALSE_8 :	drop
	LABEL_END_1 :	jmpnv LABEL_END_12 h.ipsec_hdr
	jmpneq LABEL_FALSE_10 m.pna_main_input_metadata_direction 0x0
	regrd m.pna_main_output_metadata_output_port ipsec_port_out_inbound 0x0
	jmp LABEL_END_12
	LABEL_FALSE_10 :	regrd m.pna_main_output_metadata_output_port ipsec_port_out_outbound 0x0
	LABEL_END_12 :	emit h.ipsec_hdr
	emit h.ethernet
	emit h.ipv4
	emit h.esp
	tx m.pna_main_output_metadata_output_port
}


