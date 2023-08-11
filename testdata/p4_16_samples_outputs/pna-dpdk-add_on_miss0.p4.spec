
struct ethernet_h {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_h {
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

struct tcp_h {
	bit<16> src_port
	bit<16> dst_port
	bit<32> seq_no
	bit<32> ack_no
	bit<8> data_offset_res
	bit<8> flags
	bit<16> window
	bit<16> checksum
	bit<16> urgent_ptr
}

struct udp_h {
	bit<16> src_port
	bit<16> dst_port
	bit<16> length
	bit<16> checksum
}

struct send_arg_t {
	bit<32> port
}

header ethernet instanceof ethernet_h
header ipv4 instanceof ipv4_h
header tcp instanceof tcp_h
header udp instanceof udp_h

struct metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlImpl_ct_tcp_table_ipv4_src_addr
	bit<32> MainControlImpl_ct_tcp_table_ipv4_dst_addr
	bit<8> MainControlImpl_ct_tcp_table_ipv4_protocol
	bit<16> MainControlImpl_ct_tcp_table_tcp_src_port
	bit<16> MainControlImpl_ct_tcp_table_tcp_dst_port
	bit<48> MainControlT_tmp
	bit<48> MainControlT_tmp_0
	bit<8> MainControlT_new_expire_time_profile_id
}
metadata instanceof metadata_t

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action drop args none {
	drop
	return
}

action ct_tcp_table_hit args none {
	mov m.MainControlT_tmp h.ethernet.src_addr
	and m.MainControlT_tmp 0xFFFFFFFFFF00
	mov h.ethernet.src_addr m.MainControlT_tmp
	or h.ethernet.src_addr 0xF1
	return
}

action ct_tcp_table_miss args none {
	mov m.MainControlT_tmp_0 h.ethernet.src_addr
	and m.MainControlT_tmp_0 0xFFFFFFFFFF00
	mov h.ethernet.src_addr m.MainControlT_tmp_0
	or h.ethernet.src_addr 0xA5
	learn ct_tcp_table_hit m.MainControlT_new_expire_time_profile_id
	return
}

action send args instanceof send_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

table ipv4_host {
	key {
		h.ipv4.dst_addr exact
	}
	actions {
		send
		drop
		NoAction
	}
	default_action drop args none const
	size 0x10000
}


learner ct_tcp_table {
	key {
		m.MainControlImpl_ct_tcp_table_ipv4_src_addr
		m.MainControlImpl_ct_tcp_table_ipv4_dst_addr
		m.MainControlImpl_ct_tcp_table_ipv4_protocol
		m.MainControlImpl_ct_tcp_table_tcp_src_port
		m.MainControlImpl_ct_tcp_table_tcp_dst_port
	}
	actions {
		ct_tcp_table_hit @tableonly
		ct_tcp_table_miss @defaultonly
	}
	default_action ct_tcp_table_miss args none 
	size 0x10000
	timeout {
		10
		30
		60
		120
		300
		43200
		120
		120

		}
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.ether_type 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmpeq MAINPARSERIMPL_PARSE_UDP h.ipv4.protocol 0x11
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_UDP :	extract h.udp
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_TCP :	extract h.tcp
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_new_expire_time_profile_id 0x1
	jmpnv LABEL_END h.ipv4
	jmpnv LABEL_END h.tcp
	mov m.MainControlImpl_ct_tcp_table_ipv4_src_addr h.ipv4.src_addr
	mov m.MainControlImpl_ct_tcp_table_ipv4_dst_addr h.ipv4.dst_addr
	mov m.MainControlImpl_ct_tcp_table_ipv4_protocol h.ipv4.protocol
	mov m.MainControlImpl_ct_tcp_table_tcp_src_port h.tcp.src_port
	mov m.MainControlImpl_ct_tcp_table_tcp_dst_port h.tcp.dst_port
	table ct_tcp_table
	LABEL_END :	jmpnv LABEL_END_0 h.ipv4
	table ipv4_host
	LABEL_END_0 :	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	emit h.udp
	tx m.pna_main_output_metadata_output_port
}


