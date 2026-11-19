
struct h_t {
	bit<8> f
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

struct EMPTY {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
}
metadata instanceof EMPTY

header h instanceof h_t
header IngressParser_parser_ih instanceof h_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	invalidate h.IngressParser_parser_ih
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.h
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}
