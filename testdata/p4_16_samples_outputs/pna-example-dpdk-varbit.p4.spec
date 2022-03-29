
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
	bit<16> flags_fragOffse_0
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct ipv4_option_tim_1 {
	bit<8> value
	bit<8> len
	varbit<304> data
}

struct option_t {
	bit<8> value
	bit<8> len
}

struct lookahead_tmp_h_2 {
	bit<8> f
}

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct main_metadata_t {
	bit<32> pna_main_input__3
	bit<32> pna_main_output_4
	bit<32> MainParserT_par_5
	bit<32> MainParserT_par_6
	bit<32> MainParserT_par_7
	bit<8> MainParserT_par_8
	bit<32> MainParserT_par_9
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4_base instanceof ipv4_base_t
header ipv4_option_tim_10 instanceof ipv4_option_tim_1
header MainParserT_par_11 instanceof option_t
header MainParserT_par_12 instanceof lookahead_tmp_h_2

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
	rx m.pna_main_input__3
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4_base
	jmpeq MAINPARSERIMPL_ACCEPT h.ipv4_base.version_ihl 0x45
	lookahead h.MainParserT_par_12
	mov m.MainParserT_par_8 h.MainParserT_par_12.f
	jmpeq MAINPARSERIMPL_PARSE_IPV4_OPTION_TIMESTAMP m.MainParserT_par_8 0x44
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4_OPTION_TIMESTAMP :	lookahead h.MainParserT_par_11
	mov m.MainParserT_par_5 h.MainParserT_par_11.len
	mov m.MainParserT_par_6 m.MainParserT_par_5
	shl m.MainParserT_par_6 0x3
	mov m.MainParserT_par_7 m.MainParserT_par_6
	add m.MainParserT_par_7 0xfffffff0
	mov m.MainParserT_par_9 m.MainParserT_par_7
	shr m.MainParserT_par_9 0x3
	extract h.ipv4_option_tim_10 m.MainParserT_par_9
	MAINPARSERIMPL_ACCEPT :	mov m.pna_main_output_4 0x0
	table tbl
	table tbl2
	emit h.ethernet
	emit h.ipv4_base
	tx m.pna_main_output_4
}


