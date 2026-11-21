
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
	bit<8> IngressParser_parser_tmp
	bit<8> IngressParser_parser_tmp_0
}
metadata instanceof EMPTY

header u_h instanceof h_t
header u_other instanceof h_t
header IngressParser_parser_ih_h instanceof h_t
header IngressParser_parser_ih_other instanceof h_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	validate h.u_h
	invalidate h.u_other
	mov m.IngressParser_parser_tmp 1
	jmpv LABEL_END h.IngressParser_parser_ih_h
	mov m.IngressParser_parser_tmp 0
	LABEL_END :	jmpeq MYIP_START_TRUE m.IngressParser_parser_tmp 1
	invalidate h.u_h
	jmp MYIP_START_JOIN
	MYIP_START_TRUE :	validate h.u_h
	invalidate h.u_other
	MYIP_START_JOIN :	mov m.IngressParser_parser_tmp_0 1
	jmpv LABEL_END_0 h.IngressParser_parser_ih_other
	mov m.IngressParser_parser_tmp_0 0
	LABEL_END_0 :	jmpeq MYIP_START_TRUE_0 m.IngressParser_parser_tmp_0 1
	invalidate h.u_other
	jmp MYIP_START_JOIN_0
	MYIP_START_TRUE_0 :	validate h.u_other
	invalidate h.u_h
	MYIP_START_JOIN_0 :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.u_h
	emit h.u_other
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}
