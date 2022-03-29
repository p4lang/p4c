
struct header1_t {
	bit<8> type1
	bit<8> type2
}

struct header2_t {
	bit<8> type1
	bit<16> type2
}

struct lookahead_tmp_h_0 {
	bit<8> f
}

struct main_metadata_t {
	bit<32> pna_main_input__1
	bit<8> local_metadata__2
	bit<32> pna_main_output_3
}
metadata instanceof main_metadata_t

header h1 instanceof header1_t
header h2 instanceof header2_t
header MainParserT_par_4 instanceof lookahead_tmp_h_0

apply {
	rx m.pna_main_input__1
	lookahead h.MainParserT_par_4
	mov m.local_metadata__2 h.MainParserT_par_4.f
	jmpeq MAINPARSERIMPL_PARSE_H1 m.local_metadata__2 0x1
	jmpeq MAINPARSERIMPL_PARSE_H2 m.local_metadata__2 0x2
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	extract h.h2
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_3
}


