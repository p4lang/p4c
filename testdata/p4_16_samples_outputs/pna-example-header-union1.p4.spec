
struct S {
	bit<8> t
}

struct O1 {
	bit<8> data
}

struct O2 {
	bit<16> data
}

header base instanceof S
header u_byte instanceof O1
header u_short instanceof O2
header MainControlT_hdr_1_u_byte instanceof O1
header MainControlT_hdr_1_u_short instanceof O2

struct metadata {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> ingress_debug_hdr_base_t
	bit<8> ingress_debug_hdr_u_short_isValid
	bit<8> ingress_debug_hdr_u_byte_isValid
	bit<8> MainControlT_hasReturned
	bit<8> MainControlT_retval
}
metadata instanceof metadata

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

table debug_hdr {
	key {
		m.ingress_debug_hdr_base_t exact
		m.ingress_debug_hdr_u_short_isValid exact
		m.ingress_debug_hdr_u_byte_isValid exact
	}
	actions {
		NoAction
	}
	default_action NoAction args none const
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.base
	jmpeq PARSERIMPL_PARSEO1 h.base.t 0x0
	jmpeq PARSERIMPL_PARSEO2 h.base.t 0x1
	jmp PARSERIMPL_ACCEPT
	PARSERIMPL_PARSEO2 :	extract h.u_short
	jmp PARSERIMPL_ACCEPT
	PARSERIMPL_PARSEO1 :	extract h.u_byte
	PARSERIMPL_ACCEPT :	mov m.ingress_debug_hdr_base_t h.base.t
	mov m.ingress_debug_hdr_u_short_isValid 1
	jmpv LABEL_END h.u_short
	mov m.ingress_debug_hdr_u_short_isValid 0
	LABEL_END :	mov m.ingress_debug_hdr_u_byte_isValid 1
	jmpv LABEL_END_0 h.u_byte
	mov m.ingress_debug_hdr_u_byte_isValid 0
	LABEL_END_0 :	table debug_hdr
	jmpnv LABEL_END_1 h.u_byte
	validate h.MainControlT_hdr_1_u_byte
	invalidate h.MainControlT_hdr_1_u_short
	LABEL_END_1 :	jmpnv LABEL_END_2 h.u_short
	validate h.MainControlT_hdr_1_u_short
	LABEL_END_2 :	mov m.MainControlT_hasReturned 0
	jmpnv LABEL_END_3 h.base
	jmpnv LABEL_END_3 h.u_short
	mov m.MainControlT_hasReturned 1
	mov m.MainControlT_retval 1
	LABEL_END_3 :	jmpneq LABEL_FALSE_2 m.MainControlT_hasReturned 0x1
	jmp LABEL_END_4
	LABEL_FALSE_2 :	mov m.MainControlT_retval 0
	LABEL_END_4 :	jmpneq LABEL_END_5 m.MainControlT_retval 0x1
	mov h.base.t 0x3
	LABEL_END_5 :	jmpnv LABEL_FALSE_4 h.u_short
	validate h.u_short
	mov h.u_short.data 0xFFFF
	invalidate h.u_byte
	jmp LABEL_END_6
	LABEL_FALSE_4 :	jmpnv LABEL_END_6 h.u_byte
	validate h.u_byte
	mov h.u_byte.data 0xFF
	invalidate h.u_short
	LABEL_END_6 :	emit h.base
	emit h.u_byte
	emit h.u_short
	tx m.pna_main_output_metadata_output_port
}


