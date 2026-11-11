
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
	bit<8> IngressParser_parser_tmp_1
	bit<8> IngressParser_parser_tmp_2
	bit<8> IngressParser_parser_tmp_3
	bit<8> IngressParser_parser_tmp_4
}
metadata instanceof EMPTY

header u_h instanceof h_t
header u_other instanceof h_t
header IngressParser_parser_ih_0_h instanceof h_t
;oldname:IngressParser_parser_ih_0_other
header IngressParser_parser_ih_0_oth0 instanceof h_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	validate h.u_h
	invalidate h.u_other
	jmpnv LABEL_FALSE h.IngressParser_parser_ih_0_h
	mov m.IngressParser_parser_tmp_3 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.IngressParser_parser_tmp_3 0x0
	LABEL_END :	mov m.IngressParser_parser_tmp m.IngressParser_parser_tmp_3
	jmpeq MYIP_START_TRUE m.IngressParser_parser_tmp 0x1
	jmpeq MYIP_START_FALSE m.IngressParser_parser_tmp 0x0
	MYIP_START_TRUE :	mov m.IngressParser_parser_tmp_2 1
	jmpv LABEL_END_0 h.IngressParser_parser_ih_0_h
	mov m.IngressParser_parser_tmp_2 0
	LABEL_END_0 :	jmpeq MYIP_START_TRUE_TRUE m.IngressParser_parser_tmp_2 1
	invalidate h.u_h
	invalidate h.u_other
	jmp MYIP_START_JOIN
	MYIP_START_TRUE_TRUE :	validate h.u_h
	invalidate h.u_other
	jmp MYIP_START_JOIN
	MYIP_START_FALSE :	jmpnv LABEL_FALSE_0 h.IngressParser_parser_ih_0_oth0
	mov m.IngressParser_parser_tmp_4 0x1
	jmp LABEL_END_1
	LABEL_FALSE_0 :	mov m.IngressParser_parser_tmp_4 0x0
	LABEL_END_1 :	mov m.IngressParser_parser_tmp_0 m.IngressParser_parser_tmp_4
	jmpeq MYIP_START_FALSE_TRUE m.IngressParser_parser_tmp_0 0x1
	jmpeq MYIP_START_FALSE_FALSE m.IngressParser_parser_tmp_0 0x0
	MYIP_START_FALSE_TRUE :	mov m.IngressParser_parser_tmp_1 1
	jmpv LABEL_END_2 h.IngressParser_parser_ih_0_oth0
	mov m.IngressParser_parser_tmp_1 0
	LABEL_END_2 :	jmpeq MYIP_START_FALSE_TRUE_TRUE m.IngressParser_parser_tmp_1 1
	invalidate h.u_other
	invalidate h.u_h
	jmp MYIP_START_JOIN
	MYIP_START_FALSE_TRUE_TRUE :	validate h.u_other
	invalidate h.u_h
	jmp MYIP_START_JOIN
	MYIP_START_FALSE_FALSE :	invalidate h.u_h
	invalidate h.u_other
	MYIP_START_JOIN :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.u_h
	emit h.u_other
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}
