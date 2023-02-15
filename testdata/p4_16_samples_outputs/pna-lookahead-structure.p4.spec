
struct my_header_t {
	bit<16> type1
	bit<8> type2
	bit<32> value
}

struct lookahead_tmp_hdr {
	bit<24> f
}

struct lookahead_tmp_hdr_0 {
	bit<24> f
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<16> local_metadata__s1_type10
	bit<8> local_metadata__s1_type21
	bit<32> pna_main_output_metadata_output_port
	bit<24> MainParserT_parser_tmp
	bit<24> MainParserT_parser_tmp_0
	bit<24> MainParserT_parser_tmp_1
	bit<24> MainParserT_parser_tmp_2
	bit<24> MainParserT_parser_tmp_3
	bit<24> MainParserT_parser_tmp_4
	bit<24> MainParserT_parser_tmp_5
	bit<24> MainParserT_parser_tmp_6
	bit<24> MainParserT_parser_tmp_7
	bit<24> MainParserT_parser_tmp_8
	bit<24> MainParserT_parser_tmp_12
}
metadata instanceof main_metadata_t

header h1 instanceof my_header_t
header h2 instanceof my_header_t
;oldname:MainParserT_parser_lookahead_tmp
header MainParserT_parser_lookahead_0 instanceof lookahead_tmp_hdr
;oldname:MainParserT_parser_lookahead_tmp_0
header MainParserT_parser_lookahead_1 instanceof lookahead_tmp_hdr_0

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	lookahead h.MainParserT_parser_lookahead_1
	mov m.MainParserT_parser_tmp_6 h.MainParserT_parser_lookahead_1.f
	shr m.MainParserT_parser_tmp_6 0x8
	mov m.MainParserT_parser_tmp_7 m.MainParserT_parser_tmp_6
	and m.MainParserT_parser_tmp_7 0xFFFF
	mov m.MainParserT_parser_tmp_8 m.MainParserT_parser_tmp_7
	and m.MainParserT_parser_tmp_8 0xFFFF
	jmpeq MAINPARSERIMPL_PARSE_H1 m.MainParserT_parser_tmp_8 0x1234
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	lookahead h.MainParserT_parser_lookahead_0
	mov m.MainParserT_parser_tmp_12 h.MainParserT_parser_lookahead_0.f
	mov m.MainParserT_parser_tmp m.MainParserT_parser_tmp_12
	shr m.MainParserT_parser_tmp 0x8
	mov m.MainParserT_parser_tmp_0 m.MainParserT_parser_tmp
	and m.MainParserT_parser_tmp_0 0xFFFF
	mov m.MainParserT_parser_tmp_1 m.MainParserT_parser_tmp_0
	and m.MainParserT_parser_tmp_1 0xFFFF
	mov m.local_metadata__s1_type10 m.MainParserT_parser_tmp_1
	mov m.MainParserT_parser_tmp_2 m.MainParserT_parser_tmp_12
	and m.MainParserT_parser_tmp_2 0xFF
	mov m.MainParserT_parser_tmp_3 m.MainParserT_parser_tmp_2
	and m.MainParserT_parser_tmp_3 0xFF
	mov m.local_metadata__s1_type21 m.MainParserT_parser_tmp_3
	mov m.MainParserT_parser_tmp_4 m.MainParserT_parser_tmp_12
	and m.MainParserT_parser_tmp_4 0xFF
	mov m.MainParserT_parser_tmp_5 m.MainParserT_parser_tmp_4
	and m.MainParserT_parser_tmp_5 0xFF
	jmpeq MAINPARSERIMPL_PARSE_H2 m.MainParserT_parser_tmp_5 0x1
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	extract h.h2
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_metadata_output_port
}


