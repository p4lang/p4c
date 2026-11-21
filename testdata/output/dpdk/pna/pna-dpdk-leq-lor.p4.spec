
struct hdr_t {
	bit<8> a
	bit<8> b
	bit<8> c
	bit<8> d
}

header h instanceof hdr_t

struct Meta {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof Meta

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h
	jmpgt LABEL_END h.h.a h.h.b
	jmpeq LABEL_TRUE h.h.c h.h.d
	jmp LABEL_END
	LABEL_TRUE :	add h.h.a 0x1
	LABEL_END :	jmplt LABEL_END_0 h.h.a h.h.b
	jmpneq LABEL_TRUE_0 h.h.c h.h.d
	jmp LABEL_END_0
	LABEL_TRUE_0 :	add h.h.b 0x1
	LABEL_END_0 :	jmpgt LABEL_END_1 h.h.a h.h.b
	jmpneq LABEL_END_1 h.h.c h.h.d
	add h.h.c 0x1
	LABEL_END_1 :	emit h.h
	tx m.pna_main_output_metadata_output_port
}


