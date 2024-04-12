





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

struct execute_1_arg_t {
	bit<32> index
}

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata_port_out
	bit<48> ingress_tbl_ethernet_srcAddr
	bit<32> ingress_tbl_key
	bit<32> ingress_tbl_key_0
	bit<8> IngressParser_parser_tmp
	bit<8> IngressParser_parser_tmp_0
	bit<8> IngressParser_parser_tmp_1
	bit<32> IngressParser_parser_tmp_2
	bit<16> Ingress_tmp
	bit<16> Ingress_tmp_0
	bit<8> Ingress_tmp_2
	bit<8> Ingress_tmp_3
	bit<8> Ingress_tmp_4
	bit<8> Ingress_tmp_6
	bit<8> Ingress_tmp_7
	bit<8> Ingress_tmp_8
	bit<8> Ingress_tmp_10
	bit<8> Ingress_tmp_11
	bit<8> Ingress_tmp_12
	bit<16> Ingress_tmp_13
	bit<8> Ingress_tmp_14
	bit<8> Ingress_tmp_15
	bit<8> Ingress_tmp_16
	bit<32> Ingress_tmp_18
	bit<16> Ingress_tmp_20
	bit<16> Ingress_tmp_21
	bit<16> Ingress_tmp_22
	bit<8> Ingress_tmp_23
	bit<8> Ingress_tmp_24
	bit<8> Ingress_tmp_25
	bit<32> Ingress_color_out
	bit<32> Ingress_color_in
	bit<32> Ingress_tmp_27
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray counter0_0_packets size 0x400 initval 0x0

regarray counter0_0_bytes size 0x400 initval 0x0
regarray counter1_0 size 0x400 initval 0x0
regarray counter2_0 size 0x400 initval 0x0
regarray reg_0 size 0x400 initval 0
metarray meter0_0 size 0x400
action NoAction args none {
	return
}

action execute_1 args instanceof execute_1_arg_t {
	mov m.Ingress_tmp_10 h.ipv4.version_ihl
	and m.Ingress_tmp_10 0xF0
	mov h.ipv4.version_ihl m.Ingress_tmp_10
	or h.ipv4.version_ihl 0x5
	meter meter0_0 t.index h.ipv4.totalLen m.Ingress_color_in m.Ingress_color_out
	jmpneq LABEL_FALSE_1 m.Ingress_color_out 0x0
	mov m.Ingress_tmp_27 0x1
	jmp LABEL_END_1
	LABEL_FALSE_1 :	mov m.Ingress_tmp_27 0x0
	LABEL_END_1 :	mov m.local_metadata_port_out m.Ingress_tmp_27
	regwr reg_0 t.index m.local_metadata_port_out
	mov m.Ingress_tmp h.ipv4.hdrChecksum
	and m.Ingress_tmp 0x3F
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0x3F
	jmpneq LABEL_END_2 m.Ingress_tmp_0 0x6
	mov m.Ingress_tmp_11 h.ipv4.version_ihl
	and m.Ingress_tmp_11 0xF0
	mov h.ipv4.version_ihl m.Ingress_tmp_11
	or h.ipv4.version_ihl 0x5
	LABEL_END_2 :	mov m.Ingress_tmp_2 h.ipv4.version_ihl
	shr m.Ingress_tmp_2 0x4
	mov m.Ingress_tmp_3 m.Ingress_tmp_2
	and m.Ingress_tmp_3 0xF
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	and m.Ingress_tmp_4 0xF
	jmpneq LABEL_END_3 m.Ingress_tmp_4 0x6
	mov m.Ingress_tmp_12 h.ipv4.version_ihl
	and m.Ingress_tmp_12 0xF0
	mov h.ipv4.version_ihl m.Ingress_tmp_12
	or h.ipv4.version_ihl 0x6
	LABEL_END_3 :	return
}

table tbl {
	key {
		m.ingress_tbl_ethernet_srcAddr exact
		m.ingress_tbl_key exact
		m.ingress_tbl_key_0 exact
	}
	actions {
		NoAction
		execute_1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	mov m.IngressParser_parser_tmp h.ipv4.version_ihl
	shr m.IngressParser_parser_tmp 0x4
	mov m.IngressParser_parser_tmp_0 m.IngressParser_parser_tmp
	and m.IngressParser_parser_tmp_0 0xF
	mov m.IngressParser_parser_tmp_1 m.IngressParser_parser_tmp_0
	and m.IngressParser_parser_tmp_1 0xF
	mov m.IngressParser_parser_tmp_2 m.IngressParser_parser_tmp_1
	INGRESSPARSERIMPL_ACCEPT :	mov m.Ingress_color_in 0x2
	jmpneq LABEL_END m.local_metadata_port_out 0x1
	mov m.Ingress_tmp_21 h.ipv4.hdrChecksum
	and m.Ingress_tmp_21 0x1F
	mov m.Ingress_tmp_22 m.Ingress_tmp_21
	and m.Ingress_tmp_22 0x1F
	mov m.Ingress_tmp_23 h.ipv4.version_ihl
	shr m.Ingress_tmp_23 0x4
	mov m.Ingress_tmp_24 m.Ingress_tmp_23
	and m.Ingress_tmp_24 0xF
	mov m.Ingress_tmp_25 m.Ingress_tmp_24
	and m.Ingress_tmp_25 0xF
	mov m.ingress_tbl_ethernet_srcAddr h.ethernet.srcAddr
	mov m.ingress_tbl_key m.Ingress_tmp_22
	mov m.ingress_tbl_key_0 m.Ingress_tmp_25
	table tbl
	regadd counter0_0_packets 0x3FF 1
	regadd counter0_0_bytes 0x3FF 0x14
	regadd counter1_0 0x200 1
	regadd counter2_0 0x3FF 0x40
	regrd m.local_metadata_port_out reg_0 0x1
	mov m.Ingress_tmp_6 h.ipv4.version_ihl
	shr m.Ingress_tmp_6 0x4
	mov m.Ingress_tmp_7 m.Ingress_tmp_6
	and m.Ingress_tmp_7 0xF
	mov m.Ingress_tmp_8 m.Ingress_tmp_7
	and m.Ingress_tmp_8 0xF
	jmpneq LABEL_END m.Ingress_tmp_8 0x4
	mov m.Ingress_tmp_13 h.ipv4.hdrChecksum
	and m.Ingress_tmp_13 0xFFF0
	mov m.Ingress_tmp_14 h.ipv4.version_ihl
	shr m.Ingress_tmp_14 0x4
	mov m.Ingress_tmp_15 m.Ingress_tmp_14
	and m.Ingress_tmp_15 0xF
	mov m.Ingress_tmp_16 m.Ingress_tmp_15
	and m.Ingress_tmp_16 0xF
	mov m.Ingress_tmp_18 m.Ingress_tmp_16
	add m.Ingress_tmp_18 0x5
	and m.Ingress_tmp_18 0xF
	mov m.Ingress_tmp_20 m.Ingress_tmp_18
	and m.Ingress_tmp_20 0xF
	mov h.ipv4.hdrChecksum m.Ingress_tmp_13
	or h.ipv4.hdrChecksum m.Ingress_tmp_20
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	mov h.ipv4.hdrChecksum 0x4
	emit h.ethernet
	emit h.ipv4
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


