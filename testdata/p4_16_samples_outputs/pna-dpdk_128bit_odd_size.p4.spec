
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct custom_t {
	bit<8> padding_f1_f2_f4
	bit<8> f8
	bit<16> f16
	bit<32> f32
	bit<64> f64
	bit<128> f128
}

struct a1_arg_t {
	bit<80> x
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header custom instanceof custom_t

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action a1 args instanceof a1_arg_t {
	mov h.custom.f128 t.x
	return
}

table t1 {
	key {
		h.ethernet.dstAddr exact
	}
	actions {
		a1 @tableonly
		NoAction @defaultonly
	}
	default_action NoAction args none const
	size 0x200
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_CUSTOM h.ethernet.etherType 0x8FF
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_CUSTOM :	extract h.custom
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.custom
	table t1
	LABEL_END :	emit h.ethernet
	emit h.custom
	tx m.pna_main_output_metadata_output_port
}


