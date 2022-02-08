
struct my_header_t {
	bit<16> type1
	bit<8> type2
	bit<32> value
}

struct tmp_1_header {
	bit<24> tmp_1
}

struct tmp_header {
	bit<24> tmp
}

struct main_metadata_t {
	bit<32> pna_pre_input_metadata_input_port
	bit<16> pna_pre_input_metadata_parser_error
	bit<32> pna_pre_input_metadata_direction
	bit<3> pna_pre_input_metadata_pass
	bit<8> pna_pre_input_metadata_loopedback
	bit<8> pna_pre_output_metadata_decrypt
	bit<32> pna_pre_output_metadata_said
	bit<16> pna_pre_output_metadata_decrypt_start_offset
	bit<32> pna_main_parser_input_metadata_direction
	bit<3> pna_main_parser_input_metadata_pass
	bit<8> pna_main_parser_input_metadata_loopedback
	bit<32> pna_main_parser_input_metadata_input_port
	bit<32> pna_main_input_metadata_direction
	bit<3> pna_main_input_metadata_pass
	bit<8> pna_main_input_metadata_loopedback
	bit<64> pna_main_input_metadata_timestamp
	bit<16> pna_main_input_metadata_parser_error
	bit<8> pna_main_input_metadata_class_of_service
	bit<32> pna_main_input_metadata_input_port
	bit<8> pna_main_output_metadata_class_of_service
	bit<16> local_metadata__s1_type10
	bit<8> local_metadata__s1_type21
	bit<32> pna_main_output_metadata_output_port
	bit<24> MainParserT_parser_tmp_3
	bit<24> MainParserT_parser_tmp_4
	bit<8> MainParserT_parser_tmp_0
	bit<16> MainParserT_parser_tmp_2
	bit<24> MainParserT_parser_tmp
	bit<24> MainParserT_parser_tmp_1
}
metadata instanceof main_metadata_t

header h1 instanceof my_header_t
header h2 instanceof my_header_t
header MainParserT_parser_tmp_1_tmp_h instanceof tmp_1_header
header MainParserT_parser_tmp_tmp_h instanceof tmp_header

apply {
	rx m.pna_main_input_metadata_input_port
	lookahead h.MainParserT_parser_tmp_tmp_h
	mov m.MainParserT_parser_tmp h.MainParserT_parser_tmp_tmp_h.tmp
	mov m.MainParserT_parser_tmp_4 m.MainParserT_parser_tmp
	shr m.MainParserT_parser_tmp_4 0x8
	mov m.MainParserT_parser_tmp_2 m.MainParserT_parser_tmp_4
	jmpeq MAINPARSERIMPL_PARSE_H1 m.MainParserT_parser_tmp_2 0x1234
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H1 :	extract h.h1
	lookahead h.MainParserT_parser_tmp_1_tmp_h
	mov m.MainParserT_parser_tmp_1 h.MainParserT_parser_tmp_1_tmp_h.tmp_1
	mov m.MainParserT_parser_tmp_3 m.MainParserT_parser_tmp_1
	shr m.MainParserT_parser_tmp_3 0x8
	mov m.local_metadata__s1_type10 m.MainParserT_parser_tmp_3
	mov m.local_metadata__s1_type21 m.MainParserT_parser_tmp_1
	mov m.MainParserT_parser_tmp_0 m.MainParserT_parser_tmp_1
	jmpeq MAINPARSERIMPL_PARSE_H2 m.MainParserT_parser_tmp_0 0x1
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_H2 :	extract h.h2
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_metadata_output_port
}


