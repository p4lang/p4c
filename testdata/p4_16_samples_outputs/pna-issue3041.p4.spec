
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_base_t {
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

struct ipv4_option_timestamp_t {
	bit<8> value
	bit<8> len
	varbit<304> data
}

struct option_t {
	bit<8> type
	bit<8> len
}

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainParserT_parser_tmp
	bit<32> MainParserT_parser_tmp_0
	bit<32> MainParserT_parser_tmp_1
	bit<32> MainParserT_parser_tmp_2
	bit<32> MainParserT_parser_tmp_3
	bit<32> MainParserT_parser_tmp_3_extract_tmp
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4_base instanceof ipv4_base_t
header ipv4_option_timestamp instanceof ipv4_option_timestamp_t
header option instanceof option_t

regarray direction size 0x100 initval 0

action NoAction args none {
	return
}

action a1 args instanceof a1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl2 {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4_base
	mov m.MainParserT_parser_tmp h.ipv4_base.version_ihl
	shr m.MainParserT_parser_tmp 0x4
	mov m.MainParserT_parser_tmp_2 m.MainParserT_parser_tmp
	jmpeq MAINPARSERIMPL_ACCEPT m.MainParserT_parser_tmp_2 0x5
	lookahead h.option
	jmpeq MAINPARSERIMPL_PARSE_IPV4_OPTION_TIMESTAMP h.option.type 0x44
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_OPTION_TIMESTAMP :	mov m.MainParserT_parser_tmp_0 h.option.len
	mov m.MainParserT_parser_tmp_1 m.MainParserT_parser_tmp_0
	shl m.MainParserT_parser_tmp_1 0x3
	mov m.MainParserT_parser_tmp_3 m.MainParserT_parser_tmp_1
	add m.MainParserT_parser_tmp_3 0xfffffff0
	mov m.MainParserT_parser_tmp_3_extract_tmp m.MainParserT_parser_tmp_3
	shr m.MainParserT_parser_tmp_3_extract_tmp 0x3
	extract h.ipv4_option_timestamp m.MainParserT_parser_tmp_3_extract_tmp
	MAINPARSERIMPL_ACCEPT :	mov m.pna_main_output_metadata_output_port 0x0
	table tbl
	table tbl2
	emit h.ethernet
	emit h.ipv4_base
	tx m.pna_main_output_metadata_output_port
}


