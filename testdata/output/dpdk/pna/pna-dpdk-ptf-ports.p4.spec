
struct H {
	bit<8> forward
}

header h instanceof H

struct Metadata {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof Metadata

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h
	jmpneq LABEL_FALSE h.h.forward 0x1
	mov m.pna_main_output_metadata_output_port 0x1
	jmp LABEL_END
	LABEL_FALSE :	jmpneq LABEL_FALSE_0 h.h.forward 0x2
	mov m.pna_main_output_metadata_output_port 0x0
	jmp LABEL_END
	LABEL_FALSE_0 :	drop
	LABEL_END :	emit h.h
	tx m.pna_main_output_metadata_output_port
}


