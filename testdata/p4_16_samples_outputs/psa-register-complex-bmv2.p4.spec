

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct metadata_t {
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
	bit<48> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<48> Ingress_tmp_4
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<48> Ingress_tmp
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

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

regarray regfile_0 size 0x80 initval 0

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	regwr regfile_0 0x1 0x3
	regwr regfile_0 0x2 0x4
	regrd m.Ingress_tmp regfile_0 0x1
	regrd m.Ingress_tmp_0 regfile_0 0x2
	mov m.Ingress_tmp_1 m.Ingress_tmp
	add m.Ingress_tmp_1 m.Ingress_tmp_0
	mov h.ethernet.dstAddr m.Ingress_tmp_1
	add h.ethernet.dstAddr 0xfffffffffffb
	regrd m.Ingress_tmp_2 regfile_0 0x2
	mov m.Ingress_tmp_3 m.Ingress_tmp
	add m.Ingress_tmp_3 m.Ingress_tmp_2
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	add m.Ingress_tmp_4 0xfffffffffffb
	jmpneq LABEL_0END m.Ingress_tmp_4 0x2
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	cast  h.ethernet.dstAddr bit_32 m.psa_ingress_output_metadata_egress_port
	LABEL_0END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	extract h.ethernet
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


