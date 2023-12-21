

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

struct user_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_ipv4_hdr_truncated
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainParserT_parser_tmp
	bit<8> MainParserT_parser_tmp_0
	bit<32> MainParserT_parser_tmp_1
	bit<8> MainControlT_tmp
	bit<8> MainControlT_tmp_1
	bit<8> MainControlT_tmp_2
	bit<8> MainControlT_tmp_4
	bit<8> MainControlT_tmp_5
	bit<48> MainControlT_tmp_6
	bit<48> MainControlT_tmp_8
	bit<8> MainControlT_tmp_9
	bit<32> MainControlT_tmp_10
	bit<8> tmpMask
}
metadata instanceof user_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header MainParserT_parser_tmp_2 instanceof ipv4_t

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	mov m.local_metadata_ipv4_hdr_truncated 0x0
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	lookahead h.MainParserT_parser_tmp_2
	mov m.MainParserT_parser_tmp h.MainParserT_parser_tmp_2.version_ihl
	and m.MainParserT_parser_tmp 0xF
	mov m.MainParserT_parser_tmp_0 m.MainParserT_parser_tmp
	and m.MainParserT_parser_tmp_0 0xF
	mov m.MainParserT_parser_tmp_1 m.MainParserT_parser_tmp_0
	mov m.tmpMask m.MainParserT_parser_tmp_1
	and m.tmpMask 0xC
	jmpeq MAINPARSERIMPL_PARSE_IPV4_IHL_TOO_SMALL m.tmpMask 0x0
	jmpeq MAINPARSERIMPL_PARSE_IPV4_IHL_TOO_SMALL m.MainParserT_parser_tmp_1 0x4
	extract h.ipv4
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_IHL_TOO_SMALL :	mov m.local_metadata_ipv4_hdr_truncated 0x1
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ethernet
	mov m.MainControlT_tmp_9 0x0
	mov m.MainControlT_tmp m.MainControlT_tmp_9
	and m.MainControlT_tmp 0xFE
	mov m.MainControlT_tmp_1 m.local_metadata_ipv4_hdr_truncated
	and m.MainControlT_tmp_1 0x1
	mov m.MainControlT_tmp_9 m.MainControlT_tmp
	or m.MainControlT_tmp_9 m.MainControlT_tmp_1
	jmpnv LABEL_FALSE_0 h.ipv4
	mov m.MainControlT_tmp_10 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.MainControlT_tmp_10 0x0
	LABEL_END_0 :	mov m.MainControlT_tmp_2 m.MainControlT_tmp_9
	and m.MainControlT_tmp_2 0xFD
	mov m.MainControlT_tmp_4 m.MainControlT_tmp_10
	shl m.MainControlT_tmp_4 0x1
	mov m.MainControlT_tmp_5 m.MainControlT_tmp_4
	and m.MainControlT_tmp_5 0x2
	mov m.MainControlT_tmp_9 m.MainControlT_tmp_2
	or m.MainControlT_tmp_9 m.MainControlT_tmp_5
	mov m.MainControlT_tmp_6 h.ethernet.srcAddr
	and m.MainControlT_tmp_6 0xFFFFFFFFFF00
	mov m.MainControlT_tmp_8 m.MainControlT_tmp_9
	and m.MainControlT_tmp_8 0xFF
	mov h.ethernet.srcAddr m.MainControlT_tmp_6
	or h.ethernet.srcAddr m.MainControlT_tmp_8
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


