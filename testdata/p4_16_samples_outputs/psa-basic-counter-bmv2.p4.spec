

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
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

regarray counter_0 size 0x400 initval 0x0
action execute_1 args none {
	regadd counter_0 0x100 1
	return
}

table tbl {
	actions {
		execute_1
	}
	default_action execute_1 args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xFFFFFFFF
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFFFFFFFF
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	and m.Ingress_tmp_1 0xFFFFFFFF
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp_1
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


