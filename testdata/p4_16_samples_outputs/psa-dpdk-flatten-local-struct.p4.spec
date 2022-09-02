
struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_t {
	bit<8> ver_ihl
	bit<8> diffserv
	bit<16> total_len
	bit<16> identification
	bit<16> flags_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdr_checksum
	bit<32> src_addr
	bit<32> dst_addr
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

struct action1_arg_t {
	bit<16> field
	bit<16> field1
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header outer_ethernet instanceof ethernet_t
header outer_ipv4 instanceof ipv4_t

struct local_metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> Ingress_host_info_rx_bytes_0_flex_up_flex_up1_flex_0
}
metadata instanceof local_metadata_t

action action1 args instanceof action1_arg_t {
	jmpneq LABEL_FALSE t.field 0x1
	mov m.Ingress_host_info_rx_bytes_0_flex_up_flex_up1_flex_0 t.field
	jmp LABEL_END
	LABEL_FALSE :	mov m.Ingress_host_info_rx_bytes_0_flex_up_flex_up1_flex_0 t.field1
	LABEL_END :	mov h.outer_ethernet.ether_type m.Ingress_host_info_rx_bytes_0_flex_up_flex_up1_flex_0
	return
}

action drop_1 args none {
	mov m.psa_ingress_output_metadata_egress_port 0x4
	return
}

table table1 {
	key {
		h.ethernet.dst_addr exact
	}
	actions {
		action1
		drop_1
	}
	default_action drop_1 args none const
	size 0x100000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	table table1
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


