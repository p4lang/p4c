
struct H {
	bit<8> value
}

struct forward_arg_t {
	bit<32> port
}

header h instanceof H

struct Metadata {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControl_lookup_0_MainControl_lookup_0_value
	bit<8> MainControl_lookup_0_MainControl_lookup_0_isValid
}
metadata instanceof Metadata

regarray direction size 0x100 initval 0
action forward args instanceof forward_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

action discard args none {
	drop
	return
}

table lookup_0 {
	key {
		m.MainControl_lookup_0_MainControl_lookup_0_value exact
		m.MainControl_lookup_0_MainControl_lookup_0_isValid exact
	}
	actions {
		forward
		discard
	}
	default_action discard args none const
	size 0x10
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h
	mov m.MainControl_lookup_0_MainControl_lookup_0_value h.h.value
	mov m.MainControl_lookup_0_MainControl_lookup_0_isValid 1
	jmpv LABEL_END h.h
	mov m.MainControl_lookup_0_MainControl_lookup_0_isValid 0
	LABEL_END :	table lookup_0
	emit h.h
	tx m.pna_main_output_metadata_output_port
}


