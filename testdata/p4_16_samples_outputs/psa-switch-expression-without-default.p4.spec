


struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
	bit<80> newfield
}

struct tcp_t {
	bit<16> srcPort
	bit<16> dstPort
	bit<32> seqNo
	bit<32> ackNo
	bit<16> dataOffset_res_ecn_ctrl
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
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

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct user_meta_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> local_metadata_data
	bit<48> MyIC_tbl_ethernet_srcAddr
	bit<16> Ingress_tmp
	bit<16> tmpMask
	bit<8> tmpMask_0
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

action NoAction args none {
	return
}

action a1 args instanceof a1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

table tbl {
	key {
		m.MyIC_tbl_ethernet_srcAddr exact
		m.local_metadata_data lpm
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table foo {
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table bar {
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	mov m.tmpMask h.ethernet.etherType
	and m.tmpMask 0xF00
	jmpeq MYIP_PARSE_IPV4 m.tmpMask 0x800
	jmpeq MYIP_PARSE_TCP h.ethernet.etherType 0xD00
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4 :	extract h.ipv4
	mov m.tmpMask_0 h.ipv4.protocol
	and m.tmpMask_0 0xFC
	jmpeq MYIP_PARSE_TCP m.tmpMask_0 0x4
	jmp MYIP_ACCEPT
	MYIP_PARSE_TCP :	extract h.tcp
	MYIP_ACCEPT :	mov m.Ingress_tmp 0x10
	mov m.Ingress_tmp 0x1
	mov m.MyIC_tbl_ethernet_srcAddr h.ethernet.srcAddr
	table tbl
	jmpa LABEL_SWITCH a1
	jmpa LABEL_SWITCH_0 a2
	jmp LABEL_ENDSWITCH
	LABEL_SWITCH :	jmpneq LABEL_ENDSWITCH m.Ingress_tmp 0x1
	table foo
	jmp LABEL_ENDSWITCH
	jmp LABEL_ENDSWITCH
	LABEL_SWITCH_0 :	table bar
	LABEL_ENDSWITCH :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


