
struct H {
	bit<8> value
}

header h instanceof H

struct Metadata {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControlT_tmp_0
	bit<8> MainControlT_tmp_1
}
metadata instanceof Metadata

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h
	jmpneq LABEL_FALSE h.h.value 0x1
	mov m.MainControlT_tmp_1 0x2
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainControlT_tmp_1 0x0
	LABEL_END :	mov m.MainControlT_tmp_0 m.MainControlT_tmp_1
	shl m.MainControlT_tmp_0 0x1
	mov h.h.value m.MainControlT_tmp_0
	or h.h.value 0x1
	mov m.pna_main_output_metadata_output_port 0x1
	emit h.h
	tx m.pna_main_output_metadata_output_port
}


