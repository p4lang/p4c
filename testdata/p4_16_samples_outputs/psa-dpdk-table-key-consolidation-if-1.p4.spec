


struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<4> version
	bit<4> ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<3> flags
	bit<13> fragOffset
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
	bit<4> dataOffset
	bit<3> res
	bit<3> ecn
	bit<6> ctrl
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct user_meta_t {
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<8> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_class_of_service
	bit<8> psa_ingress_output_metadata_clone
	bit<16> psa_ingress_output_metadata_clone_session_id
	bit<8> psa_ingress_output_metadata_drop
	bit<8> psa_ingress_output_metadata_resubmit
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> psa_egress_input_metadata_class_of_service
	bit<32> psa_egress_input_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<16> psa_egress_input_metadata_instance
	bit<64> psa_egress_input_metadata_egress_timestamp
	bit<8> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
	bit<16> local_metadata_data
	bit<16> local_metadata_data1
	bit<48> Ingress_tbl_ethernet_srcAddr
	bit<48> Ingress_foo_ethernet_dstAddr
	bit<16> tmpMask
	bit<8> tmpMask_0
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct a3_arg_t {
	bit<48> param
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

action a3 args instanceof a3_arg_t {
	mov h.ethernet.srcAddr t.param
	return
}

table tbl {
	key {
		m.Ingress_tbl_ethernet_srcAddr exact
		m.local_metadata_data exact
		m.local_metadata_data1 lpm
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
	key {
		m.Ingress_foo_ethernet_dstAddr exact
		m.local_metadata_data exact
		m.local_metadata_data1 lpm
	}
	actions {
		NoAction
		a3
		a2
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
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	mov m.tmpMask h.ethernet.etherType
	and m.tmpMask 0xf00
	jmpeq MYIP_PARSE_IPV4 m.tmpMask 0x800
	jmpeq MYIP_PARSE_TCP h.ethernet.etherType 0xd00
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4 :	extract h.ipv4
	mov m.tmpMask_0 h.ipv4.protocol
	and m.tmpMask_0 0xfc
	jmpeq MYIP_PARSE_TCP m.tmpMask_0 0x4
	jmp MYIP_ACCEPT
	MYIP_PARSE_TCP :	extract h.tcp
	MYIP_ACCEPT :	mov m.Ingress_tbl_ethernet_srcAddr h.ethernet.srcAddr
	table tbl
	jmpnh LABEL_0END
	mov m.Ingress_foo_ethernet_dstAddr h.ethernet.dstAddr
	table foo
	LABEL_0END :	table tbl
	jmpnh LABEL_1END
	table foo
	LABEL_1END :	table tbl
	jmpnh LABEL_2FALSE
	jmp LABEL_2END
	LABEL_2FALSE :	table bar
	LABEL_2END :	table tbl
	jmpnh LABEL_3FALSE
	jmp LABEL_3END
	LABEL_3FALSE :	table bar
	LABEL_3END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


