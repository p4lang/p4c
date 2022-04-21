
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
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

struct macswp_arg_t {
	bit<32> tmp1
	bit<32> tmp2
}

header ethernet instanceof ethernet_t

struct user_meta_data_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<48> local_metadata_addr
	bit<32> Ingress_tmp
}
metadata instanceof user_meta_data_t

action nonDefAct args none {
	mov m.local_metadata_addr h.ethernet.dst_addr
	mov h.ethernet.dst_addr h.ethernet.src_addr
	mov h.ethernet.src_addr m.local_metadata_addr
	return
}

action macswp args instanceof macswp_arg_t {
	jmpneq LABEL_END t.tmp1 0x1
	jmpneq LABEL_END t.tmp2 0x2
	mov m.local_metadata_addr h.ethernet.dst_addr
	mov h.ethernet.dst_addr h.ethernet.src_addr
	mov h.ethernet.src_addr m.local_metadata_addr
	LABEL_END :	return
}

table stub {
	key {
	}
	actions {
		macswp
		nonDefAct
	}
	default_action macswp args tmp1 0x1 tmp2 0x2 const
	size 0xf4240
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	mov m.Ingress_tmp m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp
	xor m.psa_ingress_output_metadata_egress_port 0x1
	table stub
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


