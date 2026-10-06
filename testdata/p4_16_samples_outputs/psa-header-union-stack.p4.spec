
struct Tcp_option_end_h {
	bit<8> kind
}

struct Tcp_option_nop_h {
	bit<8> kind
}

struct Tcp_option_ss_h {
	bit<8> kind
	bit<32> maxSegmentSize
}

struct Tcp_option_s_h {
	bit<8> kind
	bit<24> scale
}

struct Tcp_option_sack_h {
	bit<8> kind
	bit<8> length
	varbit<256> sack
}

struct lookahead_tmp_hdr {
	bit<16> f
}

struct lookahead_tmp_hdr_0 {
	bit<8> f
}

struct lookahead_tmp_hdr_1 {
	bit<16> f
}

struct lookahead_tmp_hdr_2 {
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

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<16> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> IngressParser_parser_tmp
	bit<16> IngressParser_parser_tmp_0
	bit<8> IngressParser_parser_tmp_2
	bit<8> IngressParser_parser_tmp_3
	bit<16> IngressParser_parser_tmp_4
	bit<16> IngressParser_parser_tmp_5
	bit<8> IngressParser_parser_tmp_7
	bit<8> IngressParser_parser_tmp_8
	bit<8> IngressParser_parser_tmp_11
	bit<16> IngressParser_parser_tmp_12
	bit<32> IngressParser_parser_tmp_9_extract_tmp
	bit<32> IngressParser_parser_tmp_10_extract_tmp
}
metadata instanceof metadata_t

header options0_end instanceof Tcp_option_end_h
header options0_nop instanceof Tcp_option_nop_h
header options0_ss instanceof Tcp_option_ss_h
header options0_s instanceof Tcp_option_s_h
header options0_sack instanceof Tcp_option_sack_h
header options1_end instanceof Tcp_option_end_h
header options1_nop instanceof Tcp_option_nop_h
header options1_ss instanceof Tcp_option_ss_h
header options1_s instanceof Tcp_option_s_h
header options1_sack instanceof Tcp_option_sack_h
header options2_end instanceof Tcp_option_end_h
header options2_nop instanceof Tcp_option_nop_h
header options2_ss instanceof Tcp_option_ss_h
header options2_s instanceof Tcp_option_s_h
header options2_sack instanceof Tcp_option_sack_h
header options3_end instanceof Tcp_option_end_h
header options3_nop instanceof Tcp_option_nop_h
header options3_ss instanceof Tcp_option_ss_h
header options3_s instanceof Tcp_option_s_h
header options3_sack instanceof Tcp_option_sack_h
header options4_end instanceof Tcp_option_end_h
header options4_nop instanceof Tcp_option_nop_h
header options4_ss instanceof Tcp_option_ss_h
header options4_s instanceof Tcp_option_s_h
header options4_sack instanceof Tcp_option_sack_h
header options5_end instanceof Tcp_option_end_h
header options5_nop instanceof Tcp_option_nop_h
header options5_ss instanceof Tcp_option_ss_h
header options5_s instanceof Tcp_option_s_h
header options5_sack instanceof Tcp_option_sack_h
header options6_end instanceof Tcp_option_end_h
header options6_nop instanceof Tcp_option_nop_h
header options6_ss instanceof Tcp_option_ss_h
header options6_s instanceof Tcp_option_s_h
header options6_sack instanceof Tcp_option_sack_h
header options7_end instanceof Tcp_option_end_h
header options7_nop instanceof Tcp_option_nop_h
header options7_ss instanceof Tcp_option_ss_h
header options7_s instanceof Tcp_option_s_h
header options7_sack instanceof Tcp_option_sack_h
header options8_end instanceof Tcp_option_end_h
header options8_nop instanceof Tcp_option_nop_h
header options8_ss instanceof Tcp_option_ss_h
header options8_s instanceof Tcp_option_s_h
header options8_sack instanceof Tcp_option_sack_h
header options9_end instanceof Tcp_option_end_h
header options9_nop instanceof Tcp_option_nop_h
header options9_ss instanceof Tcp_option_ss_h
header options9_s instanceof Tcp_option_s_h
header options9_sack instanceof Tcp_option_sack_h
;oldname:IngressParser_parser_lookahead_tmp
header IngressParser_parser_lookahea0 instanceof lookahead_tmp_hdr
;oldname:IngressParser_parser_lookahead_tmp_0
header IngressParser_parser_lookahea1 instanceof lookahead_tmp_hdr_0
;oldname:EgressParser_parser_lookahead_tmp
header EgressParser_parser_lookahead2 instanceof lookahead_tmp_hdr_1
;oldname:EgressParser_parser_lookahead_tmp_0
header EgressParser_parser_lookahead3 instanceof lookahead_tmp_hdr_2

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	lookahead h.IngressParser_parser_lookahea1
	mov m.IngressParser_parser_tmp_11 h.IngressParser_parser_lookahea1.f
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_END m.IngressParser_parser_tmp_11 0x0
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_NOP m.IngressParser_parser_tmp_11 0x1
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_SS m.IngressParser_parser_tmp_11 0x2
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_S m.IngressParser_parser_tmp_11 0x3
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_SACK m.IngressParser_parser_tmp_11 0x5
	jmp HEADERUNIONINGRESSPARSER_ACCEPT
	HEADERUNIONINGRESSPARSER_PARSE_SS :	extract h.options0_ss
	jmp HEADERUNIONINGRESSPARSER_START1
	HEADERUNIONINGRESSPARSER_PARSE_SACK :	lookahead h.IngressParser_parser_lookahea0
	mov m.IngressParser_parser_tmp_12 h.IngressParser_parser_lookahea0.f
	mov m.IngressParser_parser_tmp m.IngressParser_parser_tmp_12
	and m.IngressParser_parser_tmp 0xFF
	mov m.IngressParser_parser_tmp_0 m.IngressParser_parser_tmp
	and m.IngressParser_parser_tmp_0 0xFF
	mov m.IngressParser_parser_tmp_2 m.IngressParser_parser_tmp_0
	shl m.IngressParser_parser_tmp_2 0x3
	mov m.IngressParser_parser_tmp_3 m.IngressParser_parser_tmp_2
	add m.IngressParser_parser_tmp_3 0xF0
	mov m.IngressParser_parser_tmp_9_extract_tmp m.IngressParser_parser_tmp_3
	shr m.IngressParser_parser_tmp_9_extract_tmp 0x3
	extract h.options0_sack m.IngressParser_parser_tmp_9_extract_tmp
	jmp HEADERUNIONINGRESSPARSER_START1
	HEADERUNIONINGRESSPARSER_PARSE_S :	extract h.options0_s
	jmp HEADERUNIONINGRESSPARSER_START1
	HEADERUNIONINGRESSPARSER_PARSE_NOP :	extract h.options0_nop
	HEADERUNIONINGRESSPARSER_START1 :	lookahead h.IngressParser_parser_lookahea1
	mov m.IngressParser_parser_tmp_11 h.IngressParser_parser_lookahea1.f
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_END1 m.IngressParser_parser_tmp_11 0x0
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_NOP1 m.IngressParser_parser_tmp_11 0x1
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_SS1 m.IngressParser_parser_tmp_11 0x2
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_S1 m.IngressParser_parser_tmp_11 0x3
	jmpeq HEADERUNIONINGRESSPARSER_PARSE_SACK1 m.IngressParser_parser_tmp_11 0x5
	jmp HEADERUNIONINGRESSPARSER_ACCEPT
	HEADERUNIONINGRESSPARSER_PARSE_SS1 :	extract h.options1_ss
	jmp HEADERUNIONINGRESSPARSER_START2
	HEADERUNIONINGRESSPARSER_PARSE_SACK1 :	lookahead h.IngressParser_parser_lookahea0
	mov m.IngressParser_parser_tmp_12 h.IngressParser_parser_lookahea0.f
	mov m.IngressParser_parser_tmp_4 m.IngressParser_parser_tmp_12
	and m.IngressParser_parser_tmp_4 0xFF
	mov m.IngressParser_parser_tmp_5 m.IngressParser_parser_tmp_4
	and m.IngressParser_parser_tmp_5 0xFF
	mov m.IngressParser_parser_tmp_7 m.IngressParser_parser_tmp_5
	shl m.IngressParser_parser_tmp_7 0x3
	mov m.IngressParser_parser_tmp_8 m.IngressParser_parser_tmp_7
	add m.IngressParser_parser_tmp_8 0xF0
	mov m.IngressParser_parser_tmp_10_extract_tmp m.IngressParser_parser_tmp_8
	shr m.IngressParser_parser_tmp_10_extract_tmp 0x3
	extract h.options1_sack m.IngressParser_parser_tmp_10_extract_tmp
	jmp HEADERUNIONINGRESSPARSER_START2
	HEADERUNIONINGRESSPARSER_PARSE_S1 :	extract h.options1_s
	jmp HEADERUNIONINGRESSPARSER_START2
	HEADERUNIONINGRESSPARSER_PARSE_NOP1 :	extract h.options1_nop
	HEADERUNIONINGRESSPARSER_START2 :	mov m.psa_ingress_input_metadata_parser_error 0x3
	jmp HEADERUNIONINGRESSPARSER_ACCEPT
	jmp HEADERUNIONINGRESSPARSER_ACCEPT
	HEADERUNIONINGRESSPARSER_PARSE_END1 :	extract h.options1_end
	jmp HEADERUNIONINGRESSPARSER_ACCEPT
	HEADERUNIONINGRESSPARSER_PARSE_END :	extract h.options0_end
	HEADERUNIONINGRESSPARSER_ACCEPT :	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port 0x1
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.options0_end
	emit h.options0_nop
	emit h.options0_ss
	emit h.options0_s
	emit h.options0_sack
	emit h.options1_end
	emit h.options1_nop
	emit h.options1_ss
	emit h.options1_s
	emit h.options1_sack
	emit h.options2_end
	emit h.options2_nop
	emit h.options2_ss
	emit h.options2_s
	emit h.options2_sack
	emit h.options3_end
	emit h.options3_nop
	emit h.options3_ss
	emit h.options3_s
	emit h.options3_sack
	emit h.options4_end
	emit h.options4_nop
	emit h.options4_ss
	emit h.options4_s
	emit h.options4_sack
	emit h.options5_end
	emit h.options5_nop
	emit h.options5_ss
	emit h.options5_s
	emit h.options5_sack
	emit h.options6_end
	emit h.options6_nop
	emit h.options6_ss
	emit h.options6_s
	emit h.options6_sack
	emit h.options7_end
	emit h.options7_nop
	emit h.options7_ss
	emit h.options7_s
	emit h.options7_sack
	emit h.options8_end
	emit h.options8_nop
	emit h.options8_ss
	emit h.options8_s
	emit h.options8_sack
	emit h.options9_end
	emit h.options9_nop
	emit h.options9_ss
	emit h.options9_s
	emit h.options9_sack
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


