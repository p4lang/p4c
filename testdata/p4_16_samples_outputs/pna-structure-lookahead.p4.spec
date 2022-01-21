
struct my_header_t {
	bit<8> type1
	bit<8> type2
	bit<8> value
}

struct my_struct_t {
	bit<8> type1
	bit<8> type2
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
	bit<32> pna_main_output_metadata_output_port
	my_struct_t MainParserT_parser_tmp_0
}
metadata instanceof main_metadata_t

header h instanceof my_header_t

apply {
	rx m.pna_main_input_metadata_input_port
	lookahead m.MainParserT_parser_tmp_0
	jmpeq MAINPARSERIMPL_PARSE_HEADER m.MainParserT_parser_tmp_0.type2 0x1
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_HEADER :	extract h.h
	MAINPARSERIMPL_ACCEPT :	tx m.pna_main_output_metadata_output_port
}


