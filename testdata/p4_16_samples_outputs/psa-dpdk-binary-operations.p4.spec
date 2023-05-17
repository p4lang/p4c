
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

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata_meta
	bit<32> local_metadata_meta1
	bit<16> local_metadata_meta2
	bit<32> local_metadata_meta3
	bit<32> local_metadata_meta4
	bit<16> local_metadata_meta5
	bit<32> local_metadata_meta6
	bit<16> local_metadata_meta7
	bit<16> Ingress_tmp
	bit<32> Ingress_tmp_1
	bit<16> Ingress_tmp_2
	bit<32> Ingress_tmp_4
}
metadata instanceof metadata

action NoAction args none {
	return
}

action forward args none {
	return
}

table tbl {
	key {
		h.srcAddr exact
	}
	actions {
		NoAction
		forward
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h
	table tbl
	mov m.local_metadata_meta 0x1
	shl m.local_metadata_meta m.local_metadata_meta2
	mov m.local_metadata_meta1 0x800
	shr m.local_metadata_meta1 m.local_metadata_meta2
	mov m.Ingress_tmp 0xF0
	sub m.Ingress_tmp m.local_metadata_meta2
	mov m.local_metadata_meta2 m.Ingress_tmp
	mov m.local_metadata_meta4 0x808
	add m.local_metadata_meta4 m.local_metadata_meta6
	mov m.local_metadata_meta6 0x808
	sub m.local_metadata_meta6 m.local_metadata_meta3
	add m.local_metadata_meta3 0x1
	mov m.local_metadata_meta5 m.local_metadata_meta7
	add m.local_metadata_meta5 0xF0
	mov m.local_metadata_meta7 0xF0
	add m.local_metadata_meta7 m.local_metadata_meta2
	mov h.dstAddr m.local_metadata_meta
	mov h.srcAddr m.local_metadata_meta1
	mov h.etherType m.local_metadata_meta2
	mov m.Ingress_tmp_1 m.local_metadata_meta2
	shl m.Ingress_tmp_1 0x10
	mov m.Ingress_tmp_2 0xF0
	add m.Ingress_tmp_2 m.local_metadata_meta2
	mov m.Ingress_tmp_4 m.Ingress_tmp_2
	and m.Ingress_tmp_4 0xFFFF
	mov m.local_metadata_meta m.Ingress_tmp_1
	or m.local_metadata_meta m.Ingress_tmp_4
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


