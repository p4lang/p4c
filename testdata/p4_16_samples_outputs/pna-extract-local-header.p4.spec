
struct my_header_t {
	bit<16> type1
	bit<8> type2
	bit<32> value
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header h1 instanceof my_header_t
header h2 instanceof my_header_t
header MainParserT_parser_local_hdr instanceof my_header_t

regarray direction size 0x100 initval 0

apply {
	rx m.pna_main_input_metadata_input_port
	invalidate h.MainParserT_parser_local_hdr
	extract h.MainParserT_parser_local_hdr
	jmpeq MAINPARSERIMPL_PARSE_H1 h.MainParserT_parser_local_hdr.type1 0x1234
	jmpeq MAINPARSERIMPL_PARSE_H2 h.MainParserT_parser_local_hdr.type1 0x5678
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	validate h.h2
	mov h.h2.type1 h.MainParserT_parser_local_hdr.type1
	mov h.h2.type2 h.MainParserT_parser_local_hdr.type2
	mov h.h2.value h.MainParserT_parser_local_hdr.value
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.h1
	jmpnv LABEL_END h.h2
	emit h.h1
	emit h.h2
	LABEL_END :	tx m.pna_main_output_metadata_output_port
}


