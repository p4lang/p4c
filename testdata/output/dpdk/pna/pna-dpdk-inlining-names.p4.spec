
struct H {
	bit<8> value
}

struct left_api_inner_api_send_0_arg_t {
	bit<32> port
}

struct right_api_inner_api_send_0_arg_t {
	bit<32> port
}

header h instanceof H

struct Metadata {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof Metadata

regarray direction size 0x100 initval 0
action left_api_inner_api_send_0 args instanceof left_api_inner_api_send_0_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

action left_api_inner_api_discard_0 args none {
	drop
	return
}

action right_api_inner_api_send_0 args instanceof right_api_inner_api_send_0_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

action right_api_inner_api_discard_0 args none {
	drop
	return
}

table left_api_inner_api_route {
	key {
		h.h.value exact
	}
	actions {
		left_api_inner_api_send_0
		left_api_inner_api_discard_0
	}
	default_action left_api_inner_api_discard_0 args none const
	size 0x10
}


table right_api_inner_api_route {
	key {
		h.h.value exact
	}
	actions {
		right_api_inner_api_send_0
		right_api_inner_api_discard_0
	}
	default_action right_api_inner_api_discard_0 args none const
	size 0x10
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.h
	jmpneq LABEL_FALSE h.h.value 0x1
	table left_api_inner_api_route
	jmp LABEL_END
	LABEL_FALSE :	table right_api_inner_api_route
	LABEL_END :	emit h.h
	tx m.pna_main_output_metadata_output_port
}
