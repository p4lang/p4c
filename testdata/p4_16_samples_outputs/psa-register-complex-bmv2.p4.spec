

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
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<48> Ingress_tmp
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<48> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<48> Ingress_tmp_4
	bit<48> Ingress_tmp_5
	bit<48> Ingress_tmp_6
	bit<48> Ingress_tmp_7
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

regarray regfile_0 size 0x80 initval 0
apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	regwr regfile_0 0x1 0x3
	regwr regfile_0 0x2 0x4
	regrd m.Ingress_tmp_7 regfile_0 0x1
	regrd m.Ingress_tmp_5 regfile_0 0x2
	mov m.Ingress_tmp_6 m.Ingress_tmp_7
	add m.Ingress_tmp_6 m.Ingress_tmp_5
	mov h.ethernet.dstAddr m.Ingress_tmp_6
	add h.ethernet.dstAddr 0xFFFFFFFFFFFB
	regrd m.Ingress_tmp regfile_0 0x2
	mov m.Ingress_tmp_0 m.Ingress_tmp_7
	add m.Ingress_tmp_0 m.Ingress_tmp
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	add m.Ingress_tmp_1 0xFFFFFFFFFFFB
	jmpneq LABEL_END m.Ingress_tmp_1 0x2
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.Ingress_tmp_2 h.ethernet.dstAddr
	and m.Ingress_tmp_2 0xFFFFFFFF
	mov m.Ingress_tmp_3 m.Ingress_tmp_2
	and m.Ingress_tmp_3 0xFFFFFFFF
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	and m.Ingress_tmp_4 0xFFFFFFFF
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp_4
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


