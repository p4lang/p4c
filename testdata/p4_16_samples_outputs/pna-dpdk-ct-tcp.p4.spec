
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
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

struct send_arg_t {
	bit<32> port
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

struct main_metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<8> local_metadata__dir0
	bit<8> local_metadata__currentState1
	bit<8> local_metadata__tcpFlagsSet2
	bit<32> local_metadata__hashCode3
	bit<32> local_metadata__replyHashCode4
	bit<32> local_metadata__connHashCode5
	bit<8> local_metadata__doAddOnMiss6
	bit<8> local_metadata__updateExpireTimeoutProfile7
	bit<8> local_metadata__restartExpireTimer8
	bit<8> local_metadata__acceptPacket9
	bit<8> local_metadata__timeoutProfileId10
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControlImpl_tcp_ct_state_table_retval
	bit<8> MainControlImpl_tcp_ct_state_table_retval_0
	;oldname:MainControlImpl_tcp_ct_state_table_local_metadata__currentState1
	bit<8> MainControlImpl_tcp_ct_state_table_local_metadata__currentS0
	bit<32> MainControlImpl_tcp_ct_table_key
	bit<32> MainControlImpl_tcp_ct_table_key_0
	bit<8> MainControlImpl_tcp_ct_table_ipv4_protocol
	bit<16> MainControlImpl_tcp_ct_table_key_1
	bit<16> MainControlImpl_tcp_ct_table_key_2
	bit<8> MainControlT_tmp
	bit<8> MainControlT_tmp_0
	bit<8> MainControlT_tmp_1
	bit<8> MainControlT_tmp_2
	bit<8> MainControlT_tmp_3
	bit<8> MainControlT_tmp_4
	bit<32> MainControlT_tmp_5
	bit<32> MainControlT_tmp_6
	bit<16> MainControlT_tmp_7
	bit<16> MainControlT_tmp_8
	bit<8> MainControlT_tmp_9
	bit<32> MainControlT_tmp_10
	bit<32> MainControlT_tmp_11
	bit<16> MainControlT_tmp_12
	bit<16> MainControlT_tmp_13
	bit<32> MainControlT_tmp_14
	bit<32> MainControlT_tmp_15
	bit<8> MainControlT_tmp_16
	bit<16> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<32> MainControlT_key
	bit<32> MainControlT_key_0
	bit<16> MainControlT_key_1
	bit<16> MainControlT_key_2
	bit<32> MainControlT_tmp_19
	bit<32> MainControlT_tmp_20
	bit<16> MainControlT_tmp_21
	bit<16> MainControlT_tmp_22
	bit<8> MainControlT_hasReturned
	bit<8> MainControlT_retval
	bit<8> MainControlT_tmp_23
	bit<8> MainControlT_hasReturned_0
	bit<8> MainControlT_retval_0
}
metadata instanceof main_metadata_t

regarray tcpCtCurrentStateReg size 0x10000 initval 0

regarray tcpCtDirReg size 0x10000 initval 0

regarray direction size 0x100 initval 0

action drop args none {
	drop
	return
}

action drop_1 args none {
	drop
	return
}

action tcp_ct_syn_sent args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x1
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x3
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 1
	return
}

action tcp_ct_syn_recv args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x2
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x2
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_established args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x3
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x4
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_fin_wait args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x4
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x3
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_close_wait args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x5
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x2
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_last_ack args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x6
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x1
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_time_wait args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x7
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x3
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_close args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x8
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x0
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_syn_sent2 args none {
	regwr tcpCtCurrentStateReg m.local_metadata__connHashCode5 0x9
	mov m.local_metadata__updateExpireTimeoutProfile7 1
	mov m.local_metadata__timeoutProfileId10 0x3
	mov m.local_metadata__restartExpireTimer8 1
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_ignore args none {
	mov m.local_metadata__updateExpireTimeoutProfile7 0
	mov m.local_metadata__restartExpireTimer8 0
	mov m.local_metadata__doAddOnMiss6 0
	return
}

action tcp_ct_table_hit args none {
	mov m.local_metadata__acceptPacket9 1
	return
}

action tcp_ct_table_miss args none {
	jmpneq LABEL_FALSE_12 m.local_metadata__doAddOnMiss6 0x1
	mov m.local_metadata__acceptPacket9 1
	jmp LABEL_END_20
	LABEL_FALSE_12 :	mov m.local_metadata__acceptPacket9 0
	LABEL_END_20 :	return
}

action send args instanceof send_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

table tcp_ct_state_table {
	key {
		m.MainControlImpl_tcp_ct_state_table_retval exact
		m.MainControlImpl_tcp_ct_state_table_retval_0 exact
		m.MainControlImpl_tcp_ct_state_table_local_metadata__currentS0 exact
	}
	actions {
		drop
		tcp_ct_syn_sent
		tcp_ct_syn_recv
		tcp_ct_established
		tcp_ct_fin_wait
		tcp_ct_close_wait
		tcp_ct_last_ack
		tcp_ct_time_wait
		tcp_ct_close
		tcp_ct_syn_sent2
		tcp_ct_ignore
	}
	default_action drop args none const
	size 0x10000
}


table ip_packet_fwd {
	key {
		h.ipv4.dstAddr lpm
	}
	actions {
		drop_1
		send
	}
	default_action drop_1 args none const
	size 0x1000
}


learner tcp_ct_table {
	key {
		m.MainControlImpl_tcp_ct_table_key
		m.MainControlImpl_tcp_ct_table_key_0
		m.MainControlImpl_tcp_ct_table_ipv4_protocol
		m.MainControlImpl_tcp_ct_table_key_1
		m.MainControlImpl_tcp_ct_table_key_2
	}
	actions {
		tcp_ct_table_hit @tableonly
		tcp_ct_table_miss @defaultonly
	}
	default_action tcp_ct_table_miss args none 
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
	regrd m.pna_main_input_metadata_direction direction m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ethernet.etherType 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_TCP :	extract h.tcp
	MAINPARSERIMPL_ACCEPT :	mov m.local_metadata__doAddOnMiss6 0
	mov m.local_metadata__restartExpireTimer8 0
	mov m.local_metadata__updateExpireTimeoutProfile7 0
	mov m.local_metadata__acceptPacket9 0
	jmpnv LABEL_END h.tcp
	mov m.MainControlT_tmp_4 h.ipv4.protocol
	mov m.MainControlT_tmp_5 h.ipv4.srcAddr
	mov m.MainControlT_tmp_6 h.ipv4.dstAddr
	mov m.MainControlT_tmp_7 h.tcp.srcPort
	mov m.MainControlT_tmp_8 h.tcp.dstPort
	hash jhash m.local_metadata__hashCode3  m.MainControlT_tmp_4 m.MainControlT_tmp_8
	mov m.MainControlT_tmp_9 h.ipv4.protocol
	mov m.MainControlT_tmp_10 h.ipv4.dstAddr
	mov m.MainControlT_tmp_11 h.ipv4.srcAddr
	mov m.MainControlT_tmp_12 h.tcp.dstPort
	mov m.MainControlT_tmp_13 h.tcp.srcPort
	hash jhash m.local_metadata__replyHashCode4  m.MainControlT_tmp_9 m.MainControlT_tmp_13
	jmpeq LABEL_TRUE_0 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_tmp_19 h.ipv4.dstAddr
	jmp LABEL_END_0
	LABEL_TRUE_0 :	mov m.MainControlT_tmp_19 h.ipv4.srcAddr
	LABEL_END_0 :	jmpeq LABEL_TRUE_1 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_tmp_20 h.ipv4.srcAddr
	jmp LABEL_END_1
	LABEL_TRUE_1 :	mov m.MainControlT_tmp_20 h.ipv4.dstAddr
	LABEL_END_1 :	jmpeq LABEL_TRUE_2 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_tmp_21 h.tcp.dstPort
	jmp LABEL_END_2
	LABEL_TRUE_2 :	mov m.MainControlT_tmp_21 h.tcp.srcPort
	LABEL_END_2 :	jmpeq LABEL_TRUE_3 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_tmp_22 h.tcp.srcPort
	jmp LABEL_END_3
	LABEL_TRUE_3 :	mov m.MainControlT_tmp_22 h.tcp.dstPort
	LABEL_END_3 :	mov m.MainControlT_tmp_14 m.MainControlT_tmp_19
	mov m.MainControlT_tmp_15 m.MainControlT_tmp_20
	mov m.MainControlT_tmp_16 h.ipv4.protocol
	mov m.MainControlT_tmp_17 m.MainControlT_tmp_21
	mov m.MainControlT_tmp_18 m.MainControlT_tmp_22
	hash jhash m.local_metadata__connHashCode5  m.MainControlT_tmp_14 m.MainControlT_tmp_18
	mov m.MainControlT_hasReturned 0
	regrd m.MainControlT_tmp_23 tcpCtDirReg m.local_metadata__replyHashCode4
	jmpneq LABEL_END_4 m.MainControlT_tmp_23 0x0
	mov m.MainControlT_hasReturned 1
	mov m.MainControlT_retval 0x1
	LABEL_END_4 :	jmpneq LABEL_FALSE_1 m.MainControlT_hasReturned 0x1
	jmp LABEL_END_5
	LABEL_FALSE_1 :	mov m.MainControlT_retval 0x0
	LABEL_END_5 :	mov m.local_metadata__dir0 m.MainControlT_retval
	regrd m.local_metadata__currentState1 tcpCtCurrentStateReg m.local_metadata__connHashCode5
	mov m.MainControlT_hasReturned_0 0
	mov m.MainControlT_tmp h.tcp.flags
	and m.MainControlT_tmp 0x4
	jmpeq LABEL_END_6 m.MainControlT_tmp 0x0
	mov m.MainControlT_hasReturned_0 1
	mov m.MainControlT_retval_0 0x5
	LABEL_END_6 :	jmpneq LABEL_FALSE_3 m.MainControlT_hasReturned_0 0x1
	jmp LABEL_END_7
	LABEL_FALSE_3 :	mov m.MainControlT_tmp_1 h.tcp.flags
	and m.MainControlT_tmp_1 0x2
	jmpeq LABEL_END_7 m.MainControlT_tmp_1 0x0
	mov m.MainControlT_tmp_0 h.tcp.flags
	and m.MainControlT_tmp_0 0x10
	jmpeq LABEL_FALSE_5 m.MainControlT_tmp_0 0x0
	mov m.MainControlT_hasReturned_0 1
	mov m.MainControlT_retval_0 0x2
	jmp LABEL_END_7
	LABEL_FALSE_5 :	mov m.MainControlT_hasReturned_0 1
	mov m.MainControlT_retval_0 0x1
	LABEL_END_7 :	jmpneq LABEL_FALSE_6 m.MainControlT_hasReturned_0 0x1
	jmp LABEL_END_10
	LABEL_FALSE_6 :	mov m.MainControlT_tmp_2 h.tcp.flags
	and m.MainControlT_tmp_2 0x1
	jmpeq LABEL_END_10 m.MainControlT_tmp_2 0x0
	mov m.MainControlT_hasReturned_0 1
	mov m.MainControlT_retval_0 0x3
	LABEL_END_10 :	jmpneq LABEL_FALSE_8 m.MainControlT_hasReturned_0 0x1
	jmp LABEL_END_12
	LABEL_FALSE_8 :	mov m.MainControlT_tmp_3 h.tcp.flags
	and m.MainControlT_tmp_3 0x10
	jmpeq LABEL_END_12 m.MainControlT_tmp_3 0x0
	mov m.MainControlT_hasReturned_0 1
	mov m.MainControlT_retval_0 0x4
	LABEL_END_12 :	jmpneq LABEL_FALSE_10 m.MainControlT_hasReturned_0 0x1
	jmp LABEL_END_14
	LABEL_FALSE_10 :	mov m.MainControlT_retval_0 0x6
	LABEL_END_14 :	mov m.local_metadata__tcpFlagsSet2 m.MainControlT_retval_0
	mov m.MainControlImpl_tcp_ct_state_table_retval m.MainControlT_retval
	mov m.MainControlImpl_tcp_ct_state_table_retval_0 m.MainControlT_retval_0
	mov m.MainControlImpl_tcp_ct_state_table_local_metadata__currentS0 m.local_metadata__currentState1
	table tcp_ct_state_table
	jmpeq LABEL_TRUE_15 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key h.ipv4.dstAddr
	jmp LABEL_END_15
	LABEL_TRUE_15 :	mov m.MainControlT_key h.ipv4.srcAddr
	LABEL_END_15 :	jmpeq LABEL_TRUE_16 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_0 h.ipv4.srcAddr
	jmp LABEL_END_16
	LABEL_TRUE_16 :	mov m.MainControlT_key_0 h.ipv4.dstAddr
	LABEL_END_16 :	jmpeq LABEL_TRUE_17 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_1 h.tcp.dstPort
	jmp LABEL_END_17
	LABEL_TRUE_17 :	mov m.MainControlT_key_1 h.tcp.srcPort
	LABEL_END_17 :	jmpeq LABEL_TRUE_18 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_2 h.tcp.srcPort
	jmp LABEL_END_18
	LABEL_TRUE_18 :	mov m.MainControlT_key_2 h.tcp.dstPort
	LABEL_END_18 :	mov m.MainControlImpl_tcp_ct_table_key m.MainControlT_key
	mov m.MainControlImpl_tcp_ct_table_key_0 m.MainControlT_key_0
	mov m.MainControlImpl_tcp_ct_table_ipv4_protocol h.ipv4.protocol
	mov m.MainControlImpl_tcp_ct_table_key_1 m.MainControlT_key_1
	mov m.MainControlImpl_tcp_ct_table_key_2 m.MainControlT_key_2
	table tcp_ct_table
	LABEL_END :	jmpneq LABEL_END_19 m.local_metadata__acceptPacket9 0x1
	table ip_packet_fwd
	LABEL_END_19 :	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.pna_main_output_metadata_output_port
}


