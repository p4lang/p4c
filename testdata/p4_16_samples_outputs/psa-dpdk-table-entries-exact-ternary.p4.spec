
struct hdr {
	bit<8> e
	bit<16> t
	bit<8> l
	bit<8> r
	bit<8> v
}

struct psa_ingress_output_metadata_t {
	bit<8> class_of_service
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_output_metadata_t {
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
}

struct psa_egress_deparser_input_metadata_t {
	bit<32> egress_port
}

struct a_with_control_params_arg_t {
	bit<32> x
}

header h instanceof hdr

struct Meta_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
}
metadata instanceof Meta_t

action a_1 args none {
	mov m.psa_ingress_output_metadata_egress_port 0x0
	return
}

action a_with_control_params args instanceof a_with_control_params_arg_t {
	mov m.psa_ingress_output_metadata_egress_port t.x
	return
}

table t_exact_ternary {
	key {
		h.h.e exact
		h.h.t wildcard
	}
	actions {
		a_1
		a_with_control_params
	}
	default_action a_1 args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.h
	table t_exact_ternary
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.h
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


