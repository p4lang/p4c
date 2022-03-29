
struct my_header_t {
	bit<16> type1
	bit<8> type2
	bit<32> value
}

struct lookahead_tmp_h_0 {
	bit<24> f
}

struct lookahead_tmp_h_1 {
	bit<24> f
}

struct main_metadata_t {
	bit<32> pna_main_input__2
	bit<16> local_metadata__3
	bit<8> local_metadata__4
	bit<32> pna_main_output_5
	bit<24> MainParserT_par_6
	bit<24> MainParserT_par_7
	bit<8> MainParserT_par_8
	bit<16> MainParserT_par_9
	bit<24> MainParserT_par_10
	bit<24> MainParserT_par_11
}
metadata instanceof main_metadata_t

header h1 instanceof my_header_t
header h2 instanceof my_header_t
header MainParserT_par_12 instanceof lookahead_tmp_h_0
header MainParserT_par_13 instanceof lookahead_tmp_h_1

apply {
	rx m.pna_main_input__2
	lookahead h.MainParserT_par_13
	mov m.MainParserT_par_10 h.MainParserT_par_13.f
	mov m.MainParserT_par_7 m.MainParserT_par_10
	shr m.MainParserT_par_7 0x8
	mov m.MainParserT_par_9 m.MainParserT_par_7
	jmpeq MAINPARSERIMPL_PARSE_H1 m.MainParserT_par_9 0x1234
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	lookahead h.MainParserT_par_12
	mov m.MainParserT_par_11 h.MainParserT_par_12.f
	mov m.MainParserT_par_6 m.MainParserT_par_11
	shr m.MainParserT_par_6 0x8
	mov m.local_metadata__3 m.MainParserT_par_6
	mov m.local_metadata__4 m.MainParserT_par_11
	mov m.MainParserT_par_8 m.MainParserT_par_11
	jmpeq MAINPARSERIMPL_PARSE_H2 m.MainParserT_par_8 0x1
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	extract h.h2
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_5
}


