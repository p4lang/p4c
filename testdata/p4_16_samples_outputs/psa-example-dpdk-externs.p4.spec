





struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<4> version
	bit<4> ihl
	bit<8> diffserv
	bit<32> totalLen
	bit<16> identification
	bit<3> flags
	bit<13> fragOffset
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

struct execute_arg_t {
	bit<12> index
}

struct metadata_t {
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<16> psa_ingress_input_metadata_parser_error
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
	bit<16> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
	bit<32> local_metadata_port_in
	bit<32> local_metadata_port_out
	bit<32> Ingress_color_out_0
	bit<32> Ingress_color_in_0
	bit<32> Ingress_tmp
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

action execute args instanceof execute_arg_t {
	meter meter0_0 t.index h.ipv4.totalLen m.Ingress_color_in_0 m.Ingress_color_out_0
	jmpneq LABEL_FALSE_0 m.Ingress_color_out_0 0x0
	mov m.Ingress_tmp 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.Ingress_tmp 0x0
	LABEL_END_0 :	mov m.local_metadata_port_out m.Ingress_tmp
	regwr reg_0 t.index m.local_metadata_port_out
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		execute
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	INGRESSPARSERIMPL_ACCEPT :	mov m.Ingress_color_in_0 0x2
	jmpneq LABEL_END m.local_metadata_port_out 0x1
	table tbl
	regadd counter0_0_packets 0x3ff 1
	regadd counter0_0_bytes 0x3ff 0x14
	regadd counter1_0 0x200 1
	regadd counter2_0 0x3ff 0x40
	regrd m.local_metadata_port_out reg_0 0x1
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


