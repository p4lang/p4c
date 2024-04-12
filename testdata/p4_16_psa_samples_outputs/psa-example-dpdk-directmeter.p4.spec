

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

header ethernet instanceof ethernet_t

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata_port_out
	bit<32> table_entry_index
	bit<32> Ingress_color_out
	bit<32> Ingress_color_in
	bit<32> Ingress_tmp
}
metadata instanceof metadata_t

metarray meter0_0 size 0x10001
action NoAction args none {
	return
}

action execute_meter args none {
	entryid m.table_entry_index 
	meter meter0_0 m.table_entry_index 0x400 m.Ingress_color_in m.Ingress_color_out
	jmpneq LABEL_FALSE m.Ingress_color_out 0x0
	mov m.Ingress_tmp 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.Ingress_tmp 0x0
	LABEL_END :	mov m.local_metadata_port_out m.Ingress_tmp
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		execute_meter
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	mov m.Ingress_color_in 0x2
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


