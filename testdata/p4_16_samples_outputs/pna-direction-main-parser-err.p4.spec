
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

struct next_hop_0_arg_t {
	bit<32> vport
}

struct main_metadata_t {
	bit<16> pna_pre_input_metadata_parser_error
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_tmpDir
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainParserT_parser_tmp
	bit<32> MainParserT_parser_tmp_0
	bit<32> MainControlT_tmpDir
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray direction size 0x100 initval 0
action next_hop_0 args instanceof next_hop_0_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action default_route_drop_0 args none {
	drop
	return
}

table ipv4_da_lpm {
	key {
		m.MainControlT_tmpDir lpm
	}
	actions {
		next_hop_0
		default_route_drop_0
	}
	default_action default_route_drop_0 args none const
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	regrd m.pna_main_input_metadata_direction direction m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpneq LABEL_FALSE 0x0 m.pna_main_input_metadata_direction
	mov m.MainParserT_parser_tmp_0 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainParserT_parser_tmp_0 0x0
	LABEL_END :	mov m.MainParserT_parser_tmp m.MainParserT_parser_tmp_0
	jmpeq MAINPARSERIMPL_ACCEPT m.MainParserT_parser_tmp 0x1
	jmpeq MAINPARSERIMPL_ACCEPT m.MainParserT_parser_tmp 0x0
	jmp MAINPARSERIMPL_NOMATCH
	jmp MAINPARSERIMPL_ACCEPT
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_NOMATCH :	mov m.pna_pre_input_metadata_parser_error 0x2
	MAINPARSERIMPL_ACCEPT :	jmpneq LABEL_FALSE_0 0x0 m.pna_main_input_metadata_direction
	mov m.local_metadata_tmpDir h.ipv4.srcAddr
	jmp LABEL_END_1
	LABEL_FALSE_0 :	mov m.local_metadata_tmpDir h.ipv4.dstAddr
	LABEL_END_1 :	jmpneq LABEL_FALSE_1 0x0 m.pna_main_input_metadata_direction
	mov m.MainControlT_tmpDir h.ipv4.srcAddr
	jmp LABEL_END_2
	LABEL_FALSE_1 :	mov m.MainControlT_tmpDir h.ipv4.dstAddr
	LABEL_END_2 :	jmpnv LABEL_END_3 h.ipv4
	table ipv4_da_lpm
	LABEL_END_3 :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


