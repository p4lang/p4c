
struct Hdr1 {
	bit<8> a
}

struct Hdr2 {
	bit<16> b
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
	jmpeq P_GETH1 h.h1.a 0x0
	extract h.u_h2
	jmp P_ACCEPT
	P_GETH1 :	extract h.u_h1
	P_ACCEPT :	emit h.h1
	emit h.u_h1
	emit h.u_h2
	tx m.pna_main_output_metadata_output_port
}


