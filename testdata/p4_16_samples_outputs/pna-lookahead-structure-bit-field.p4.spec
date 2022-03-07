
struct header1_t {
	bit<8> type1
	bit<8> type2
}

struct header2_t {
	bit<8> type1
	bit<16> type2
}

struct lookahead_tmp_hdr {
	bit<8> f
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<8> local_metadata_f1
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header h1 instanceof header1_t
header h2 instanceof header2_t
header MainParserT_parser_lookahead_tmp instanceof lookahead_tmp_hdr

apply {
	rx m.pna_main_input_metadata_input_port
	lookahead h.MainParserT_parser_lookahead_tmp
	mov m.local_metadata_f1 h.MainParserT_parser_lookahead_tmp.f
	jmpeq MAINPARSERIMPL_PARSE_H1 m.local_metadata_f1 0x1
	jmpeq MAINPARSERIMPL_PARSE_H2 m.local_metadata_f1 0x2
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	extract h.h2
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_metadata_output_port
}


