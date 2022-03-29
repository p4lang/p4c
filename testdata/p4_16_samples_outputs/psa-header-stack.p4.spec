
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct vlan_tag_h {
	bit<16> pcp_cfi_vid
	bit<16> ether_type
}

struct psa_ingress_out_0 {
	bit<8> class_of_servic_1
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_3 {
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
}

struct psa_egress_depa_4 {
	bit<32> egress_port
}

struct EMPTY_M {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
}
metadata instanceof EMPTY_M

header ethernet instanceof ethernet_t
header vlan_tag_0 instanceof vlan_tag_h
header vlan_tag_1 instanceof vlan_tag_h


action NoAction args none {
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	jmpeq MYIP_PARSE_VLAN_TAG h.ethernet.etherType 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG :	extract h.vlan_tag_0
	jmpeq MYIP_PARSE_VLAN_TAG1 h.vlan_tag_0.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG1 :	extract h.vlan_tag_1
	jmpeq MYIP_PARSE_VLAN_TAG2 h.vlan_tag_1.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG2 :	mov m.psa_ingress_i_8 0x3
	MYIP_ACCEPT :	jmpnv LABEL_FALSE h.ethernet
	jmp LABEL_END_0
	LABEL_FALSE :	table tbl
	LABEL_END_0 :	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


