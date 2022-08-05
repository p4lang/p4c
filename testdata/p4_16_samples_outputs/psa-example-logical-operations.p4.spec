
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
	bit<48> local_metadata_meta
	bit<48> local_metadata_meta2
	bit<48> local_metadata_meta3
}
metadata instanceof metadata

action NoAction args none {
	return
}

action forward args none {
	mov m.local_metadata_meta h.srcAddr
	or m.local_metadata_meta h.dstAddr
	mov m.local_metadata_meta2 h.srcAddr
	and m.local_metadata_meta2 h.dstAddr
	mov m.local_metadata_meta3 h.srcAddr
	xor m.local_metadata_meta3 h.dstAddr
	return
}

table tbl {
	key {
		h.srcAddr exact
	}
	actions {
		forward
		NoAction
	}
	default_action NoAction args none const
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h
	table tbl
	mov h.dstAddr m.local_metadata_meta
	mov h.srcAddr m.local_metadata_meta2
	mov h.etherType m.local_metadata_meta3
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


