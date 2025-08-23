
struct Header1 {
	bit<32> data
}

struct Header2 {
	bit<16> data
}

header h1 instanceof Header1
header u0_h1 instanceof Header1
header u0_h2 instanceof Header2
header u0_h3 instanceof Header1
header u1_h1 instanceof Header1
header u1_h2 instanceof Header2
header u1_h3 instanceof Header1
header MainControlT_u_1_h1 instanceof Header1
header MainControlT_u_1_h2 instanceof Header2
header MainControlT_u_1_h3 instanceof Header1
header MainControlT_u_2_h1 instanceof Header1
header MainControlT_u_2_h2 instanceof Header2
header MainControlT_u_2_h3 instanceof Header1

struct M {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainParserT_parser_tmp
}
metadata instanceof M

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	validate h.u0_h1
	invalidate h.u0_h2
	invalidate h.u0_h3
	extract h.u0_h3
	validate h.u0_h3
	mov h.u0_h3.data 0x1
	invalidate h.u0_h1
	invalidate h.u0_h2
	mov m.MainParserT_parser_tmp 1
	jmpv LABEL_END h.u0_h3
	mov m.MainParserT_parser_tmp 0
	LABEL_END :	jmpeq PARSERIMPL_START_TRUE m.MainParserT_parser_tmp 1
	invalidate h.u0_h1
	jmp PARSERIMPL_START_JOIN
	PARSERIMPL_START_TRUE :	validate h.u0_h1
	jmpnv LABEL_FALSE h.u0_h3
	validate h.u0_h1
	jmp LABEL_END_0
	LABEL_FALSE :	invalidate h.u0_h1
	LABEL_END_0 :	mov h.u0_h1.data h.u0_h3.data
	invalidate h.u0_h2
	invalidate h.u0_h3
	PARSERIMPL_START_JOIN :	validate h.u0_h1
	mov h.u0_h1.data 0x1
	invalidate h.u0_h2
	invalidate h.u0_h3
	validate h.u0_h1
	mov h.u0_h1.data 0x1
	invalidate h.u0_h2
	invalidate h.u0_h3
	validate h.u0_h2
	mov h.u0_h2.data 0x1
	invalidate h.u0_h1
	invalidate h.u0_h3
	validate h.u0_h3
	mov h.u0_h3.data 0x1
	invalidate h.u0_h1
	invalidate h.u0_h2
	invalidate h.MainControlT_u_1_h1
	invalidate h.MainControlT_u_1_h2
	invalidate h.MainControlT_u_1_h3
	invalidate h.MainControlT_u_2_h1
	invalidate h.MainControlT_u_2_h2
	invalidate h.MainControlT_u_2_h3
	validate h.MainControlT_u_1_h1
	invalidate h.MainControlT_u_1_h2
	invalidate h.MainControlT_u_1_h3
	validate h.MainControlT_u_1_h1
	mov h.MainControlT_u_1_h1.data 0x1
	invalidate h.MainControlT_u_1_h2
	invalidate h.MainControlT_u_1_h3
	validate h.MainControlT_u_1_h2
	invalidate h.MainControlT_u_1_h1
	invalidate h.MainControlT_u_1_h3
	validate h.MainControlT_u_2_h2
	mov h.MainControlT_u_2_h2.data 0x1
	invalidate h.MainControlT_u_2_h1
	invalidate h.MainControlT_u_2_h3
	validate h.MainControlT_u_1_h2
	invalidate h.MainControlT_u_1_h1
	invalidate h.MainControlT_u_1_h3
	invalidate h.MainControlT_u_1_h3
	tx m.pna_main_output_metadata_output_port
}


