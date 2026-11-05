
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

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> ingress_test_table_ethernet_isValid
}
metadata instanceof metadata

header ethernet instanceof ethernet_t

action NoAction args none {
	return
}

action DummyAction args none {
	return
}

table test_table {
	key {
		m.ingress_test_table_ethernet_isValid wildcard
	}
	actions {
		DummyAction
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
	mov m.ingress_test_table_ethernet_isValid 1
	jmpv LABEL_END h.ethernet
	mov m.ingress_test_table_ethernet_isValid 0
	LABEL_END :	table test_table
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


