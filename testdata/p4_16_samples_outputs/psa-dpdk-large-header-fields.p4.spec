
struct ethernet_t {
	bit<8> x0
	bit<16> x1_x2_x3
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
	bit<88> x4_x5
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

struct user_meta_data_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<128> local_metadata_k1
	bit<128> local_metadata_k2
	bit<48> local_metadata_addr
	bit<8> local_metadata_x2
	bit<16> Ingress_tmp
	bit<16> Ingress_tmp_0
	bit<16> Ingress_tmp_1
	bit<16> Ingress_tmp_2
	bit<8> Ingress_flg
}
metadata instanceof user_meta_data_t

action NoAction_1 args none {
	return
}

action macswp args none {
	jmpneq LABEL_END m.Ingress_flg 0x2
	mov m.local_metadata_k1 m.local_metadata_k2
	mov m.local_metadata_x2 h.ethernet.x0
	mov m.Ingress_tmp h.ethernet.ether_type
	shr m.Ingress_tmp 0x2
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFF
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	and m.Ingress_tmp_1 0xFF
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	and m.Ingress_tmp_2 0xFF
	mov m.local_metadata_x2 m.Ingress_tmp_2
	mov m.local_metadata_addr h.ethernet.dst_addr
	mov h.ethernet.dst_addr h.ethernet.src_addr
	mov h.ethernet.src_addr m.local_metadata_addr
	LABEL_END :	return
}

table stub_0 {
	key {
		m.local_metadata_k1 exact
	}
	actions {
		macswp
		NoAction_1
	}
	default_action NoAction_1 args none 
	size 0xF4240
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	mov m.Ingress_flg 0x0
	table stub_0
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


