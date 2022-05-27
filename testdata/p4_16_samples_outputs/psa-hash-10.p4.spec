

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<32> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
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

struct user_meta_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> local_metadata_data
	bit<48> ethernet_t_dstAddr
	bit<48> ethernet_t_srcAddr
	bit<16> ethernet_t_etherType
	bit<8> ipv4_t_version_ihl
	bit<8> ipv4_t_diffserv
	bit<32> ipv4_t_totalLen
	bit<16> ipv4_t_identification
	bit<16> ipv4_t_flags_fragOffset
	bit<8> ipv4_t_ttl
	bit<8> ipv4_t_protocol
	bit<16> ipv4_t_hdrChecksum
	bit<32> ipv4_t_srcAddr
	bit<32> ipv4_t_dstAddr
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header Ingress_tmp instanceof ethernet_t
header Ingress_tmp_0 instanceof ipv4_t

action NoAction args none {
	return
}

action a1 args none {
	mov m.ethernet_t_dstAddr h.Ingress_tmp.dstAddr
	mov m.ethernet_t_srcAddr h.Ingress_tmp.srcAddr
	mov m.ethernet_t_etherType h.Ingress_tmp.etherType
	mov m.ipv4_t_version_ihl h.Ingress_tmp_0.version_ihl
	mov m.ipv4_t_diffserv h.Ingress_tmp_0.diffserv
	mov m.ipv4_t_totalLen h.Ingress_tmp_0.totalLen
	mov m.ipv4_t_identification h.Ingress_tmp_0.identification
	mov m.ipv4_t_flags_fragOffset h.Ingress_tmp_0.flags_fragOffset
	mov m.ipv4_t_ttl h.Ingress_tmp_0.ttl
	mov m.ipv4_t_protocol h.Ingress_tmp_0.protocol
	mov m.ipv4_t_hdrChecksum h.Ingress_tmp_0.hdrChecksum
	mov m.ipv4_t_srcAddr h.Ingress_tmp_0.srcAddr
	mov m.ipv4_t_dstAddr h.Ingress_tmp_0.dstAddr
	hash crc32 m.local_metadata_data  m.ethernet_t_dstAddr m.ipv4_t_dstAddr
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


