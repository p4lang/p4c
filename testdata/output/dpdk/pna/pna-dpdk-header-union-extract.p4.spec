
struct H {
	bit<8> x
}

header u_a instanceof H
header u_b instanceof H
header s0_a instanceof H
header s0_b instanceof H
header MainParserT_parser_l_0_a instanceof H
header MainParserT_parser_l_0_b instanceof H

struct metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof metadata_t

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	invalidate h.MainParserT_parser_l_0_a
	invalidate h.MainParserT_parser_l_0_b
	extract h.MainParserT_parser_l_0_b
	invalidate h.MainParserT_parser_l_0_a
	extract h.MainParserT_parser_l_0_a
	invalidate h.MainParserT_parser_l_0_b
	extract h.u_b
	invalidate h.u_a
	extract h.u_a
	invalidate h.u_b
	extract h.s0_b
	invalidate h.s0_a
	extract h.s0_a
	invalidate h.s0_b
	mov m.pna_main_output_metadata_output_port 0x0
	emit h.u_a
	emit h.u_b
	emit h.s0_a
	emit h.s0_b
	tx m.pna_main_output_metadata_output_port
}


