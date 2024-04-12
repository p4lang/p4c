

struct ethernet_t {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct ipv4_t {
	;oldname:ver_ihl_ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd_k
	bit<8> ver_ihl_ddddddddddddddddddddd0
	;oldname:diffserv_dpdk_dpdk_dpdk_dpdk_dpdk
	bit<8> diffserv_dpdk_dpdk_dpdk_dpdk_1
	bit<16> total_len
	bit<16> identification
	bit<16> flags_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdr_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct vxlan_t {
	bit<8> flags
	bit<24> reserved
	bit<24> vni
	bit<8> reserved2
}

struct udp_t {
	bit<16> src_port
	bit<16> dst_port
	bit<16> length
	bit<16> checksum
}

struct cksum_state_t {
	bit<16> state_0
}

struct dpdk_pseudo_header_t {
	bit<16> pseudo
	bit<16> pseudo_0
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

;oldname:vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_arg_t
struct vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dp2 {
	bit<48> ethernet_dst_addr
	bit<48> ethernet_src_addr
	bit<16> ethernet_ether_type
	bit<8> ipv4_ver_ihl
	bit<8> ipv4_diffserv
	bit<16> ipv4_total_len
	bit<16> ipv4_identification
	bit<16> ipv4_flags_offset
	bit<8> ipv4_ttl
	bit<8> ipv4_protocol
	bit<16> ipv4_hdr_checksum
	bit<32> ipv4_src_addr
	bit<32> ipv4_dst_addr
	bit<16> udp_src_port
	bit<16> udp_dst_port
	bit<16> udp_length
	bit<16> udp_checksum
	bit<8> vxlan_flags
	bit<24> vxlan_reserved
	bit<24> vxlan_vni
	bit<8> vxlan_reserved2
	bit<32> port_out
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header vxlan instanceof vxlan_t
header outer_ethernet instanceof ethernet_t
;oldname:outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk
header outer_ipv4_dpdk_dpdk_dpdk_dpd3 instanceof ipv4_t
header outer_udp instanceof udp_t
header outer_vxlan instanceof vxlan_t
header cksum_state instanceof cksum_state_t
header dpdk_pseudo_header instanceof dpdk_pseudo_header_t

;oldname:local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t
struct local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpd4 {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> local_metadata_tmp
	bit<16> Ingress_tmp
	bit<16> Ingress_tmp_0
}
metadata instanceof local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpd4

;oldname:vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk
action vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dp5 args instanceof vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dp2 {
	mov h.outer_ethernet.src_addr t.ethernet_src_addr
	mov h.outer_ethernet.dst_addr t.ethernet_dst_addr
	mov h.outer_ethernet.ether_type t.ethernet_ether_type
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.ver_ihl_ddddddddddddddddddddd0 t.ipv4_ver_ihl
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.diffserv_dpdk_dpdk_dpdk_dpdk_1 t.ipv4_diffserv
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.total_len t.ipv4_total_len
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.identification t.ipv4_identification
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.flags_offset t.ipv4_flags_offset
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.ttl t.ipv4_ttl
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.protocol t.ipv4_protocol
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.hdr_checksum t.ipv4_hdr_checksum
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.src_addr t.ipv4_src_addr
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.dst_addr t.ipv4_dst_addr
	mov h.outer_udp.src_port t.udp_src_port
	mov h.outer_udp.dst_port t.udp_dst_port
	mov h.outer_udp.length t.udp_length
	mov h.outer_udp.checksum t.udp_checksum
	mov h.vxlan.flags t.vxlan_flags
	mov h.vxlan.reserved t.vxlan_reserved
	mov h.vxlan.vni t.vxlan_vni
	mov h.vxlan.reserved2 t.vxlan_reserved2
	mov m.psa_ingress_output_metadata_egress_port t.port_out
	mov m.Ingress_tmp h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.hdr_checksum
	mov m.Ingress_tmp_0 h.ipv4.total_len
	mov h.dpdk_pseudo_header.pseudo m.Ingress_tmp
	mov h.dpdk_pseudo_header.pseudo_0 m.Ingress_tmp_0
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_0
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.hdr_checksum h.cksum_state.state_0
	mov h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.total_len t.ipv4_total_len
	add h.outer_ipv4_dpdk_dpdk_dpdk_dpd3.total_len h.ipv4.total_len
	mov h.outer_udp.length t.udp_length
	add h.outer_udp.length h.ipv4.total_len
	return
}

action drop_1 args none {
	mov m.psa_ingress_output_metadata_egress_port 0x4
	return
}

;oldname:vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk
table vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpd6 {
	key {
		h.ethernet.dst_addr exact
	}
	actions {
		vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dp5
		drop_1
	}
	default_action drop_1 args none const
	size 0x100000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	extract h.ipv4
	table vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpd6
	jmpa LABEL_SWITCH vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dp5
	jmp LABEL_ENDSWITCH
	LABEL_SWITCH :	mov m.local_metadata_tmp 0x0
	LABEL_ENDSWITCH :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.outer_ethernet
	emit h.outer_ipv4_dpdk_dpdk_dpdk_dpd3
	emit h.outer_udp
	emit h.outer_vxlan
	emit h.ethernet
	emit h.ipv4
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


