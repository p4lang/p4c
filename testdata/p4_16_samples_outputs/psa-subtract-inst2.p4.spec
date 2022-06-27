
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct udp_t {
	bit<16> sport
	bit<16> dport
	bit<16> length
	bit<16> csum
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
header udp_0 instanceof udp_t
header udp_1 instanceof udp_t


struct user_meta_data_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata_depth1
	bit<32> Ingress_tmp
	bit<32> Ingress_var1
	bit<32> Ingress_var2
	bit<32> Ingress_var3
	bit<32> Ingress_var4
}
metadata instanceof user_meta_data_t

action nonDefAct args none {
	jmpneq LABEL_END m.Ingress_var4 0x3
	mov m.local_metadata_depth1 m.Ingress_var1
	add m.local_metadata_depth1 m.Ingress_var2
	and m.local_metadata_depth1 0x7
	LABEL_END :	return
}

table stub {
	key {
	}
	actions {
		nonDefAct
	}
	default_action nonDefAct args none const
	size 0xf4240
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	mov m.Ingress_var1 0x0
	mov m.Ingress_var2 0x2
	mov m.Ingress_var4 m.Ingress_var3
	add m.Ingress_var4 0xfffffffd
	mov m.Ingress_tmp m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp
	xor m.psa_ingress_output_metadata_egress_port 0x1
	table stub
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


