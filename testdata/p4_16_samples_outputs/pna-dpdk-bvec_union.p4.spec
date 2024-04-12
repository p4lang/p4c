
struct Hdr1 {
	bit<8> _a0
	;oldname:_row0_alt0_valid1__row0_alt0_port2
	bit<8> _row0_alt0_valid1__row0_alt0_0
	;oldname:_row0_alt1_valid3__row0_alt1_port4
	bit<8> _row0_alt1_valid3__row0_alt1_1
	;oldname:_row1_alt0_valid5__row1_alt0_port6
	bit<8> _row1_alt0_valid5__row1_alt0_2
	;oldname:_row1_alt1_valid7__row1_alt1_port8
	bit<8> _row1_alt1_valid7__row1_alt1_3
}

struct Hdr2 {
	bit<16> _b0
	;oldname:_row_alt0_valid1__row_alt0_port2
	bit<8> _row_alt0_valid1__row_alt0_po4
	;oldname:_row_alt1_valid3__row_alt1_port4
	bit<8> _row_alt1_valid3__row_alt1_po5
}

header h1 instanceof Hdr1
header u_h1 instanceof Hdr1
header u_h2 instanceof Hdr2

struct Meta {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof Meta

regarray direction size 0x100 initval 0
apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h1
	jmpeq PARSERIMPL_GETH1 h.h1._a0 0x0
	extract h.u_h2
	jmp PARSERIMPL_ACCEPT
	PARSERIMPL_GETH1 :	extract h.u_h1
	PARSERIMPL_ACCEPT :	jmpnv LABEL_END h.u_h2
	invalidate h.u_h2
	LABEL_END :	emit h.h1
	emit h.u_h1
	emit h.u_h2
	tx m.pna_main_output_metadata_output_port
}


