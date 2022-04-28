
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

struct udp_t {
	bit<16> src_port
	bit<16> dst_port
	bit<16> length
	bit<16> checksum
}

struct main_metadata_t {
	bit<32> pna_main_parser_input_metadata_pass
	bit<32> pna_main_input_metadata_pass
	bit<32> pna_main_input_metadata_input_port
	bit<16> local_metadata_port
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainParserT_parser_tmp
	bit<32> MainControlT_tmp
	bit<8> MainControlT_tmp_0
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header udp instanceof udp_t

regarray direction size 0x100 initval 0

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	extract h.udp
	jmpeq MAINPARSERIMPL_PARSE_UDP_TRUE m.MainParserT_parser_tmp 0x1
	jmpeq MAINPARSERIMPL_ACCEPT m.MainParserT_parser_tmp 0x0
	jmp MAINPARSERIMPL_NOMATCH
	MAINPARSERIMPL_PARSE_UDP_TRUE :	recircid m.pna_main_parser_input_metadata_pass
	jmpeq LABEL_FALSE m.pna_main_parser_input_metadata_pass 0x4
	mov m.MainParserT_parser_tmp 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainParserT_parser_tmp 0x0
	LABEL_END :	mov m.local_metadata_port h.udp.src_port
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_NOMATCH :	mov m.pna_pre_input_metadata_parser_error 0x2
	MAINPARSERIMPL_ACCEPT :	recircid m.pna_main_input_metadata_pass
	mov m.MainControlT_tmp m.pna_main_input_metadata_pass
	mov m.MainControlT_tmp_0 m.MainControlT_tmp
	jmpgt LABEL_END_1 m.MainControlT_tmp_0 0x4
	add h.udp.src_port 0x1
	LABEL_END_1 :	emit h.ethernet
	emit h.ipv4
	emit h.udp
	tx m.pna_main_output_metadata_output_port
}


