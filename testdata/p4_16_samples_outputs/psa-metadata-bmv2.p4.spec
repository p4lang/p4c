
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
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

struct set_output_arg_t {
	bit<32> port
}

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> local_metadata_field
	bit<48> Ingress_tmp
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<48> Ingress_tmp_2
	bit<8> Ingress_key
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

action NoAction args none {
	return
}

action set_output args instanceof set_output_arg_t {
	mov m.psa_ingress_output_metadata_egress_port t.port
	return
}

table match_meta {
	key {
		m.Ingress_key exact
	}
	actions {
		set_output
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	invalidate h.ethernet
	extract h.ethernet
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xFF
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFF
	mov m.local_metadata_field m.Ingress_tmp_0
	mov m.Ingress_tmp_1 h.ethernet.dstAddr
	and m.Ingress_tmp_1 0xFF
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	and m.Ingress_tmp_2 0xFF
	mov m.Ingress_key m.Ingress_tmp_2
	table match_meta
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


