

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_base_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffse_0
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct ipv4_option_tim_1 {
	bit<8> value
	bit<8> len
	varbit<304> data
}

struct lookahead_tmp_h_2 {
	bit<16> f
}

struct lookahead_tmp_h_3 {
	bit<8> f
}

struct psa_ingress_out_4 {
	bit<8> class_of_servic_5
	bit<8> clone
	bit<16> clone_session_i_6
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_7 {
	bit<8> clone
	bit<16> clone_session_i_6
	bit<8> drop
}

struct psa_egress_depa_8 {
	bit<32> egress_port
}

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct tbl2_set_member_9 {
	bit<32> member_id
}

struct tbl_set_member__10 {
	bit<32> member_id
}

header ethernet instanceof ethernet_t
header ipv4_base instanceof ipv4_base_t
header ipv4_option_tim_11 instanceof ipv4_option_tim_1
header IngressParser_p_12 instanceof lookahead_tmp_h_2
header IngressParser_p_13 instanceof lookahead_tmp_h_3

struct EMPTY {
	bit<32> psa_ingress_inp_14
	bit<8> psa_ingress_out_15
	bit<32> psa_ingress_out_16
	bit<32> psa_ingress_out_17
	bit<8> IngressParser_p_18
	bit<32> IngressParser_p_19
	bit<32> IngressParser_p_20
	bit<32> IngressParser_p_21
	bit<16> IngressParser_p_22
	bit<8> IngressParser_p_23
	bit<32> Ingress_ap_memb_24
	bit<32> IngressParser_p_25
}
metadata instanceof EMPTY

action NoAction args none {
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action tbl_set_member__26 args instanceof tbl_set_member__10 {
	mov m.Ingress_ap_memb_24 t.member_id
	return
}

action tbl2_set_member_27 args instanceof tbl2_set_member_9 {
	mov m.Ingress_ap_memb_24 t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_member__26
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ap {
	key {
		m.Ingress_ap_memb_24 exact
	}
	actions {
		NoAction
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl2 {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl2_set_member_27
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_14
	mov m.psa_ingress_out_15 0x0
	extract h.ethernet
	jmpeq MYIP_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4 :	extract h.ipv4_base
	jmpeq MYIP_ACCEPT h.ipv4_base.version_ihl 0x45
	lookahead h.IngressParser_p_13
	mov m.IngressParser_p_23 h.IngressParser_p_13.f
	jmpeq MYIP_PARSE_IPV4_OPTION_TIMESTAMP m.IngressParser_p_23 0x44
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4_OPTION_TIMESTAMP :	lookahead h.IngressParser_p_12
	mov m.IngressParser_p_22 h.IngressParser_p_12.f
	mov m.IngressParser_p_18 m.IngressParser_p_22
	mov m.IngressParser_p_19 m.IngressParser_p_18
	mov m.IngressParser_p_20 m.IngressParser_p_19
	shl m.IngressParser_p_20 0x3
	mov m.IngressParser_p_21 m.IngressParser_p_20
	add m.IngressParser_p_21 0xfffffff0
	mov m.IngressParser_p_25 m.IngressParser_p_21
	shr m.IngressParser_p_25 0x3
	extract h.ipv4_option_tim_11 m.IngressParser_p_25
	MYIP_ACCEPT :	mov m.psa_ingress_out_15 0
	mov m.psa_ingress_out_16 0x0
	mov m.psa_ingress_out_17 0x0
	mov m.Ingress_ap_memb_24 0x0
	table tbl
	table ap
	mov m.Ingress_ap_memb_24 0x0
	table tbl2
	table ap
	jmpneq LABEL_DROP m.psa_ingress_out_15 0x0
	emit h.ethernet
	emit h.ipv4_base
	tx m.psa_ingress_out_17
	LABEL_DROP :	drop
}


