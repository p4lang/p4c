
struct Header1 {
	bit<32> data
}

struct Header2 {
	bit<16> data
}

header h1 instanceof Header1
header u1_h1 instanceof Header1
header u1_h2 instanceof Header2
header u1_h3 instanceof Header1
header u2_h1 instanceof Header1
header u2_h2 instanceof Header2
header u2_h3 instanceof Header1
header MainControlT_u1_0_h1 instanceof Header1
header MainControlT_u1_0_h2 instanceof Header2
header MainControlT_u1_0_h3 instanceof Header1
header MainControlT_u2_0_h1 instanceof Header1
header MainControlT_u2_0_h2 instanceof Header2
header MainControlT_u2_0_h3 instanceof Header1

struct M {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof M

regarray direction size 0x100 initval 0

apply {
	rx m.pna_main_input_metadata_input_port
	validate h.u1_h1
	extract h.u1_h3
	mov h.u1_h3.data 0x1
	jmpnv LABEL_FALSE h.u1_h3
	validate h.u1_h1
	jmp LABEL_END
	LABEL_FALSE :	invalidate h.u1_h1
	LABEL_END :	mov h.u1_h1.data h.u1_h3.data
	mov h.u1_h1.data 0x1
	mov h.u1_h1.data 0x1
	mov h.u1_h2.data 0x1
	mov h.u1_h3.data 0x1
	invalidate h.MainControlT_u1_0_h1
	invalidate h.MainControlT_u1_0_h2
	invalidate h.MainControlT_u1_0_h3
	invalidate h.MainControlT_u2_0_h1
	invalidate h.MainControlT_u2_0_h2
	invalidate h.MainControlT_u2_0_h3
	validate h.MainControlT_u1_0_h1
	mov h.MainControlT_u1_0_h1.data 0x1
	validate h.MainControlT_u1_0_h2
	jmpnv LABEL_FALSE_0 h.MainControlT_u1_0_h1
	validate h.MainControlT_u2_0_h1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	invalidate h.MainControlT_u2_0_h1
	LABEL_END_0 :	mov h.MainControlT_u2_0_h1.data h.MainControlT_u1_0_h1.data
	jmpnv LABEL_FALSE_1 h.MainControlT_u1_0_h2
	validate h.MainControlT_u2_0_h2
	jmp LABEL_END_1
	LABEL_FALSE_1 :	invalidate h.MainControlT_u2_0_h2
	LABEL_END_1 :	mov h.MainControlT_u2_0_h2.data h.MainControlT_u1_0_h2.data
	jmpnv LABEL_FALSE_2 h.MainControlT_u1_0_h3
	validate h.MainControlT_u2_0_h3
	jmp LABEL_END_2
	LABEL_FALSE_2 :	invalidate h.MainControlT_u2_0_h3
	LABEL_END_2 :	mov h.MainControlT_u2_0_h3.data h.MainControlT_u1_0_h3.data
	mov h.MainControlT_u2_0_h2.data 0x1
	validate h.MainControlT_u1_0_h2
	invalidate h.MainControlT_u1_0_h3
	tx m.pna_main_output_metadata_output_port
}


