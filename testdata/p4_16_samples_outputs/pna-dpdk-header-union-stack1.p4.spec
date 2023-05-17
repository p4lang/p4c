
struct Header1 {
	bit<32> data
}

struct Header2 {
	bit<16> data
}

header h1 instanceof Header1
header h2_0 instanceof Header2
header h2_1 instanceof Header2

header u0_h1 instanceof Header1
header u0_h2 instanceof Header2
header u0_h3 instanceof Header1
header u1_h1 instanceof Header1
header u1_h2 instanceof Header2
header u1_h3 instanceof Header1

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
	mov h.h2_0.data 0x1
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
	validate h.u0_h1
	invalidate h.u0_h2
	invalidate h.u0_h3
	validate h.u0_h1
	mov h.u0_h1.data 0x1
	invalidate h.u0_h2
	invalidate h.u0_h3
	validate h.u0_h2
	invalidate h.u0_h1
	invalidate h.u0_h3
	jmpnv LABEL_FALSE_0 h.u0_h1
	validate h.u1_h1
	jmpnv LABEL_FALSE_1 h.u0_h1
	validate h.u1_h1
	jmp LABEL_END_2
	LABEL_FALSE_1 :	invalidate h.u1_h1
	LABEL_END_2 :	mov h.u1_h1.data h.u0_h1.data
	invalidate h.u1_h2
	invalidate h.u1_h3
	jmp LABEL_END_1
	LABEL_FALSE_0 :	invalidate h.u1_h1
	LABEL_END_1 :	jmpnv LABEL_FALSE_2 h.u0_h2
	validate h.u1_h2
	jmpnv LABEL_FALSE_3 h.u0_h2
	validate h.u1_h2
	jmp LABEL_END_4
	LABEL_FALSE_3 :	invalidate h.u1_h2
	LABEL_END_4 :	mov h.u1_h2.data h.u0_h2.data
	invalidate h.u1_h1
	invalidate h.u1_h3
	jmp LABEL_END_3
	LABEL_FALSE_2 :	invalidate h.u1_h2
	LABEL_END_3 :	jmpnv LABEL_FALSE_4 h.u0_h3
	validate h.u1_h3
	jmpnv LABEL_FALSE_5 h.u0_h3
	validate h.u1_h3
	jmp LABEL_END_6
	LABEL_FALSE_5 :	invalidate h.u1_h3
	LABEL_END_6 :	mov h.u1_h3.data h.u0_h3.data
	invalidate h.u1_h1
	invalidate h.u1_h2
	jmp LABEL_END_5
	LABEL_FALSE_4 :	invalidate h.u1_h3
	LABEL_END_5 :	validate h.u1_h1
	mov h.u1_h1.data 0x1
	invalidate h.u1_h2
	invalidate h.u1_h3
	validate h.u1_h2
	mov h.u1_h2.data 0x1
	invalidate h.u1_h1
	invalidate h.u1_h3
	validate h.u0_h2
	invalidate h.u0_h1
	invalidate h.u0_h3
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
	tx m.pna_main_output_metadata_output_port
}


