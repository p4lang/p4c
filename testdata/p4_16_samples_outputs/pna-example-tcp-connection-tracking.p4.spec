
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLength
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct tcp_t {
	bit<16> srcPort
	bit<16> dstPort
	bit<32> seqNo
	bit<32> ackNo
	bit<8> dataOffset_res
	bit<8> flags
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControlImpl_ct_tcp_table_ipv4_protocol
	bit<8> MainControlT_do_add_on_miss
	bit<8> MainControlT_update_aging_info
	bit<8> MainControlT_update_expire_time
	bit<8> MainControlT_new_expire_time_profile_id
	bit<32> MainControlT_key
	bit<32> MainControlT_key_0
	bit<16> MainControlT_key_1
	bit<16> MainControlT_key_2
	bit<32> reg_read_tmp
	bit<32> left_shift_tmp
}
metadata instanceof metadata_t

header eth instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

regarray network_port_mask size 0x1 initval 0

action tcp_syn_packet args none {
	mov m.MainControlT_do_add_on_miss 1
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x1
	return
}

action tcp_fin_or_rst_packet args none {
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x0
	return
}

action tcp_other_packets args none {
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x2
	return
}

action ct_tcp_table_hit args none {
	jmpneq LABEL_END_5 m.MainControlT_update_aging_info 0x1
	jmpneq LABEL_FALSE_2 m.MainControlT_update_expire_time 0x1
	rearm m.MainControlT_new_expire_time_profile_id
	jmp LABEL_END_5
	LABEL_FALSE_2 :	rearm
	LABEL_END_5 :	return
}

action ct_tcp_table_miss args none {
	jmpneq LABEL_FALSE_3 m.MainControlT_do_add_on_miss 0x1
	learn ct_tcp_table_hit m.MainControlT_new_expire_time_profile_id
	jmp LABEL_END_7
	LABEL_FALSE_3 :	drop
	LABEL_END_7 :	return
}

table set_ct_options {
	key {
		h.tcp.flags wildcard
	}
	actions {
		tcp_syn_packet
		tcp_fin_or_rst_packet
		tcp_other_packets
	}
	default_action tcp_other_packets args none const
	size 0x10000
}


learner ct_tcp_table {
	key {
		m.MainControlT_key
		m.MainControlT_key_0
		m.MainControlImpl_ct_tcp_table_ipv4_protocol
		m.MainControlT_key_1
		m.MainControlT_key_2
	}
	actions {
		ct_tcp_table_hit @tableonly
		ct_tcp_table_miss @defaultonly
	}
	default_action ct_tcp_table_miss args none 
	size 65536
	timeout {
		60
		120
		180
		}
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.eth
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.eth.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_TCP :	extract h.tcp
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_do_add_on_miss 0
	mov m.MainControlT_update_expire_time 0
	regrd m.reg_read_tmp network_port_mask 0x0
	mov m.left_shift_tmp 0x1
	shl m.left_shift_tmp m.pna_main_input_metadata_input_port
	mov m.pna_main_input_metadata_direction m.reg_read_tmp
	and m.pna_main_input_metadata_direction m.left_shift_tmp
	jmpneq LABEL_END m.pna_main_input_metadata_direction 0x1
	jmpnv LABEL_END h.ipv4
	jmpnv LABEL_END h.tcp
	table set_ct_options
	LABEL_END :	jmpnv LABEL_END_0 h.ipv4
	jmpnv LABEL_END_0 h.tcp
	regrd m.reg_read_tmp network_port_mask 0x0
	mov m.left_shift_tmp 0x1
	shl m.left_shift_tmp m.pna_main_input_metadata_input_port
	mov m.pna_main_input_metadata_direction m.reg_read_tmp
	and m.pna_main_input_metadata_direction m.left_shift_tmp
	jmpeq LABEL_TRUE_1 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key h.ipv4.dstAddr
	jmp LABEL_END_1
	LABEL_TRUE_1 :	mov m.MainControlT_key h.ipv4.srcAddr
	LABEL_END_1 :	regrd m.reg_read_tmp network_port_mask 0x0
	mov m.left_shift_tmp 0x1
	shl m.left_shift_tmp m.pna_main_input_metadata_input_port
	mov m.pna_main_input_metadata_direction m.reg_read_tmp
	and m.pna_main_input_metadata_direction m.left_shift_tmp
	jmpeq LABEL_TRUE_2 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_0 h.ipv4.srcAddr
	jmp LABEL_END_2
	LABEL_TRUE_2 :	mov m.MainControlT_key_0 h.ipv4.dstAddr
	LABEL_END_2 :	regrd m.reg_read_tmp network_port_mask 0x0
	mov m.left_shift_tmp 0x1
	shl m.left_shift_tmp m.pna_main_input_metadata_input_port
	mov m.pna_main_input_metadata_direction m.reg_read_tmp
	and m.pna_main_input_metadata_direction m.left_shift_tmp
	jmpeq LABEL_TRUE_3 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_1 h.tcp.dstPort
	jmp LABEL_END_3
	LABEL_TRUE_3 :	mov m.MainControlT_key_1 h.tcp.srcPort
	LABEL_END_3 :	regrd m.reg_read_tmp network_port_mask 0x0
	mov m.left_shift_tmp 0x1
	shl m.left_shift_tmp m.pna_main_input_metadata_input_port
	mov m.pna_main_input_metadata_direction m.reg_read_tmp
	and m.pna_main_input_metadata_direction m.left_shift_tmp
	jmpeq LABEL_TRUE_4 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_2 h.tcp.srcPort
	jmp LABEL_END_4
	LABEL_TRUE_4 :	mov m.MainControlT_key_2 h.tcp.dstPort
	LABEL_END_4 :	mov m.MainControlImpl_ct_tcp_table_ipv4_protocol h.ipv4.protocol
	table ct_tcp_table
	LABEL_END_0 :	emit h.eth
	tx m.pna_main_output_metadata_output_port
}


