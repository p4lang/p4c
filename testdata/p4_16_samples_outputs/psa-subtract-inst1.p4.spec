
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
	bit<32> local_metadata_depth2
	bit<32> local_metadata_depth3
	bit<32> local_metadata_depth4
	bit<32> local_metadata_depth5
	bit<32> local_metadata_depth6
	bit<32> local_metadata_depth7
	bit<64> Ingress_tmp
	bit<32> Ingress_var1
	bit<32> Ingress_var2
	bit<64> Ingress_var3
	bit<64> Ingress_var4
	bit<64> Ingress_var5
}
metadata instanceof user_meta_data_t

action nonDefAct args none {
	jmpneq LABEL_END m.Ingress_var4 0x3
	jmpneq LABEL_END m.Ingress_var5 0x1
	add m.local_metadata_depth3 0x1D
	and m.local_metadata_depth3 0x1F
	mov m.local_metadata_depth4 m.Ingress_var1
	add m.local_metadata_depth4 m.Ingress_var2
	and m.local_metadata_depth4 0x1F
	mov m.local_metadata_depth5 m.Ingress_var1
	xor m.local_metadata_depth5 0x2
	mov m.local_metadata_depth6 m.Ingress_var2
	add m.local_metadata_depth6 0x1F
	and m.local_metadata_depth6 0x1F
	mov m.local_metadata_depth7 m.Ingress_var2
	add m.local_metadata_depth7 0x3
	and m.local_metadata_depth7 0x1F
	LABEL_END :	return
}

table stub {
	actions {
		nonDefAct
	}
	default_action nonDefAct args none const
	size 0xF4240
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	add m.local_metadata_depth1 0x7
	and m.local_metadata_depth1 0x7
	extract h.ethernet
	mov m.Ingress_var1 0x8
	mov m.Ingress_var2 0x2
	mov m.Ingress_var4 m.Ingress_var3
	add m.Ingress_var4 0xFFFFFFFFFFFFFFFD
	mov m.Ingress_tmp m.Ingress_var3
	add m.Ingress_tmp 0xFFFFFFFFFFFFFFFD
	mov m.Ingress_var5 m.Ingress_tmp
	and m.Ingress_var3 0x100000000
	shr m.Ingress_var3 0x20
	jmpeq LABEL_TRUE m.Ingress_var3 0x1
	or m.Ingress_var3 0xFFFFFFFE00000000
	LABEL_TRUE :	sub m.Ingress_var5 m.Ingress_var3
	mov m.local_metadata_depth4 m.local_metadata_depth2
	add m.local_metadata_depth4 0x1C
	and m.local_metadata_depth4 0x1F
	mov m.psa_ingress_output_metadata_egress_port m.psa_ingress_input_metadata_ingress_port
	xor m.psa_ingress_output_metadata_egress_port 0x1
	table stub
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


