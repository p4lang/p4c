
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
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
	bit<8> psa_ingress_output_metadata_clone
	bit<16> psa_ingress_output_metadata_clone_session_id
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<48> Ingress_tmp
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	mov m.psa_ingress_output_metadata_clone 1
	mov m.psa_ingress_output_metadata_clone_session_id 0x8
	jmpneq LABEL_FALSE h.ethernet.dstAddr 0x9
	mov m.psa_ingress_output_metadata_drop 1
	jmp LABEL_END
	LABEL_FALSE :	mov h.ethernet.srcAddr 0xcafe
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xffffffff
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	extract h.ethernet
	jmpneq LABEL_END_0 m.psa_egress_input_metadata_packet_path 0x3
	mov h.ethernet.etherType 0xface
	LABEL_END_0 :	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


